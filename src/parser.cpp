#include "parser.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <vector>
#include <iostream>

namespace Parser {

/*
@badges=<badges>;bits=<bits>;color=<color>;display-name=<display-name>
;emotes=<emotes>;id=<id>;mod=<mod>;room-id=<room-id>;subscriber=<subscriber>
;turbo=<turbo>;user-id=<user-id>;user-type=<user-type>
 :<user>!<user>@<user>.tmi.twitch.tv PRIVMSG #<channel> :<message>
*/

/*
@badges=moderator/1;color=#DAA520;display-name=hemirt
;emotes=;id=13391e02-a511-43fa-a812-26401e114510;mod=1;room-id=133122648;subscriber=0
;tmi-sent-ts=1492905044421;turbo=0;user-id=29628676;user-type=mod
 :hemirt!hemirt@hemirt.tmi.twitch.tv PRIVMSG #weneedmoreautisticbots :\join pajlada
*/

IRCMessage
parseMessage(const std::string &rawMessage)
{
    IRCMessage ircMessage;
    if (rawMessage.size() >= 1 && rawMessage.at(0) == '@')
    {
        // remove \r\n
        ircMessage.raw = rawMessage.substr(1, rawMessage.size() - 3);
    } else {
        // remove \r\n
        ircMessage.raw = rawMessage.substr(0, rawMessage.size() -2);
    }
    if (rawMessage.compare(0, 4, "PING") == 0) {
        ircMessage.type = IRCMessage::Type::PING;
        return ircMessage;
    }
    
    {
        size_t endtags = ircMessage.raw.find(" :", 0);
        if (endtags == std::string::npos) {
            return ircMessage;
        }
        ircMessage.tags = ircMessage.raw.substr(0, endtags);
        
        size_t endmiddle = ircMessage.raw.find(" :", endtags + 1);
        if (endmiddle == std::string::npos) {
            ircMessage.middle = ircMessage.raw.substr(endtags + 2);
        } else {
            ircMessage.middle = ircMessage.raw.substr(endtags + 2, endmiddle - endtags - 2);
            ircMessage.message = ircMessage.raw.substr(endmiddle + 2, std::string::npos);
        }
    }
    
    {
        size_t exclamation = ircMessage.middle.find('!');
        if (exclamation != std::string::npos) {
            ircMessage.user = ircMessage.middle.substr(0, exclamation);
        }
    }
    
    {
        size_t space = ircMessage.middle.find(' ');
        if (space != std::string::npos) {
            space += 1;
            size_t endspace = ircMessage.middle.find(' ', space);
            if (endspace != std::string::npos) {
                auto type = ircMessage.middle.substr(space, endspace - space);
                ircMessage.channel = ircMessage.middle.substr(endspace + 2);
                if (type == "PRIVMSG") {
                    ircMessage.type = IRCMessage::Type::PRIVMSG;
                } else if (type == "USERSTATE") {
                    ircMessage.type = IRCMessage::Type::USERSTATE;
                    return ircMessage;
                } else if (type == "ROOMSTATE") {
                    ircMessage.type = IRCMessage::Type::ROOMSTATE;
                    return ircMessage;
                } else if (type == "CLEARCHAT") {
                    ircMessage.type = IRCMessage::Type::CLEARCHAT;
                    return ircMessage;
                } else {
                    std::cerr << "unknown message type: " << type << "\nmessage: " << ircMessage.raw << std::endl;
                    return ircMessage;
                }
            }
        }
    }

    std::vector<std::string> tags;
    boost::algorithm::split(tags, ircMessage.tags,
                            boost::algorithm::is_any_of(";"), boost::algorithm::token_compress_on);

    for (const auto &fulltag : tags) {
        std::vector<std::string> tag;
        
        const auto equal_index = fulltag.find_first_of('=');
        if (equal_index == std::string::npos) {
            continue;
        } else {
            tag.push_back(fulltag.substr(0, equal_index));
            tag.push_back(fulltag.substr(equal_index + 1));
        }
        
        if (tag.size() < 2) {
            continue;
        }
        
        boost::algorithm::replace_all(tag[1], "\\:", ";");
        boost::algorithm::replace_all(tag[1], "\\s", " ");
        boost::algorithm::replace_all(tag[1], "\\\\", "\\");
        
        if (tag[0] == "badges") {   
            ircMessage.badges = tag[1];
        } else if (tag[0] == "bits") {
            ircMessage.bits = tag[1];
        } else if (tag[0] == "color") {
            ircMessage.color = tag[1];
        } else if (tag[0] == "display-name") {
            ircMessage.displayname = tag[1];
        } else if (tag[0] == "emotes") {
            ircMessage.rawemotes = tag[1];
        } else if (tag[0] == "user-type") {
            ircMessage.userType = tag[1];
        } else if (tag[0] == "user-id") {
            ircMessage.userid = tag[1];
        } else if (tag[0] == "mod") {
            if (tag[1] == "1") {
                ircMessage.mod = true;
            } else {
                ircMessage.mod = false;
            }
        } else if (tag[0] == "subscriber") {
            if (tag[1] == "1") {
                ircMessage.subscriber = true;
            } else {
                ircMessage.subscriber = false;
            }
        } else if (tag[0] == "turbo") {
            if (tag[1] == "1") {
                ircMessage.turbo = true;
            } else {
                ircMessage.turbo = false;
            }
        }
    }

    return ircMessage;
}

} // namespace Parser
