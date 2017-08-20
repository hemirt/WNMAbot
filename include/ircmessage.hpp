#ifndef IRCMESSAGE_HPP
#define IRCMESSAGE_HPP

#include <algorithm>
#include <string>
#include <iostream>

class IRCMessage
{
public: 
    enum class Type {
        UNKNOWN,
        PRIVMSG,
        PING,
        PONG,
        USERSTATE,
        ROOMSTATE,
        CLEARCHAT,
        
        NUM_TYPES,
    } type = Type::UNKNOWN;

    IRCMessage() = default;

    std::string tags;
    std::string middle;
    std::string message;
    
    std::string user;
    std::string channel;
    
    std::string raw;
    std::string badges;
    std::string bits;
    std::string color;
    std::string displayname;
    std::string rawemotes;
    bool mod = false;
    bool subscriber = false;
    bool turbo = false;
    std::string userid;
    std::string userType;
    
    friend std::ostream& operator<< (std::ostream& stream, const IRCMessage& ircMessage) {
        stream << "raw: " << ircMessage.raw
               << "\ntags: " << ircMessage.tags
               << "\nmiddle: " << ircMessage.middle
               << "\nmessage: " << ircMessage.message
               << "\nusername: " << ircMessage.user
               << "\nbadges: " << ircMessage.badges
               << "\nbits: " << ircMessage.bits
               << "\ncolor: " << ircMessage.color
               << "\ndisplayname: " << ircMessage.displayname
               << "\nrawem: " << ircMessage.rawemotes
               << "\nmod: " << ircMessage.mod
               << "\nsub: " << ircMessage.subscriber
               << "\ntrbo: " << ircMessage.turbo
               << "\nuserid: " << ircMessage.userid
               << "\nuserty: " << ircMessage.userType;
        return stream;
    }
    
    
};

#endif  // IRCMESSAGE_HPP
