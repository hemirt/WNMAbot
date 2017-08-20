#include "pingme.hpp"
#include <iostream>

redisContext *PingMe::context = nullptr;
std::map<std::string, std::map<std::string, std::string>> PingMe::pingMap;
std::mutex PingMe::access;
PingMe PingMe::instance;

PingMe::PingMe()
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
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SMEMBERS WNMA:pingers"));
    if (reply && reply->type == REDIS_REPLY_ARRAY) {
        for (decltype(reply->elements) i = 0; i < reply->elements; i++) {
            std::string user(reply->element[i]->str, reply->element[i]->len);

            redisReply *reply2 = static_cast<redisReply *>(
                redisCommand(this->context, "HGETALL WNMA:ping:%b",
                             user.c_str(), user.size()));
            std::map<std::string, std::string> pingedUsers;
            if (reply2 && reply2->type == REDIS_REPLY_ARRAY) {
                for (decltype(reply2->elements) j = 0; j < reply2->elements; j += 2) {
                    pingedUsers.insert(std::make_pair(
                        std::string(reply2->element[j]->str,
                                    reply2->element[j]->len),
                        std::string(reply2->element[j + 1]->str,
                                    reply2->element[j + 1]->len)));
                }
            }
            freeReplyObject(reply2);
            this->pingMap.insert(
                std::make_pair(std::move(user), std::move(pingedUsers)));
        }
    }
    freeReplyObject(reply);
}

PingMe::~PingMe()
{
    if (!this->context)
        return;
    redisFree(this->context);
}

// first == channel
// second == users who are to be pinged
std::map<std::string, std::vector<std::string>>
PingMe::ping(const std::string &bywhom)
{
    std::lock_guard<std::mutex> lk(this->access);
    std::map<std::string, std::vector<std::string>> map;
    auto search = this->pingMap.find(bywhom);
    if (search == this->pingMap.end()) {
        return map;
    }

    for (const auto &i : search->second) {
        // i.first == name of the user to be pinged
        // i.second == channel
        map[i.second].push_back(i.first);
    }

    
    
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SREM WNMA:pingers %b",
                     search->first.c_str(), search->first.size()));
    freeReplyObject(reply);
    reply = static_cast<redisReply *>(
        redisCommand(this->context, "DEL WNMA:ping:%b", search->first.c_str(),
                     search->first.size()));
    freeReplyObject(reply);
    
    this->pingMap.erase(search);
    return map;
}

bool
PingMe::isPinger(const std::string &name)
{
    std::lock_guard<std::mutex> lk(this->access);
    return this->pingMap.count(name) == 1;
}

void
PingMe::addPing(const std::string &who, const std::string &bywhom,
                const std::string &channel)
{
    std::lock_guard<std::mutex> lk(this->access);

    this->pingMap[bywhom][who] = channel;

    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SADD WNMA:pingers %b", bywhom.c_str(), bywhom.size()));
    freeReplyObject(reply);
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "HSET WNMA:ping:%b %b %b", bywhom.c_str(), bywhom.size(),
        who.c_str(), who.size(), channel.c_str(), channel.size()));
    freeReplyObject(reply);
}