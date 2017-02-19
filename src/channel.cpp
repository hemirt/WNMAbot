#include "channel.hpp"
#include "connectionhandler.hpp"
#include "utilities.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/algorithm/string/predicate.hpp>

Channel::Channel(const std::string &_channelName,
                 boost::asio::io_service &_ioService, ConnectionHandler *_owner)
    : channelName(_channelName)
    , pingReplied(false)
    , owner(_owner)
    , ioService(_ioService)
    , credentials(owner->nick, owner->pass)
    , messageCount(0)
    , commandsHandler(_ioService, this)
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
Channel::whisper(const std::string &message, const std::string &recipient)
{
    std::string rawMessage = "PRIVMSG #jtv :/w " + recipient + " ";
    // Message length at most 350 characters
    if (message.length() >= 350) {
        rawMessage += message.substr(0, 350);
    } else {
        rawMessage += message;
    }
    this->sendToOne(rawMessage);
    return true;
}

bool
Channel::handleMessage(const IRCMessage &message)
{
    switch (message.type) {
        case IRCMessage::Type::PRIVMSG: {
            // std::cout << '#' << message.channel << ": " << message.user << ":
            // " << message.params << std::endl;

            auto timeNow = std::chrono::high_resolution_clock::now();
            auto msSinceLastMessage =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeNow - lastMessageTime);

            bool sent = false;
            {
                auto afk = owner->afkers.getAfker(message.user);
                if (afk.exists) {
                    auto now = std::chrono::steady_clock::now();
                    if (std::chrono::duration_cast<std::chrono::seconds>(
                            now - afk.time)
                            .count() > 10 || boost::iequals(message.params.substr(0, 5), "!back")) {
                        owner->afkers.removeAfker(message.user);

                        if (msSinceLastMessage.count() <= 1500) {
                            auto t = new boost::asio::steady_timer(
                                ioService, std::chrono::milliseconds(1500));
                            t->async_wait([
                                user = message.user, message = afk.message, this
                            ](const boost::system::error_code &er) {
                                if (er) {
                                    return;
                                }
                                if (std::chrono::duration_cast<
                                        std::chrono::milliseconds>(
                                        std::chrono::high_resolution_clock::
                                            now() -
                                        this->lastMessageTime)
                                        .count() <= 1500) {
                                    return;
                                }
                                if(message.empty()) {
                                    this->say(user + " is back HeyGuys");
                                } else {
                                    this->say(user + " is back: " + message);
                                }
                                
                            });
                        } else {
                            if(afk.message.empty()) {
                                sent = this->say(message.user + " is back HeyGuys");
                            } else {
                                sent = this->say(message.user + " is back: " +
                                             afk.message);
                            }
                        }
                    } else {
                        afk.time = now;
                        owner->afkers.updateAfker(message.user, afk);
                    }
                }
            }

            owner->comebacks.sendMsgs(message.user);

            if (messageCount >= 19) {
                // Too many messages sent recently
                return false;
            }

            if (msSinceLastMessage.count() <= 1500) {
                // Last message was sent less than 1.5 seconds ago
                return false;
            }

            const auto response = this->commandsHandler.handle(message);

            if (response.type == Response::Type::MESSAGE) {
                if (sent) {
                    auto t = new boost::asio::steady_timer(
                        ioService, std::chrono::milliseconds(1550));
                    t->async_wait([
                        user = message.user, message = response.message, this
                    ](const boost::system::error_code &er) {
                        if (er) {
                            return;
                        }
                        if (std::chrono::duration_cast<
                                std::chrono::milliseconds>(
                                std::chrono::high_resolution_clock::now() -
                                this->lastMessageTime)
                                .count() <= 1500) {
                            return;
                        }
                        this->say(message);
                    });
                } else {
                    sent = this->say(response.message);
                }
            } else if (response.type == Response::Type::WHISPER) {
                this->whisper(response.message, response.whisperReceiver);
            }
            if (!sent) {
                std::vector<std::string> tokens;
                boost::algorithm::split(tokens, message.params,
                                        boost::algorithm::is_space(),
                                        boost::token_compress_on);

                changeToLower(tokens[0]);

                if (tokens[0] == "zululending" && message.user == "hemirt") {
                    sent = this->say("Shutting down FeelsBadMan");
                    this->owner->shutdown();
                } else if (tokens[0] == "!peng") {
                    auto now = std::chrono::steady_clock::now();
                    auto runT =
                        std::chrono::duration_cast<std::chrono::seconds>(
                            now - owner->runTime);
                    auto conT =
                        std::chrono::duration_cast<std::chrono::seconds>(
                            now - connectedTime);

                    auto convert = [](std::chrono::seconds T) -> std::string {
                        std::string str;
                        int seconds = T.count() % 60;
                        int minutes = T.count() / 60;
                        int hours = minutes / 60;
                        minutes %= 60;
                        int days = hours / 24;
                        hours %= 24;
                        std::stringstream ss;
                        ss << " ";
                        if (days != 0) {
                            ss << days;
                            if (days == 1) {
                                ss << " day ";
                            } else {
                                ss << " days ";
                            }
                        }
                        if (hours != 0) {
                            ss << hours;
                            if (hours == 1) {
                                ss << " hour ";
                            } else {
                                ss << " hours ";
                            }
                        }
                        if (minutes != 0) {
                            ss << minutes;
                            if (minutes == 1) {
                                ss << " minute ";
                            } else {
                                ss << " minutes ";
                            }
                        }
                        if (seconds != 0) {
                            ss << seconds;
                            if (seconds == 1) {
                                ss << " second ";
                            } else {
                                ss << " seconds ";
                            }
                        }
                        str = ss.str();
                        str.pop_back();
                        return str;
                    };

                    auto running = convert(runT);
                    auto connected = convert(conT);
                    sent = this->say("Running for" + running +
                                     ". Connected to this channel for" +
                                     connected + ".");
                }
            }

            if (sent) {
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
    if (messageCount >= 19) {
        // Too many messages sent recently
        return;
    }

    // TODO: randomize which connection to send to if we have multiple
    auto &connection = this->connections.at(0);

    connection.writeMessage(message);
}

void
Channel::sendToAll(const std::string &message)
{
    if (messageCount >= 19) {
        // Too many messages sent recently
        return;
    }

    for (auto &connection : this->connections) {
        connection.writeMessage(message);
    }
}
