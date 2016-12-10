#include "commandshandler.hpp"

CommandsHandler::CommandsHandler(const IRCMessage &message)
{
    if(message.user == "hemirt") {
        this->response = "I'm responding to hemirt's message FeelsGoodMan";
        this->isValid = true;
    }
}