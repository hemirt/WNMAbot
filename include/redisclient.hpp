#ifndef REDISCLIENT_HPP
#define REDISCLIENT_HPP

#include <hiredis/hiredis.h>
#include <string>
#include <iostream>
#include <unordered_map>


class RedisClient
{
public:
    RedisClient();
    ~RedisClient();
    
    void reconnect();
    
    std::unordered_map<std::string, std::string> getCommand(const std::string &channel, const std::string &user, const std::string &command);
    
    void addCommand(const std::string &channel, const std::string &user, const std::string &command, const std::string &rest);
    
    bool isValid() {
        return this->valid;
    }
    
private:
    redisContext *context;
    bool valid;
    
};

#endif