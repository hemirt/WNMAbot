#ifndef COMMANDSHANDLER_HPP
#define COMMANDSHANDLER_HPP

#include "ircmessage.hpp"

#include <string>

class Response
{
public:
    Response::Response();
    
    bool isValid() const {
        return this->valid;
    }
    
    std::string message;
    
private:
    bool valid = false;
};

class CommandsHandler
{
public:
    CommandsHandler();
    
    Response handle(const IRCMessage &message)

};

#endif