#include "modules.hpp"
#include <iostream>
#include <sstream>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
namespace pt = boost::property_tree;

Module::Module(const std::string &_name)
    : name(_name)
{
    
}

void
Module::setName(const std::string &newName)
{
    this->name = newName;
}

bool
Module::setType(const std::string &newType)
{
    if (newType != "int" && newType != "double" && newType != "string") {
        return false;
    }
    this->type = newType;
    return true;
}

bool
Module::setSubtype(const std::string &newSubtype)
{
    if (newSubtype != "allow" && newSubtype != "range" && newSubtype != "free") {
        return false;
    }
    this->subtype = newSubtype;
    return true;
}


bool
Module::setStatus(bool newStatus)
{
    if (newStatus && this->type.size() != 0 /*&& this->subtype.size() != 0*/ && this->name.size() != 0 && this->format.size() != 0) {
        this->status = newStatus;
        return true;
    }
    this->status = false;
    return false;
}

void
Module::setFormat(const std::string &newFormat)
{
    this->format = newFormat;
}

ModulesManager::ModulesManager()
    : userIDs(UserIDs::getInstance())
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "RedisClient error: " << this->context->errstr
                      << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "RedisClient can't allocate redis context"
                      << std::endl;
        }
    }
    this->loadAllModules();
}

ModulesManager::~ModulesManager()
{
    if (!this->context)
        return;
    redisFree(this->context);
}

bool
ModulesManager::createModule(const std::string &name)
{
    std::lock_guard<std::mutex> lock(this->access);
    auto p = this->modules.emplace(std::make_pair(name, Module(name)));
    return p.second;
}

bool
ModulesManager::setType(const std::string &moduleName, const std::string &type)
{
    std::lock_guard<std::mutex> lock(this->access);
    auto search = this->modules.find(moduleName);
    if (search != this->modules.end()) {
        return search->second.setType(type);
    }
    return false;
}

bool
ModulesManager::setSubtype(const std::string &moduleName, const std::string &subtype)
{
    std::lock_guard<std::mutex> lock(this->access);
    auto search = this->modules.find(moduleName);
    if (search != this->modules.end()) {
        return search->second.setSubtype(subtype);
    }
    return false;
}

bool
ModulesManager::setStatus(const std::string &moduleName, const std::string &status)
{
    std::lock_guard<std::mutex> lock(this->access);
    auto search = this->modules.find(moduleName);
    if (search != this->modules.end()) {
        if (status == "on" || status == "online" || status == "true")
        {
            return search->second.setStatus(true);
        }
        return search->second.setStatus(false);
    }
    return false;
}

bool
ModulesManager::setName(const std::string &moduleName, const std::string &newName)
{
    std::lock_guard<std::mutex> lock(this->access);
    auto search = this->modules.find(moduleName);
    if (search != this->modules.end() && this->modules.count(newName) == 0) {
        search->second.setName(newName);
        return true;
    }
    return false;
}

bool
ModulesManager::setFormat(const std::string &moduleName, const std::string &format)
{
    std::lock_guard<std::mutex> lock(this->access);
    auto search = this->modules.find(moduleName);
    if (search != this->modules.end()) {
        search->second.setFormat(format);
        return true;
    }
    return false;
}

std::string
ModulesManager::getInfo(const std::string &moduleName)
{
    std::lock_guard<std::mutex> lock(this->access);
    auto search = this->modules.find(moduleName);
    if (search != this->modules.end()) {
        return search->second.getName() + 
               ": online: " + std::to_string(search->second.getStatus()) + 
               ", type: " +search->second.getType() +
               ", subtype: " + search->second.getSubtype() + 
               ", format: " + search->second.getFormat();
    }
    return "no such module found";
}

std::pair<bool, std::string>
ModulesManager::getData(const std::string &user, const std::string &moduleName)
{
    std::pair<bool, std::string> p;
    std::lock_guard<std::mutex> lock(this->access);
    auto search = this->modules.find(moduleName);
    if (search != this->modules.end()) {
        std::string userIDstr = this->userIDs.getID(user);
        if(userIDstr.empty()) {
            p.first = true;
            p.second = "user " + user + " not found";
            return p;
        }
        redisReply *reply = static_cast<redisReply *>(redisCommand(this->context,
            "HGET WNMA:modulesData:%b %b", userIDstr.c_str(), userIDstr.size(), moduleName.c_str(), moduleName.size()));
        if (reply && reply->type == REDIS_REPLY_STRING) {
            std::string data(reply->str, reply->len);
            p.first = true;
            p.second = search->second.getFormat();
            boost::algorithm::replace_all(p.second, "{user}", user);
            boost::algorithm::replace_all(p.second, "{value}", data);
            freeReplyObject(reply);
            return p;
        } else {
            p.first = true;
            p.second = "no " + moduleName + " data found for " + user;
            freeReplyObject(reply);
            return p;
        }
    }
    p.first = false;
    p.second = "no such module found";
    return p;
}

std::string
ModulesManager::deleteData(const std::string &user, const std::string &moduleName)
{
    std::pair<bool, std::string> p;
    std::lock_guard<std::mutex> lock(this->access);
    auto search = this->modules.find(moduleName);
    if (search != this->modules.end()) {
        std::string userIDstr = this->userIDs.getID(user);
        if(userIDstr.empty()) {
            return "user " + user + " not found";
        }
        redisReply *reply = static_cast<redisReply *>(redisCommand(this->context,
            "HDEL WNMA:modulesData:%b %b", userIDstr.c_str(), userIDstr.size(), moduleName.c_str(), moduleName.size()));
        freeReplyObject(reply);
        return moduleName + " data for " + user + " deleted FeelsBadMan";
    }
    return "no such module found";
}

std::string
ModulesManager::deleteDataFull(const std::string &user)
{
    std::lock_guard<std::mutex> lock(this->access);
    std::string userIDstr = this->userIDs.getID(user);
    if (userIDstr.empty()) {
        return "user " + user + " not found";
    }
    redisReply *reply = static_cast<redisReply *>(redisCommand(this->context, "DEL WNMA:modulesData:%b", userIDstr.c_str(), userIDstr.size()));
    freeReplyObject(reply);
    return "All module data for " + user + " was deleted";
}

std::string
ModulesManager::getAllData(const std::string &user)
{
    std::string ret;
    std::lock_guard<std::mutex> lock(this->access);
    std::string userIDstr = this->userIDs.getID(user);
    if(userIDstr.empty()) {
        ret = "user " + user + " not found";
    } else {
        for (const auto &i : this->modules) {
            if (i.second.getStatus() == false) {
                continue;
            }
            redisReply *reply = static_cast<redisReply *>(redisCommand(this->context,
                "HGET WNMA:modulesData:%b %b", userIDstr.c_str(), userIDstr.size(), i.first.c_str(), i.first.size()));
            if (reply && reply->type == REDIS_REPLY_STRING) {
                std::string data(reply->str, reply->len);
                ret += i.first + ": " + data + " ";
            }
            freeReplyObject(reply);
        }
        if (ret.back() == ' ') {
            ret.pop_back();
        }
        if (ret.empty()) {
            ret = "No data for " + user + " found eShrug";
        } else {
            ret = "Data for " + user + ": " + ret;
        }
    }
    return ret;
}

std::string
ModulesManager::setData(const std::string &user, const std::string &moduleName, const std::string &data)
{
    std::lock_guard<std::mutex> lock(this->access);
    auto search = this->modules.find(moduleName);
    if (search != this->modules.end()) {
        if (search->second.getStatus() == false) {
            return "this module is inactive NaM";
        }
        std::string userIDstr = this->userIDs.getID(user);
        if(userIDstr.empty()) {
            return "user " + user + " not found";
        }
        
        if (search->second.getType() == "string") {
            redisReply *reply = static_cast<redisReply *>(redisCommand(this->context,
                "HSET WNMA:modulesData:%b %b %b", userIDstr.c_str(), userIDstr.size(), moduleName.c_str(), moduleName.size(), data.c_str(), data.size()));
            freeReplyObject(reply);
            return "module " + moduleName + " data for user " + user + " was set to " + data;
        } else if (search->second.getType() == "double") {
            redisReply *reply = nullptr;
            try {
                double value = std::stod(data);
                std::stringstream ss;
                ss << value;
                std::string strVal = ss.str();
                reply = static_cast<redisReply *>(redisCommand(this->context,
                    "HSET WNMA:modulesData:%b %b %b", userIDstr.c_str(), userIDstr.size(), moduleName.c_str(), moduleName.size(), strVal.c_str(), strVal.size()));
                freeReplyObject(reply);
                reply = nullptr;
                return "module " + moduleName + " data for user " + user + " was set to " + strVal;
            } catch (const std::invalid_argument &ex) {
                if (reply != nullptr) {
                    freeReplyObject(reply);
                }
                return "invalid argument";
            } catch (const std::out_of_range &ex) {
                if (reply != nullptr) {
                    freeReplyObject(reply);
                }
                return "out of range";
            } catch (const std::exception &ex) {
                if (reply != nullptr) {
                    freeReplyObject(reply);
                }
                return ex.what();
            } catch (...) {
                if (reply != nullptr) {
                    freeReplyObject(reply);
                }
                return "unknown exception";
            }
        } else if (search->second.getType() == "int") {
            redisReply *reply = nullptr;
            try {
                int value = std::stoi(data);
                std::string strVal = std::to_string(value);
                reply = static_cast<redisReply *>(redisCommand(this->context,
                    "HSET WNMA:modulesData:%b %b %b", userIDstr.c_str(), userIDstr.size(), moduleName.c_str(), moduleName.size(), strVal.c_str(), strVal.size()));
                freeReplyObject(reply);
                reply = nullptr;
                return "module " + moduleName + " data for user " + user + " was set to " + strVal;
            } catch (const std::invalid_argument &ex) {
                if (reply != nullptr) {
                    freeReplyObject(reply);
                }
                return "invalid argument";
            } catch (const std::out_of_range &ex) {
                if (reply != nullptr) {
                    freeReplyObject(reply);
                }
                return "out of range";
            } catch (const std::exception &ex) {
                if (reply != nullptr) {
                    freeReplyObject(reply);
                }
                return ex.what();
            } catch (...) {
                if (reply != nullptr) {
                    freeReplyObject(reply);
                }
                return "unknown exception";
            }
        } else {
            return "unknown type";
        }
    }
    return "no such module found";
}

bool
ModulesManager::saveModule(const std::string &moduleName)
{
    std::lock_guard<std::mutex> lock(this->access);
    auto search = this->modules.find(moduleName);
    if (search != this->modules.end()) {
        const auto &name = search->second.getName();
        const auto &type = search->second.getType();
        const auto &subtype = search->second.getSubtype();
        const auto &format = search->second.getFormat();
        const auto &status = search->second.getStatus();
        
        pt::ptree tree;
        tree.put("name", name);
        tree.put("type", type);
        tree.put("subtype", subtype);
        tree.put("format", format);
        tree.put("status", status);
        
        std::stringstream ss;
        pt::write_json(ss, tree, false);
        std::string moduleInfo = ss.str();
        
        redisReply *reply = static_cast<redisReply *>(redisCommand(this->context,
            "HSET WNMA:modules %b %b", moduleName.c_str(), moduleName.size(), moduleInfo.c_str(), moduleInfo.size()));
        freeReplyObject(reply);
        return true;
    }
    return false;
}

void
ModulesManager::deleteModule(const std::string &moduleName)
{
    std::lock_guard<std::mutex> lock(this->access);
    auto search = this->modules.find(moduleName);
    if (search != this->modules.end()) {
        redisReply *reply = static_cast<redisReply *>(redisCommand(this->context,
            "HDEL WNMA:modules %b", moduleName.c_str(), moduleName.size()));
        freeReplyObject(reply);
        this->modules.erase(search);
    }
}

void
ModulesManager::loadAllModules()
{
    std::lock_guard<std::mutex> lock(this->access);
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGETALL WNMA:modules"));
    if (reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return;
    }
    
    for (decltype(reply->elements) i = 0; i < reply->elements; i += 2) {
        pt::ptree tree;
        std::string moduleName(reply->element[i]->str, reply->element[i]->len);
        std::string module(reply->element[i + 1]->str, reply->element[i + 1]->len);
        std::stringstream ss(module);
        pt::read_json(ss, tree);
        
        std::string name = tree.get("name", "");
        std::string type = tree.get("type", "");
        std::string subtype = tree.get("subtype", "");
        std::string format = tree.get("format", "");
        bool status = tree.get("status", false);
        
        if(name == "") {
            continue;
        }
        
        this->modules.emplace(std::make_pair(moduleName, Module(name, type, subtype, format, status)));
    }
    freeReplyObject(reply);
}

std::string
ModulesManager::getAllModules()
{
    std::string ret;
    std::string active;
    std::string inactive;
    std::lock_guard<std::mutex> lock(this->access);
    for (const auto &i : this->modules) {
        if (i.second.getStatus() == true) {
            active += i.first + ", ";
        } else {
            inactive += i.first + ", ";
        }
    }
    if (!active.empty()) {
        active.pop_back();
        active.pop_back();
    }
    if (!inactive.empty()) {
        inactive.pop_back();
        inactive.pop_back();
    }
    if (!active.empty()) {
        ret += "active modules: " + active;
    }
    if (!inactive.empty()) {
        ret += ", inactive modules: " + inactive;
    }
    
    return ret;
}