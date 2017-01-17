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
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/steady_timer.hpp>

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
    messageCount++;
    return true;
}

bool
Channel::handleMessage(const IRCMessage &message)
{
    switch (message.type) {
        case IRCMessage::Type::PRIVMSG: {
            // std::cout << '#' << message.channel << ": " << message.user << ":
            // " << message.params << std::endl;
            if (messageCount >= 19) {
                // Too many messages sent recently
                return false;
            }

            auto timeNow = std::chrono::high_resolution_clock::now();
            auto msSinceLastMessage =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeNow - lastMessageTime);
            if (msSinceLastMessage.count() <= 1500) {
                // Last message was sent less than 1.5 seconds ago
                return false;
            }

            bool sent = false;
            const auto response = this->commandsHandler.handle(message);

            if (response.type == Response::Type::MESSAGE) {
                sent = this->say(response.message);
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
                } else if (tokens[0] == "!joinchn" &&
                           message.user == "hemirt") {
                    std::string msg = message.params;
                    msg.erase(0, strlen("!joinchn "));
                    if (msg.find(' ') == std::string::npos) {
                        this->owner->joinChannel(msg);
                        sent = this->say("Joined the " + msg + " channel.");
                    }
                } else if (tokens[0] == "!leavechn" &&
                           message.user == "hemirt") {
                    std::string msg = message.params;
                    msg.erase(0, strlen("!leavechn "));
                    if (msg.find(' ') == std::string::npos) {
                        this->owner->leaveChannel(msg);
                        sent = this->say("Left the " + msg + " channel.");
                    }
                } else if (tokens[0] == "!chns" && message.user == "hemirt") {
                    std::string channels;
                    for (const auto &i : owner->channels) {
                        channels += i.first + ", ";
                    }
                    channels.pop_back();
                    channels.pop_back();
                    sent = this->say("Currently active in channels " +
                                     channels + ".");
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
                } else if (tokens[0] == "!remindme") {
                    auto i = tokens.size();
                    for (; i-- > 0;) {
                        if (tokens[i] == "in" || tokens[i] == "IN" ||
                            tokens[i] == "iN" || tokens[i] == "In") {
                            break;
                        }
                    }
                    if (i == 0 || i + 1 >= tokens.size()) {
                        this->whisper("Usage \"!remindme [your message] in 20s "
                                      "15h 10d 9m (the order does not matter, "
                                      "the number must be immediately followed "
                                      "by an identifier)\"",
                                      message.user);
                        return false;
                    }
                    int end = i;
                    ++i;
                    long long seconds = 0;
                    for (; i < tokens.size(); i++) {
                        if (tokens[i].back() == 'd') {
                            seconds +=
                                std::atoll(tokens[i].c_str()) * 24 * 3600;

                        } else if (tokens[i].back() == 'h') {
                            seconds += std::atoll(tokens[i].c_str()) * 3600;
                        } else if (tokens[i].back() == 'm') {
                            seconds += std::atoll(tokens[i].c_str()) * 60;
                        } else if (tokens[i].back() == 's') {
                            seconds += std::atoll(tokens[i].c_str());
                        }
                    }
                    if (seconds == 0) {
                        this->whisper("Usage \"!remindme [your message] in 20s "
                                      "15h 10d 9m (the order does not matter, "
                                      "the number must be immediately followed "
                                      "by an identifier)\"",
                                      message.user);
                        return false;
                    } else if (seconds > 604800) {
                        this->whisper("Maximum amount of time is 7 days NaM",
                                      message.user);
                        return false;
                    } else if (seconds < 0) {
                        this->whisper("Negative amount of time "
                                      "\xF0\x9F\xA4\x94, sorry I can't travel "
                                      "back in time (yet)",
                                      message.user);
                        return false;
                    }

                    int j = 1;
                    std::string str;
                    for (; j < end; j++) {
                        str += tokens[j] + " ";
                    }
                    if (str.back() == ' ')
                        str.pop_back();
                    std::string responsex = "Your message: " + str;
                    if (str.empty()) {
                        responsex = "You didnt set any message so here's your "
                                    "anonymous reminder eShrug";
                    }

                    auto remindFunction =
                        [ owner = this->owner, user = message.user,
                          responsex ](const boost::system::error_code &er)
                            ->void
                    {
                        if (owner->channels.empty()) {
                            owner->joinChannel(owner->nick);
                        }

                        owner->channels.at(owner->nick)
                            .whisper(responsex, user);
                    };

                    auto t = new boost::asio::steady_timer(
                        ioService, std::chrono::seconds(seconds));
                    t->async_wait(remindFunction);
                    std::string msg = message.user + ", reminding you in " +
                                      std::to_string(seconds) + " seconds";
                    if (j != 1) {
                        msg += " of " + str + " SeemsGood";
                    } else {
                        msg += " SeemsGood";
                    }
                    sent = this->say(msg);
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
