#ifndef COMMANDSHANDLER_HPP
#define COMMANDSHANDLER_HPP

#include "encryption.hpp"
#include "ircmessage.hpp"
#include "network.hpp"
#include "redisclient.hpp"

#include "countries.hpp"
#include "userids.hpp"

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
        WHISPER,
        FUNCTION,

        NUM_TYPES,
    } type = Type::UNKNOWN;

    Response() = default;
    Response(int _priority)
        : priority(_priority){};
    // -1 dont send msg if full, 0 put to back, 1 put to front
    int priority = -1;
    std::string message;
    std::string whisperReceiver;

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

    RedisClient redisClient;

    bool isAdmin(const std::string &user);

    Encryption crypto;

private:
    using cmdFunction = Response (CommandsHandler::*) (const IRCMessage &, std::vector<std::string> &);
    static std::unordered_map<std::string, cmdFunction> adminCommands;
    static std::unordered_map<std::string, cmdFunction> normalCommands;
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
    Response remindMe(const IRCMessage &message,
                      std::vector<std::string> &tokens);
    Response remind(const IRCMessage &message,
                    std::vector<std::string> &tokens);
    Response say(const IRCMessage &message, std::vector<std::string> &tokens);
    Response afk(const IRCMessage &message, std::vector<std::string> &tokens);
    Response goodNight(const IRCMessage &message,
                       std::vector<std::string> &tokens);
    Response isAfk(const IRCMessage &message, std::vector<std::string> &tokens);
    Response comeBackMsg(const IRCMessage &message,
                         std::vector<std::string> &tokens);
    Response addBlacklist(const IRCMessage &message,
                          std::vector<std::string> &tokens);
    Response removeBlacklist(const IRCMessage &message,
                             std::vector<std::string> &tokens);
    Response whoIsAfk(const IRCMessage &message, std::vector<std::string> &tokens);
    Response regexTest(const IRCMessage &message,
                       std::vector<std::string> &tokens);
    Response setUserCountryFrom(const IRCMessage &message,
                                std::vector<std::string> &tokens);
    Response setUserCountryLive(const IRCMessage &message,
                                std::vector<std::string> &tokens);
    Response deleteUser(const IRCMessage &message,
                        std::vector<std::string> &tokens);
    Response isFrom(const IRCMessage &message,
                    std::vector<std::string> &tokens);
    Response getUsersFrom(const IRCMessage &message,
                          std::vector<std::string> &tokens);
    Response getUsersLiving(const IRCMessage &message,
                            std::vector<std::string> &tokens);
    Response myFrom(const IRCMessage &message,
                    std::vector<std::string> &tokens);
    Response myLiving(const IRCMessage &message,
                      std::vector<std::string> &tokens);
    Response myDelete(const IRCMessage &message,
                      std::vector<std::string> &tokens);
    Response createCountry(const IRCMessage &message,
                           std::vector<std::string> &tokens);
    Response deleteCountry(const IRCMessage &message,
                           std::vector<std::string> &tokens);
    Response renameCountry(const IRCMessage &message,
                           std::vector<std::string> &tokens);
    Response getCountryID(const IRCMessage &message,
                          std::vector<std::string> &tokens);
    Response createAlias(const IRCMessage &message,
                         std::vector<std::string> &tokens);
    Response deleteAlias(const IRCMessage &message,
                         std::vector<std::string> &tokens);
    Response myReminders(const IRCMessage &message,
                         std::vector<std::string> &tokens);
    Response checkReminder(const IRCMessage &message,
                           std::vector<std::string> &tokens);
    Response pingMeCommand(const IRCMessage &message,
                           std::vector<std::string> &tokens);
    Response randomIslamicQuote(const IRCMessage &message,
                                std::vector<std::string> &tokens);
    Response randomChristianQuote(const IRCMessage &message,
                                  std::vector<std::string> &tokens);
    Response encrypt(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response decrypt(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response createModule(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response setModuleType(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response setModuleSubtype(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response setModuleName(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response setModuleFormat(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response setModuleStatus(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response getModuleInfo(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response getUserData(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response getRandomQuote(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response ignore(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response unignore(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response saveModule(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response setData(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response setDataAdmin(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response modules(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response deleteModule(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response deleteMyData(const IRCMessage &message,
                     std::vector<std::string> &tokens);
    Response deleteUserData(const IRCMessage &message,
                     std::vector<std::string> &tokens);
                     
    boost::asio::io_service &ioService;

    std::unordered_map<std::string, std::chrono::steady_clock::time_point>
        cooldownsMap;
    std::mutex cooldownsMtx;
    
    bool cooldownCheck(const std::string &user, const std::string &cmd);
};

#endif