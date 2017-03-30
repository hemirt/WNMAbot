#include "modules.hpp"
#include <iostream>

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
    if (newStatus && this->type.size() != 0 && this->subtype.size() != 0 && this->name.size() != 0 && this->format.size() != 0) {
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

std::string
ModulesManager::getData(const std::string &user, const std::string &moduleName)
{
    std::lock_guard<std::mutex> lock(this->access);
    auto search = this->modules.find(moduleName);
    if (search != this->modules.end()) {
        return "here be data NaM";
    }
    return "no such module found";
}