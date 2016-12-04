#include "channel.hpp"
#include "connectionhandler.hpp"

#include <iostream>
#include <string>

void
handler(const boost::system::error_code &error, std::size_t bytes_transferred)
{
}

Channel::Channel(const std::string &_channelName, BotEventQueue &_eventQueue,
                 boost::asio::io_service &_ioService, ConnectionHandler *_owner)
    : channelName(_channelName)
    , eventQueue(_eventQueue)
    , pingReplied(false)
    , quit(false)
    , owner(_owner)
    , ioService(_ioService)
    , credentials(owner->nick, owner->pass)
{
    // Create initial connection
    this->createConnection();
}

Channel::~Channel()
{
    quit = true;
}

void
Channel::createConnection()
{
    this->connections.emplace_back(this->ioService, this->owner->getEndpoint(),
                                   this->credentials, this->channelName, this);
}

bool
Channel::say(const std::string &message)
{
    auto timeNow = std::chrono::high_resolution_clock::now();
    auto msSinceLastMessage =
        std::chrono::duration_cast<std::chrono::milliseconds>(timeNow -
                                                              lastMessageTime);

    if (messageCount >= 19) {
        // Too many messages sent recently
        return false;
    }

    if (msSinceLastMessage.count() <= 1500) {
        // Last message was sent less than 1.5 seconds ago
        return false;
    }

    std::string rawMessage = "PRIVMSG #" + this->channelName + " :";

    // Message length at most 350 characters
    if (message.length() >= 350) {
        rawMessage += message.substr(0, 350);
    } else {
        rawMessage += message;
    }

    this->sendToOne(rawMessage);

    messageCount++;
    lastMessageTime = timeNow;

    return true;
}

bool
Channel::handleMessage(const IRCMessage &message)
{
    switch (message.type) {
        case IRCMessage::Type::PRIVMSG: {
            std::cout << message.user << ": " << message.params << std::endl;

            // TODO: Implement command handler here
            if (message.user == "pajlada") {
                this->say("KKona");
            } else if (message.user == "hemirt") {
                this->say("EleGiggle");
            }

            return true;
        } break;
        default: {
            // Unknown message type
        } break;
    }

    return false;
}

void
Channel::sendToOne(const std::string &message)
{
    // TODO: randomize which connection to send to if we have multiple
    auto &connection = this->connections.at(0);

    connection.writeMessage(message);
}

void
Channel::sendToAll(const std::string &message)
{
    for (auto &connection : this->connections) {
        connection.writeMessage(message);
    }
}
