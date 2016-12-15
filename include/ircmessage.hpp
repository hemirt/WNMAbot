#ifndef IRCMESSAGE_HPP
#define IRCMESSAGE_HPP

#include <algorithm>
#include <string>

struct MiddleMessage {
    std::string tags;
    std::string prefix;
    std::string command;
    std::string middle;
    std::string params;
};

class IRCMessage
{
public:
    enum class Type {
        UNKNOWN,
        PRIVMSG,
        PING,
        PONG,

        // TODO: add remaining types,
        NUM_TYPES,
    } type = Type::UNKNOWN;

    IRCMessage()
    {
    }

    IRCMessage(const MiddleMessage &mm)
    {
        if (mm.prefix.length() > 0) {
            auto at = std::find(mm.prefix.begin(), mm.prefix.end(), '@');
            auto dot = std::find(mm.prefix.begin(), mm.prefix.end(), '.');

            if (at == mm.prefix.end() && dot != mm.prefix.end()) {
                this->server = mm.prefix;
            } else {
                if (at == mm.prefix.end()) {
                    this->nickname = mm.prefix;
                } else {
                    auto exclamation =
                        std::find(mm.prefix.begin(), mm.prefix.end(), '!');

                    if (exclamation == mm.prefix.end()) {
                        this->nickname = std::string(mm.prefix.begin(), at);
                        this->host = std::string(at + 1, mm.prefix.end());
                    } else if (exclamation < at) {
                        this->nickname =
                            std::string(mm.prefix.begin(), exclamation);
                        this->user = std::string(exclamation + 1, at);
                        this->host = std::string(at + 1, mm.prefix.end());
                    }
                }
            }
        }

        if (mm.command == "PRIVMSG") {
            this->type = Type::PRIVMSG;
            // middle "#channelname"
            // channel:
            this->channel = mm.middle.substr(1, std::string::npos);
        } else if (mm.command == "PING") {
            this->type = Type::PING;
        } else if (mm.command == "PONG") {
            this->type = Type::PONG;
        }

        this->middle = mm.middle;
        this->params = mm.params;
    }

    std::string server;
    std::string nickname;
    std::string user;
    std::string middle;
    std::string params;
    std::string host;
    std::string channel;
};

#endif  // IRCMESSAGE_HPP
