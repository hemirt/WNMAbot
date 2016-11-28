#ifndef CHANNEL_HPP
#define CHANNEL_HPP

/// Includes are spread up into three parts
// 1st party files
#include "eventqueue.hpp"
#include "connhandler.hpp"

// 3rd party files
#include "asio.hpp"

// stdlib includes
#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <string>

typedef EventQueue<std::pair<std::unique_ptr<asio::streambuf>, std::string>> BotEventQueue;
class ConnHandler;

class Channel
{
public:
    Channel(const std::string &_channelName,
            BotEventQueue &evq,
            const asio::io_service &io_s,
            ConnHandler *_owner);
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

    //right now public, connhandler is using it
    unsigned int messageCount = 0;
    // Socket so we can send messages
    asio::ip::tcp::socket sock;
    
    const bool operator<(const Channel &r) const
    {
        return channelName < r.channelName;
    }
    const bool operator==(const Channel &r) const
    {
        return channelName == r.channelName;
    }
    
private:
    // XXX: This doesn't even seem to be used
    std::atomic<bool> pingReplied;

    // Used in anti-spam measure so we don't get globally banned
    std::chrono::high_resolution_clock::time_point lastMessageTime;
    
    //The ConnHandler managin this channel, should be only one in whole app
    ConnHandler *owner;
    
    //thread that reads and we join
    std::thread readThread;
};

#endif