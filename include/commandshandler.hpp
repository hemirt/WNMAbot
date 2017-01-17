#ifndef COMMANDSHANDLER_HPP
#define COMMANDSHANDLER_HPP

#include "ircmessage.hpp"
#include "network.hpp"
#include "redisclient.hpp"

#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>

class Channel;

class Response
{
public:
    enum class Type {
        UNKNOWN,
        MESSAGE,
        FUNCTION,

        NUM_TYPES,
    } type = Type::UNKNOWN;

    Response() = default;

    std::string message;

private:
};

class CommandsHandler
{
public:
    CommandsHandler(boost::asio::io_service &_ioService,
                    Channel *_channelObject);
    ~CommandsHandler();

    Response handle(const IRCMessage &message);

    Channel *channelObject;

private:
    RedisClient redisClient;

    bool isAdmin(const std::string &user);
    Response addCommand(const IRCMessage &message,
                        std::vector<std::string> &tokens);
    Response editCommand(const IRCMessage &message,
                         std::vector<std::string> &tokens);
    Response deleteCommand(const IRCMessage &message,
                           std::vector<std::string> &tokens);
    Response rawEditCommand(const IRCMessage &message,
                            std::vector<std::string> &tokens);
    Response deleteFullCommand(const IRCMessage &message,
                               std::vector<std::string> &tokens);
    Response makeResponse(const IRCMessage &message,
                          std::string &responseString,
                          std::vector<std::string> &tokens,
                          const boost::property_tree::ptree &commandTree,
                          const std::string &path);
    Response joinChannel(const IRCMessage &message,
                         std::vector<std::string> &tokens);
    Response leaveChannel(const IRCMessage &message,
                          std::vector<std::string> &tokens);
    Response printChannels(const IRCMessage &message,
                           std::vector<std::string> &tokens);

    boost::asio::io_service &ioService;

    std::unordered_map<std::string, std::chrono::steady_clock::time_point>
        cooldownsMap;
};

#endif