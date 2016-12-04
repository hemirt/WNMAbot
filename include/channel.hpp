#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "connection.hpp"
#include "credentials.hpp"
#include "eventqueue.hpp"
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
    Channel(const std::string &_channelName, BotEventQueue &_eventQueue,
            boost::asio::io_service &_ioService, ConnectionHandler *_owner);
    ~Channel();

    std::deque<Connection> connections;

    bool say(const std::string &message);
    void ping();
    bool handleMessage(const IRCMessage &message) final;

    // Channel name (i.e. "pajlada" or "forsenlol)
    std::string channelName;

    // What does the event queue do?
    BotEventQueue &eventQueue;

    // Set to true if the channel should stop reading new messages
    std::atomic<bool> quit;

    // right now public, connhandler is using it
    unsigned int messageCount = 0;

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

    // The ConnectionHandler managin this channel, should be only one in whole
    // app
    ConnectionHandler *owner;

    Credentials credentials;
};

#endif
