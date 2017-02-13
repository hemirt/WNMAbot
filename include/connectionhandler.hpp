#ifndef CONNHANDLER_HPP
#define CONNHANDLER_HPP

#include "afkers.hpp"
#include "comebackmsg.hpp"
#include "network.hpp"
#include "redisauth.hpp"
#include "remindusers.hpp"

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/bind.hpp>

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

class Channel;

class ConnectionHandler
{
public:
    ConnectionHandler();
    ConnectionHandler(const std::string &_pass, const std::string &_nick);
    ~ConnectionHandler();

    bool joinChannel(const std::string &channelName);
    bool leaveChannel(const std::string &channelName);
    unsigned int ofChannelCount(const std::string &channel);
    void run();

    // It's not a map of channel sockets, it's a map of channels.
    // I would just rename this to channels
    std::map<std::string, Channel> channels;

    // Login details
    std::string pass;
    std::string nick;

    const BoostConnection::endpoint &
    getEndpoint() const
    {
        return this->twitchEndpoint;
    }

    void shutdown();

    std::chrono::steady_clock::time_point runTime =
        std::chrono::steady_clock::now();

    RemindUsers userReminders;

    Afkers afkers;

    ComeBackMsg comebacks;

    void addBlacklist(const std::string &search, const std::string &replace);
    void removeBlacklist(const std::string &search);
    void sanitizeMsg(std::string &msg);
    bool isBlacklisted(const std::string &msg);

private:
    std::mutex channelMtx;
    std::shared_mutex blacklistMtx;
    std::map<std::string, std::string> blacklist;

    // A bool for quit? checking
    std::atomic<bool> quit;

    boost::asio::io_service ioService;

    // Dummy work that makes sure ioService doesn't shut down
    std::unique_ptr<boost::asio::io_service::work> dummyWork;

    BoostConnection::resolver resolver;

    // Iterator for twitch chat server endpoints
    BoostConnection::endpoint twitchEndpoint;

    // steady_timer handler which decreases the messageCount on all Channels
    void MsgDecreaseHandler(const boost::system::error_code &ec);

    RedisAuth authFromRedis;

    void loadAllReminders();
    void start();
};

#endif
