#ifndef COMMANDSHANDLER_HPP
#define COMMANDSHANDLER_HPP

#include "ircmessage.hpp"
#include "redisclient.hpp"
#include "network.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <chrono>



class Response
{
public:
    enum class Type {
        UNKNOWN,
        MESSAGE,
        FUNCTION,
        
        NUM_TYPES,
    } type = Type::UNKNOWN;
    
    Response() = default;

    std::string message;

private:
};

class CommandsHandler
{
public:
    CommandsHandler(boost::asio::io_service &_ioService);
    ~CommandsHandler();

    Response handle(const IRCMessage &message);

private:
    RedisClient redisClient;
    
    bool isAdmin(const std::string &user);
    Response addCommand(const IRCMessage &message,
                        std::vector<std::string> &tokens);
    Response editCommand(const IRCMessage &message,
                         std::vector<std::string> &tokens);
    Response deleteCommand(const IRCMessage &message,
                           std::vector<std::string> &tokens);
    Response rawEditCommand(const IRCMessage &message,
                            std::vector<std::string> &tokens);
    Response deleteFullCommand(const IRCMessage &message,
                            std::vector<std::string> &tokens);
    
    boost::asio::io_service &ioService;
    
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> cooldownsMap;
};

#endif