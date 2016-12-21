#ifndef REDISCLIENT_HPP
#define REDISCLIENT_HPP

#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <map>

class RedisClient
{
public:
    RedisClient();
    ~RedisClient();

    void reconnect();

    std::map<std::string, std::string> getCommand(
        const std::string &channel, const std::string &user,
        const std::string &command);

    void addCommand(const std::string &channel, const std::string &user,
                    const std::string &command, const std::string &rest);

    bool
    isValid()
    {
        return this->valid;
    }

private:
    redisContext *context;
    bool valid;
};

#endif