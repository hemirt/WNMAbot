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
    boost::property_tree::ptree getRemindersOfUser(const std::string &user);

    void setCommandTree(const std::string &trigger, const std::string &json);

    void deleteFullCommand(const std::string &trigger);

    std::string addReminder(const std::string &user, int seconds,
                            const std::string &reminder);
    void setReminder(const std::string &user, const std::string &json);

    bool isAdmin(const std::string &user);

private:
    redisContext *context;
    void deleteRedisKey(const std::string &key);
};

#endif