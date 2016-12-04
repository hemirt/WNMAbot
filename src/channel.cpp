#include "channel.hpp"
#include "connhandler.hpp"

#include <iostream>
#include <string>

void
handler(const boost::system::error_code &error, std::size_t bytes_transferred)
{
}

Channel::Channel(const std::string &_channelName, BotEventQueue &evq,
                 boost::asio::io_service &io_s, ConnHandler *_owner)
    : channelName(_channelName)
    , eventQueue(evq)
    , pingReplied(false)
    , quit(false)
    , sock(io_s)
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
    if (messageCount < 19 &&
        std::chrono::duration_cast<std::chrono::milliseconds>(timeNow -
                                                              lastMessageTime)
                .count() > 1500) {
        std::string sendirc = "PRIVMSG #" + channelName + " :";
        sendirc += msg + ' ';
        if (sendirc.length() > 387)
            sendirc = sendirc.substr(0, 387) + "\r\n";
        else
            sendirc += "\r\n";
        sock.async_send(boost::asio::buffer(sendirc),
                        handler);  // define handler
        messageCount++;
        lastMessageTime = timeNow;
        return true;
    } else {
        return false;
    }
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
            std::cout << "xD" << std::endl;
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
