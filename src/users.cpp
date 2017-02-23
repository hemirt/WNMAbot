#include "users.hpp"
#include <algorithm>
#include "utilities.hpp"
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

namespace pt = boost::property_tree;

Users::Users()
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "Users error: " << this->context->errstr << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "Users can't allocate redis context" << std::endl;
        }
    }

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGETALL WNMA:usersplaces"));
    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return;
    } else if (reply->type == REDIS_REPLY_ARRAY) {
        for (int i = 0; i < reply->elements; i += 2) {
            std::string user(reply->element[i]->str, reply->element[i]->len);

            std::string jsonString(reply->element[i + 1]->str,
                                reply->element[i + 1]->len);
            pt::ptree tree;
            std::stringstream ss(jsonString);
            pt::read_json(ss, tree);
            
            std::string from = tree.get<std::string>("from", "");
            std::string living = tree.get<std::string>("living", "");
            if(from.empty() && living.empty()) {
                continue;
            }
            this->users.emplace(std::make_pair(user, Account{from, living}));
        }
    }
    freeReplyObject(reply);
}

Users::~Users()
{
    if (!this->context)
        return;
    redisFree(this->context);
}

void
Users::setUser(std::string user, std::string &country)
{
    changeToLower(country);
    changeToLower(user);
    std::unique_lock<std::shared_mutex> lock(containersMtx);
    auto search = this->users.find(user);
    if (search != this->users.end()) {
        std::string oldCountry = search->second.from;
        std::string oldLiving = search->second.living;
        Account acc{country, oldLiving};
        users[user] = acc;
        
    }
    auto search2 = this->users.emplace(std::make_pair(user, Account{country, ""}));
    
    pt::ptree tree;
    tree.put("from", search2.first->second.from);
    tree.put("living", search2.first->second.living);
    
    std::stringstream ss;
    pt::write_json(ss, tree, false);
    std::string json = ss.str();

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HSET WNMA:usersplaces %b %b", user.c_str(),
                     user.size(), json.c_str(), json.size()));
    freeReplyObject(reply);
}

void
Users::setUserLiving(std::string user, std::string &living)
{
    changeToLower(living);
    changeToLower(user);
    std::unique_lock<std::shared_mutex> lock(containersMtx);
    auto search = this->users.find(user);
    if (search != this->users.end()) {
        std::string oldCountry = search->second.from;
        std::string oldLiving = search->second.living;
        Account acc{oldCountry, living};
        users[user] = acc;
    }
    auto search2 = this->users.emplace(std::make_pair(user, Account{"", living}));
    
    pt::ptree tree;
    tree.put("from", search2.first->second.from);
    tree.put("living", search2.first->second.living);
    
    std::stringstream ss;
    pt::write_json(ss, tree, false);
    std::string json = ss.str();

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HSET WNMA:usersplaces %b %b", user.c_str(),
                     user.size(), json.c_str(), json.size()));
    freeReplyObject(reply);
}


void
Users::deleteUser(std::string user)
{
    changeToLower(user);
    std::unique_lock<std::shared_mutex> lock(containersMtx);
    this->users.erase(user);
    
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HDEL WNMA:usersplaces %b", user.c_str(),
                     user.size()));
    freeReplyObject(reply);
}

std::string
Users::getUsersCountry(std::string user)
{
    changeToLower(user);
    std::shared_lock<std::shared_mutex> lock(containersMtx);
    auto it = users.find(user);
    if (it != users.end()) {
        if (it->second.from.empty() && !it->second.living.empty()) {
            return std::string(user + " lives in " + it->second.living);
        } else if (!it->second.from.empty() && it->second.living.empty()) {
            return std::string(user + " is from " + it->second.from);
        } else if (!it->second.from.empty() && !it->second.living.empty()) {
            return std::string(user + " is from " + it->second.from + " and lives in " + it->second.living);
        }
    }
    return std::string(user + " not found");
}

std::string
Users::getUsersFrom(std::string country)
{
    changeToLower(country);
    std::string usersString;
    std::shared_lock<std::shared_mutex> lock(containersMtx);
    for (const auto &i : users) {
        if (i.second.from == country) {
            usersString += i.first[0] + std::string("\u05C4") + i.first.substr(1, std::string::npos) + ", ";         
        }
    }

    if (usersString.back() == ' ') {
        usersString.pop_back();
        usersString.pop_back();
    }

    return usersString;
}

std::string
Users::getUsersLiving(std::string country)
{
    changeToLower(country);
    std::string usersString;
    std::shared_lock<std::shared_mutex> lock(containersMtx);
    for (const auto &i : users) {
        if (i.second.living == country || (i.second.living.empty() && i.second.from == country)) {
            usersString += i.first[0] + std::string("\u05C4") + i.first.substr(1, std::string::npos) + ", ";
        }
    }

    if (usersString.back() == ' ') {
        usersString.pop_back();
        usersString.pop_back();
    }

    return usersString;
}

void
Users::printAllCout()
{
    std::shared_lock<std::shared_mutex> lock(containersMtx);
    std::cout << "User Country" << std::endl;
    for (const auto &i : users) {
        std::cout << i.first << " " << i.second.from << " : " << i.second.living << std::endl;
    }
}
