#ifndef COMMANDSHANDLER_HPP
#define COMMANDSHANDLER_HPP

#include "ircmessage.hpp"

#include <string>

class CommandsHandler
{
public:
    CommandsHandler(const IRCMessage &message);
    
    explicit operator bool() const {
        return this->isValid;
    }
    
    std::string response;
    
private:
    bool isValid = false;
};

#endif