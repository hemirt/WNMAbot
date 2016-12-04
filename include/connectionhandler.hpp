#ifndef CONNHANDLER_HPP
#define CONNHANDLER_HPP

#include "eventqueue.hpp"
#include "utilities.hpp"

#include <boost/asio.hpp>

#include <map>
#include <memory>
#include <string>

class Channel;

class ConnectionHandler
{
public:
    ConnectionHandler(const std::string &_pass, const std::string &_nick);
    ~ConnectionHandler();

    bool joinChannel(const std::string &channelName);
    bool leaveChannel(const std::string &channelName);
    void run();

    bool handleMessage(const std::string &user, const std::string &channelName,
                       const std::string &msg);

    // Send message to channel
    void sendMsg(const std::string &channel, const std::string &message);

    // It's not a map of channel sockets, it's a map of channels.
    // I would just rename this to channels
    std::map<std::string, Channel> channels;

    BotEventQueue eventQueue;

    // Iterator for twitch chat server endpoints
    boost::asio::ip::tcp::resolver::iterator twitchResolverIterator;

    // Login details
    std::string pass;
    std::string nick;

private:
    // What does this mutex do?
    // Naming it "mtx" tells me nothing, other than that it's a mutex
    // But I already know that because of its type
    std::mutex mtx;

    // A bool for quit? checking
    std::atomic<bool> quit;

    boost::asio::io_service ioService;

    // Dummy work that makes sure ioService doesn't shut down
    boost::asio::io_service::work dummyWork;

    std::thread asioThread;

    // Thread which decreases the messageCount on all Channels
    std::thread msgDecreaser;
};

#endif
