#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "eventqueue.hpp"

#include <boost/asio.hpp>

#include <atomic>
#include <chrono>
#include <map>
#include <string>

class ConnHandler;

class Channel
{
public:
    Channel(const std::string &_channelName, BotEventQueue &evq,
            boost::asio::io_service &io_s, ConnHandler *_owner);
    ~Channel();

    void read();
    bool sendMsg(const std::string &msg);
    void ping();

    // Channel name (i.e. "pajlada" or "forsenlol)
    std::string channelName;

    // What does the event queue do?
    BotEventQueue &eventQueue;

    // Set to true if the channel should stop reading new messages
    std::atomic<bool> quit;

    // right now public, connhandler is using it
    unsigned int messageCount = 0;
    // Socket so we can send messages
    boost::asio::ip::tcp::socket sock;

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
    // XXX: This doesn't even seem to be used
    std::atomic<bool> pingReplied;

    // Used in anti-spam measure so we don't get globally banned
    std::chrono::high_resolution_clock::time_point lastMessageTime;

    // The ConnHandler managin this channel, should be only one in whole app
    ConnHandler *owner;

    // thread that reads and we join
    std::thread readThread;
};

#endif
