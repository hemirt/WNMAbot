#ifndef PARSER_HPP
#define PARSER_HPP

#include "ircmessage.hpp"

#include <string>

namespace Parser {

IRCMessage parseMessage(const std::string &rawMessage);

} // namespace Parser

#endif // PARSER_HPP
