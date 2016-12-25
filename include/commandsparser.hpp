#ifndef COMMANDSPARSER_HPP
#define COMMANDSPARSER_HPP

#include "ircmessage.hpp"

#include <vector>

class CommandsParser
{
public:
    CommandsParser() = delete;
    CommandsParser(const IRCMessage &message);
    
    std::string trigger;
    std::vector<std::string> parameters;
}


#endif