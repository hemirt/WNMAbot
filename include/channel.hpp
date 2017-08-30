#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "commandshandler.hpp"
#include "connection.hpp"
#include "credentials.hpp"
#include "messagehandler.hpp"
#include "messenger.hpp"
#include "network.hpp"
#include "pingme.hpp"
#include "ignore.hpp"

#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <string>
#include <vector>
#include <memory>

class ConnectionHandler;

class Channel : public MessageHandler, public std::enable_shared_from_this<Channel>
{
public:
    Channel(const std::string &_channelName,
            boost::asio::io_service &_ioService, ConnectionHandler *_owner);
    ~Channel();

    std::deque<Connection> connections;

    bool say(const std::string &message);
    bool whisper(const std::string &message, const std::string &recipient);
    void ping();
    bool handleMessage(const IRCMessage &message) final;

    // Channel name (i.e. "pajlada" or "forsenlol)
    std::string channelName;

    // right now public, connhandler is using it
    std::atomic<unsigned int> messageCount;

    bool
    operator<(const Channel &r) const
    {
        return channelName < r.channelName;
    }
    bool
    operator==(const Channel &r) const
    {
        return channelName == r.channelName;
    }


    // The ConnectionHandler managin this channel, should be only one in whole
    // app
    ConnectionHandler *owner;

    CommandsHandler commandsHandler;
    Messenger messenger;
    PingMe &pingMe;
    Ignore ignore;
    
    std::shared_ptr<Channel> getShared() {
        return shared_from_this();
    }
    
    void createConnection();
    
    void shutdown();

private:
    // Create a new connection add it to the connections vector

    // Send a message to one (random?) currently 0th connection
    void sendToOne(const std::string &message);
    
    // send Pong to 0th connection;
    void pong();

    // Send a message to all connections
    void sendToAll(const std::string &message);

    boost::asio::io_service &ioService;

    Credentials credentials;

    std::chrono::steady_clock::time_point connectedTime =
        std::chrono::steady_clock::now();
};

#endif
