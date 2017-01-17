#ifndef REDISAUTH_HPP
#define REDISAUTH_HPP

#include <hiredis/hiredis.h>

#include <iostream>
#include <string>
#include <vector>

class RedisAuth
{
public:
    RedisAuth();
    ~RedisAuth();
    std::string
    getOauth()
    {
        return oauth;
    }
    std::string
    getName()
    {
        return name;
    }
    void setOauth(const std::string &oauth);
    void setName(const std::string &name);
    bool
    isValid()
    {
        return valid;
    }
    bool
    hasAuth()
    {
        return auth;
    }

private:
    bool valid;
    bool auth;
    std::string oauth;
    std::string name;
    redisContext *context;
};

#endif