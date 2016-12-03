#ifndef CONNHANDLER_HPP
#define CONNHANDLER_HPP

#include "eventqueue.hpp"
#include "utilities.hpp"

#include <boost/asio.hpp>

#include <map>
#include <memory>
#include <string>

class Channel;

class ConnHandler
{
public:
    ConnHandler(const std::string &pass, const std::string &nick);
    ~ConnHandler();

    void joinChannel(const std::string &);
    void leaveChannel(const std::string &);
    void run();

    // user should be a const string reference
    void handleCommands(std::string &user, const std::string &channel,
                        std::string &msg);

    // Send message to channel
    void sendMsg(const std::string &channel, const std::string &message);

    // It's not a map of channel sockets, it's a map of channels.
    // I would just rename this to channels
    std::map<std::string, std::unique_ptr<Channel>> currentChannels;

    BotEventQueue eventQueue;

    // Iterator for twitch chat server endpoints
    boost::asio::ip::tcp::resolver::iterator twitch_it;

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

    // Why do you shorten this?
    // TODO: rename to ioService
    boost::asio::io_service io_s;

    // Dummy work that we can start/stop at will to control the ioService
    std::unique_ptr<boost::asio::io_service::work> dummywork;

    // Thread which decreases the messageCount on all Channels
    std::thread msgDecreaser;
};

#endif
