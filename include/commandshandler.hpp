#ifndef COMMANDSHANDLER_HPP
#define COMMANDSHANDLER_HPP

#include "ircmessage.hpp"
#include "redisclient.hpp"

#include <iostream>
#include <string>

class Response
{
public:
    Response() = default;

    bool valid = false;

    std::string message;

private:
};

class CommandsHandler
{
public:
    CommandsHandler();
    ~CommandsHandler();

    Response handle(const IRCMessage &message);

private:
    bool isAdmin(const std::string &user);
    RedisClient redisClient;
    Response addCommand(const IRCMessage &message,
                        std::vector<std::string> &tokens);
    Response editCommand(const IRCMessage &message,
                         std::vector<std::string> &tokens);
    Response rawEditCommand(const IRCMessage &message,
                            std::vector<std::string> &tokens);
    Response deleteCommand();
};

#endif