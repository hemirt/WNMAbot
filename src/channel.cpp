#include "channel.hpp"
#include "connectionhandler.hpp"

#include <iostream>
#include <string>

void
handler(const boost::system::error_code &error, std::size_t bytes_transferred)
{
}

Channel::Channel(const std::string &_channelName, BotEventQueue &_eventQueue,
                 boost::asio::io_service &ioService, ConnectionHandler *_owner)
    : channelName(_channelName)
    , eventQueue(_eventQueue)
    , pingReplied(false)
    , quit(false)
    , sock(ioService)
    , owner(_owner)
    , readThread(&Channel::read, this)
{
    boost::asio::connect(sock, owner->twitch_it);

    std::string passx = "PASS " + owner->pass + "\r\n";
    std::string nickx = "NICK " + owner->nick + "\r\n";
    std::string cmds = "CAP REQ :twitch.tv/commands\r\n";
    std::string join = "JOIN #" + channelName + "\r\n";

    sock.send(boost::asio::buffer(passx));
    sock.send(boost::asio::buffer(nickx));
    sock.send(boost::asio::buffer(cmds));
    sock.send(boost::asio::buffer(join));
}

Channel::~Channel()
{
    quit = true;
    sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    sock.close();
    std::cout << "channel " << channelName << " before join" << std::endl;
    // std::cout << "threadjoinable: " << readThread.joinable() << std::endl;
    readThread.join();
    std::cout << "channel " << channelName << " destructed" << std::endl;
}

bool
Channel::sendMsg(const std::string &msg)
{
    auto timeNow = std::chrono::high_resolution_clock::now();
    auto msSinceLastMessage =
        std::chrono::duration_cast<std::chrono::milliseconds>(timeNow -
                                                              lastMessageTime);

    std::cerr << messageCount << std::endl;
    if (messageCount >= 19) {
        // Too many messages sent recently
        return false;
    }

    if (msSinceLastMessage.count() <= 1500) {
        // Last message was sent less than 1.5 seconds ago
        return false;
    }

    std::string rawMessage = "PRIVMSG #" + channelName + " :";

    // Message length at most 350 characters
    if (msg.length() >= 350) {
        rawMessage += msg.substr(0, 350);
    } else {
        rawMessage += msg;
    }

    rawMessage += " \r\n";

    sock.async_send(boost::asio::buffer(rawMessage),
                    handler);  // define handler

    messageCount++;
    lastMessageTime = timeNow;

    return true;
}

bool
Channel::handleMessage(const std::string &user, const std::string &msg)
{
    if (user == "hemirt") {
        return this->sendMsg("EleGiggle");

        /*
        if (msg == "!dung") {
            quit = true;
        } else if (msg == "!deng") {
            leaveChannel(channel);
        }
        */
    } else if (user == "pajlada") {
        return this->sendMsg("KKona");
    }

    return false;
}

void
Channel::read()
{
    std::cout << "started reading: " << channelName << std::endl;
    // int i = 0;
    try {
        while (!this->quit) {
            // i++;
            // std::cout << i << channelName << std::endl;
            // if(i > 7) this->quit = true;
            std::unique_ptr<boost::asio::streambuf> b(
                new boost::asio::streambuf);
            boost::system::error_code ec;
            boost::asio::read_until(sock, *b, "\r\n", ec);
            if (!(ec && !(this->quit))) {
                eventQueue.push(
                    std::pair<std::unique_ptr<boost::asio::streambuf>,
                              std::string>(std::move(b), channelName));
            } else
                break;
        }
    } catch (std::exception &e) {
        std::cout << "exception running Channel" << channelName << " "
                  << e.what() << std::endl;
    }

    if (!(this->quit)) {
        // this restarted reading, but tbh its quite pointless because it stops
        // reading only
        // when quiting(destroying this) or when the socket is in a wrong state,
        // therefore we need to recreate this object

        // this->readThread.detach();
        // this->readThread = std::thread(&Channel::read, this);

        // cant do this, cause it results in a deadlock (this function will wait
        // for this function to end)
        // owner->leaveChannel(channelName);
    }
}
