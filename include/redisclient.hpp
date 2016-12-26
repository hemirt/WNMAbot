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

    void addCommand(const std::string &trigger, const std::string &json);

private:
    redisContext *context;
};

#endif