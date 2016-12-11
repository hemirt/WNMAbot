#ifndef COMMANDSHANDLER_HPP
#define COMMANDSHANDLER_HPP

#include "ircmessage.hpp"

#include <hiredis/hiredis.h>

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
    void reconnect();
    void isAlive();
    bool isAdmin(const std::string &user);
    redisContext *redisC;
    Response addCommand();
    Response editCommand();
    Response deleteCommand();
};

#endif