#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "commandshandler.hpp"
#include "connection.hpp"
#include "credentials.hpp"
#include "messagehandler.hpp"
#include "network.hpp"

#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <string>
#include <vector>

class ConnectionHandler;

class Channel : public MessageHandler
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

    const bool
    operator<(const Channel &r) const
    {
        return channelName < r.channelName;
    }
    const bool
    operator==(const Channel &r) const
    {
        return channelName == r.channelName;
    }

    // The ConnectionHandler managin this channel, should be only one in whole
    // app
    ConnectionHandler *owner;

    CommandsHandler commandsHandler;

private:
    // Create a new connection add it to the connections vector
    void createConnection();

    // Send a message to one (random?) connection
    void sendToOne(const std::string &message);

    // Send a message to all connections
    void sendToAll(const std::string &message);

    // XXX: This doesn't even seem to be used
    std::atomic<bool> pingReplied;

    // Used in anti-spam measure so we don't get globally banned
    std::chrono::high_resolution_clock::time_point lastMessageTime;

    boost::asio::io_service &ioService;

    Credentials credentials;

    std::chrono::steady_clock::time_point connectedTime =
        std::chrono::steady_clock::now();
};

#endif
