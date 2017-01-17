#ifndef REDISCLIENT_HPP
#define REDISCLIENT_HPP

#include <hiredis/hiredis.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <boost/property_tree/ptree.hpp>

class RedisClient
{
public:
    RedisClient();
    ~RedisClient();

    void reconnect();

    boost::property_tree::ptree getCommandTree(const std::string &trigger);

    void setCommandTree(const std::string &trigger, const std::string &json);

    void deleteFullCommand(const std::string &trigger);

    bool addReminder(int timestamp, const std::string &user, int seconds,
                     const std::string &reminder = "");

    bool isAdmin(const std::string &user);

private:
    redisContext *context;
    void deleteRedisKey(const std::string &key);
};

#endif