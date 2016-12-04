#ifndef MESSAGEHANDLER_HPP
#define MESSAGEHANDLER_HPP

#include "ircmessage.hpp"

class MessageHandler
{
public:
    virtual bool handleMessage(const IRCMessage &message) = 0;
};

#endif  // MESSAGEHANDLER_HPP
