#include "channel.hpp"
#include "connectionhandler.hpp"

#include <iostream>
#include <string>

Channel::Channel(const std::string &_channelName,
                 boost::asio::io_service &_ioService, ConnectionHandler *_owner)
    : channelName(_channelName)
    , pingReplied(false)
    , owner(_owner)
    , ioService(_ioService)
    , credentials(owner->nick, owner->pass)
    , messageCount(0)
    , commandsHandler(_ioService)
{
    // Create initial connection
    this->createConnection();
}

Channel::~Channel()
{
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
    std::string rawMessage = "PRIVMSG #" + this->channelName + " :";

    // Message length at most 350 characters
    if (message.length() >= 350) {
        rawMessage += message.substr(0, 350);
    } else {
        rawMessage += message;
    }

    this->sendToOne(rawMessage);

    messageCount++;

    return true;
}



bool
Channel::handleMessage(const IRCMessage &message)
{
    switch (message.type) {
        case IRCMessage::Type::PRIVMSG: {
            //std::cout << '#' << message.channel << ": " << message.user << ": " << message.params << std::endl;
            
            if (messageCount >= 19) {
                // Too many messages sent recently
                return false;
            }
            
            auto timeNow = std::chrono::high_resolution_clock::now();
            auto msSinceLastMessage =
                std::chrono::duration_cast<std::chrono::milliseconds>(timeNow -
                                                                      lastMessageTime);
            if (msSinceLastMessage.count() <= 1500) {
                // Last message was sent less than 1.5 seconds ago
                return false;
            }
            
            bool sent = false;
            const auto response = this->commandsHandler.handle(message);

            if (response.valid)
                sent = this->say(response.message);

            if (message.params.find("ZULULending") != std::string::npos &&
                message.user == "hemirt") {
                sent = this->say("Shutting down FeelsBadMan");
                this->owner->shutdown();
            }
            
            if(sent) {
                lastMessageTime = timeNow;
            }
            
            return true;
        } break;

        case IRCMessage::Type::PING: {
            // TODO: This needs to reply to the specific connection
            // handleMessage should take a shared_ptr to the connection the
            // message came from as a parameter
            // only relevant when we have more than one connection per channel
            this->sendToOne("PONG :" + message.params);
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
