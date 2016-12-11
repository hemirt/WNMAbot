#include "parser.hpp"

#include <iostream>

namespace Parser {

IRCMessage
parseMessage(const std::string &rawMessage)
{
    IRCMessage message;
    MiddleMessage mm;

    auto it = rawMessage.begin();
    auto rawEnd = rawMessage.end();

    if (*it == '@') {
        // Parse tags
    }

    if (*it == ':') {
        // Parse prefix
        ++it;

        auto end = std::find(it, rawEnd, ' ');
        if (end == rawEnd) {
            // ERROR
            return IRCMessage();
        }

        mm.prefix = std::string(it, end);
        it = end + 1;
    }

    if (it == rawEnd) {
        // ERROR
        return IRCMessage();
    }

    // Parse command
    {
        auto end = std::find(it, rawEnd, ' ');
        if (end == rawEnd) {
            end = rawEnd - 1;

            if (end == it) {
                // ERROR
                return IRCMessage();
            }
        }

        mm.command = std::string(it, end);
        it = end + 1;
    }

    if (it != rawEnd) {
        // Parse params
        if (*it != ':') {
            auto end = std::find(it, rawEnd, ' ');

            if (end == rawEnd) {
                end = rawEnd - 1;

                if (end == it) {
                    // ERROR
                    return IRCMessage();
                }
            }

            mm.middle = std::string(it, end);
            it = end + 1;
        }

        if (it != rawEnd) {
            if (*it == ':') {
                mm.params = std::string(it + 1, rawEnd);
            }
        }
    }

    /*
    std::cout << "Tags: " << mm.tags << std::endl;
    std::cout << "Prefix: " << mm.prefix << std::endl;
    std::cout << "Command: " << mm.command << std::endl;
    std::cout << "Middle: " << mm.middle << std::endl;
    std::cout << "Params: " << mm.params << std::endl;
    std::cout << "Command: " << mm.middle << std::endl;
    */

    if (mm.params.back() == '\n') {
        mm.params.pop_back();
    }

    if (mm.params.back() == '\r') {
        mm.params.pop_back();
    }

    return IRCMessage(mm);
}

}  // namespace Parser
