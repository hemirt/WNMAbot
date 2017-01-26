#ifndef REDISAUTH_HPP
#define REDISAUTH_HPP

#include <hiredis/hiredis.h>

#include <iostream>
#include <string>
#include <vector>
#include "stdint.h"
#include <map>

struct Reminder
{
    std::string id;
    int64_t timeStamp;
    std::string message;
};

class RedisAuth
{
public:
    RedisAuth();
    ~RedisAuth();
    std::string getOauth();
    std::string getName();
    void setOauth(const std::string &oauth);
    void setName(const std::string &name);
    bool isValid();
    bool hasAuth();
    std::map<std::string, std::vector<Reminder>> getAllReminders();

private:
    bool valid;
    bool auth;
    std::string oauth;
    std::string name;
    redisContext *context;
};

#endif