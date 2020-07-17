#ifndef USAGE_HPP
#define USAGE_HPP

#include <hiredis/hiredis.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <mutex>

class Usage
{
public:
    Usage();
    ~Usage();
    void IncrUser(const std::string& user, const std::string& channel);
    void IncrChannel(const std::string& channel);
    void IncrUserCommand(const std::string& user, const std::string& channel, const std::string& command);
    void IncrChannelCommand(const std::string& channel, const std::string& command);
    void IncrChannelUser(const std::string& channel, const std::string& user);
    void IncrAll(const std::string& user, const std::string& channel, const std::string& command);

private:
    redisContext *context;
    void reconnect();
    std::mutex mtx;
    
    void doIncrUser(const std::string& user, const std::string& channel);
    void doIncrChannel(const std::string& channel);
    void doIncrUserCommand(const std::string& user, const std::string& channel, const std::string& command);
    void doIncrChannelCommand(const std::string& channel, const std::string& command);
    void doIncrChannelUser(const std::string& channel, const std::string& user);
    void doIncrChannelCommandUser(const std::string& channel, const std::string& command, const std::string& user);
    
    int attempt = 0;
};

#endif