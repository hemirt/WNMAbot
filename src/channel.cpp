#include "channel.hpp"
#include "connectionhandler.hpp"
#include "utilities.hpp"
#include "hemirt.hpp"
#include "acccheck.hpp"

#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/steady_timer.hpp>

Channel::Channel(const std::string &_channelName,
                 boost::asio::io_service &_ioService, ConnectionHandler *_owner)
    : channelName(_channelName)
    , messageCount(0)
    , owner(_owner)
    , commandsHandler(_ioService, this)
    , messenger(_ioService,
                std::bind(&Channel::say, this, std::placeholders::_1))
    , pingMe(PingMe::getInstance())
    , ioService(_ioService)
    , credentials(owner->nick, owner->pass)
{
    if (this->channelName == "forsen") {
        this->messenger.setMaxLen(135);
    }
}

Channel::~Channel()
{
    this->shutdown();
    std::cout << "Channel::~Channel(): " << this->channelName << std::endl;
}

void
Channel::shutdown()
{
    for (auto & conn : this->connections) {
        conn->shutdown();
        std::cout << "Channel shutdown connection: " << this->channelName << std::endl;
    }
}

void
Channel::createConnection()
{
    this->connections.emplace_back(std::make_shared<Connection>(this->ioService, this->owner->getEndpoint(),
                                   this->credentials, this->channelName));
    if (this->connections.size() != 1) {
        for (auto & conn : this->connections) {
            conn->shutdown();
        }
        this->connections.emplace_back(std::make_shared<Connection>(this->ioService, this->owner->getEndpoint(),
                                   this->credentials, this->channelName));
        auto& connptrref = this->connections[0];
        connptrref->startConnect(this->getShared());
    } else {
        auto& connptrref = this->connections[0];
        connptrref->startConnect(this->getShared());
    }
}

bool
Channel::say(const std::string &message)
{
    if (messageCount >= 19) {
        // Too many messages sent recently
        return false;
    }
    if (Hemirt::banned(message, this->channelName)) {
        return false;
    }

    std::string rawMessage = "PRIVMSG #" + this->channelName + " :" + message;

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
            
            static std::atomic<bool> maintenance = false;
            if ((message.user == "hemirt") && (message.message.compare(0, strlen("!maintenance"), "!maintenance") == 0 )) {
                maintenance = !maintenance;
                std::cout << "maintenance: " << maintenance << std::endl;
            }
            if (maintenance && message.user != "hemirt") {
                return true;
            }
            
            auto afk = owner->afkers.getAfker(message.user);
            if (afk.exists && !afk.announce) {
                auto now = std::chrono::system_clock::now();
                if (boost::iequals(message.message.substr(0, 5), "!back")) {
                    owner->usage.IncrAll(message.user, message.channel, "!back");
                    owner->afkers.removeAfker(message.user);
                    owner->usage.IncrAll(message.user, message.channel, "!afk comeback");

                    auto seconds =
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now() - afk.time)
                            .count();
                    std::string howLongWasGone = makeTimeString(seconds);

                    if (afk.message.empty()) {
                        std::string msg = message.user + " is back(" + howLongWasGone + " ago) HeyGuys";
                        if (Hemirt::banned(msg, this->channelName)) {
                            msg = message.user + " is back(" + howLongWasGone + " ago)";
                        }                        
                        this->messenger.push_back(msg);
                    } else {
                        if (message.channel == "forsen" || message.channel == "nani") {
                            boost::algorithm::replace_all(afk.message, "ResidentSleeper", "zzz");
                        }
                        
                        if (Hemirt::banned(afk.message, this->channelName)) {
                            this->messenger.push_back(message.user + " is back(" + howLongWasGone + " ago): BANPHRASED MESSAGE");
                        } else {
                            auto msg = message.user + " is back(" + howLongWasGone + " ago): " + afk.message;
                            auto vec = splitIntoChunks(std::move(msg), 350);
                            this->messenger.push_back(std::move(vec));
                        }
                    }
                } else if (std::chrono::duration_cast<std::chrono::seconds>(now -
                                                                     afk.time)
                            .count() > 15) {
                    owner->afkers.removeAfker(message.user);
                    owner->usage.IncrAll(message.user, message.channel, "!shadow afk comeback");
                } else {
                    afk.time = now;
                    owner->afkers.updateAfker(message.user, afk);
                }
            }
            if (afk.exists && afk.announce) {
                int maxlen = 350;
                if (this->channelName == "forsen") {
                    maxlen = 135;
                } else if (this->channelName == "pajlada") {
                    maxlen = 500;
                }
                auto now = std::chrono::system_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now -
                                                                     afk.time)
                            .count() > 60 ||
                    boost::iequals(message.message.substr(0, 5), "!back")) {
                    if (boost::iequals(message.message.substr(0, 5), "!back")) {
                        owner->usage.IncrAll(message.user, message.channel, "!back");
                    }
                    owner->afkers.removeAfker(message.user);
                    owner->usage.IncrAll(message.user, message.channel, "!afk comeback");

                    auto seconds =
                        std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now() - afk.time)
                            .count();
                    std::string howLongWasGone = makeTimeString(seconds);

                    if (afk.message.empty()) {
                        std::string msg = message.user + " is back(" + howLongWasGone + " ago) HeyGuys";
                        if (Hemirt::banned(msg, this->channelName)) {
                            msg = message.user + " is back(" + howLongWasGone + " ago)";
                        }
                        this->messenger.push_back(msg);
                    } else {
                        if (message.channel == "forsen" || message.channel == "nani") {
                            boost::algorithm::replace_all(afk.message, "ResidentSleeper", "zzz");
                        }
                        
                        if (Hemirt::banned(afk.message, this->channelName)) {
                            this->messenger.push_back(message.user + " is back(" + howLongWasGone + " ago): BANPHRASED MESSAGE");
                        } else {
                            auto msg = message.user + " is back(" + howLongWasGone + " ago): " + afk.message;
                            auto vec = splitIntoChunks(std::move(msg), maxlen);
                            this->messenger.push_back(std::move(vec));
                        }
                    }
                } else {
                    afk.time = now;
                    owner->afkers.updateAfker(message.user, afk);
                }
            }

            owner->comebacks.sendMsgs(message.user, this->messenger, this->owner);
            
            
            if (!this->ignore.isIgnored(message.user)) {
                // user is not ignored
                auto response = this->commandsHandler.handle(message);
                
                if (response.type != Response::Type::UNKNOWN) {
                    this->owner->usage.IncrAll(message.user, message.channel, response.trigger);
                    if (AccCheck::ageById(message.userid).count() < 47335428LL && !AccCheck::isAllowed(message.user)) {
                        return true;
                    }
                }

                if (response.type == Response::Type::MESSAGE) {
                    int maxlen = 350;
                    if (this->channelName == "forsen") {
                        maxlen = 135;
                    }
                    if (response.priority == 1) {
                        auto vec =
                            splitIntoChunks(std::move(response.message), maxlen);
                        this->messenger.push_front(std::move(vec));
                    } else if (response.priority == 0) {
                        auto vec =
                            splitIntoChunks(std::move(response.message), maxlen);
                        for (decltype(vec.size()) i = 0; i < 2 && i < vec.size(); i++) {
                            this->messenger.push_back(std::move(vec[i]));
                        }
                    } else if (this->messenger
                                   .able() /* && message.priority == -1 */) {
                        auto vec =
                            splitIntoChunks(std::move(response.message), maxlen);
                        for (decltype(vec.size()) i = 0; i < 2 && i < vec.size(); i++) {
                            this->messenger.push_back(std::move(vec[i]));
                        }
                    }

                } else if (response.type == Response::Type::WHISPER) {
                    this->whisper(response.message, response.whisperReceiver);
                } else {
                    std::vector<std::string> tokens;
                    boost::algorithm::split(tokens, message.message,
                                            boost::algorithm::is_space(),
                                            boost::token_compress_on);

                    changeToLower(tokens[0]);

                    if (tokens[0] == "zululending" && message.user == "hemirt") {
                        // push_front - its a priority message
                        this->messenger.push_front("Shutting down FeelsBadMan");
                        this->owner->shutdown();
                        return false;
                    } else if (this->messenger.able() && (tokens[0] == "!peng" || tokens[0] == "!pingall")) {
                        auto now = std::chrono::steady_clock::now();
                        auto runT =
                            std::chrono::duration_cast<std::chrono::seconds>(
                                now - owner->runTime);
                        auto conT =
                            std::chrono::duration_cast<std::chrono::seconds>(
                                now - connectedTime);

                        auto running = makeTimeString(runT.count());
                        auto connected = makeTimeString(conT.count());
                        this->messenger.push_back(
                            "Running for " + running +
                            ". Connected to this channel for " + connected + ".");
                    }
                }
            } // end of not ignored user

            if (this->pingMe.isPinger(message.user)) {
                auto map = this->pingMe.ping(message.user);
                if (!map.empty()) {
                    std::string users;

                    for (const auto &i : map) {
                        // i.first == channel
                        // i.second == vector of users to ping
                        if (this->owner->ofChannelCount(i.first) == 1 &&
                            !i.second.empty()) {
                            std::string users;
                            for (const auto &i : i.second) {
                                users += i + ", ";
                            }
                            if (users.empty()) {
                                continue;
                            }
                            users.pop_back();
                            users.pop_back();
                            if (i.first != this->channelName) {
                                users +=
                                    ". At channel " + this->channelName + ".";
                            }
                            int maxlen = 350;
                            if (this->channelName == "forsen") {
                                maxlen = 135;
                            }
                            auto vec = splitIntoChunks(
                                message.user + " is back: " + users, maxlen);

                            try {
                                for (auto &msg : vec) {
                                    this->owner->channels.at(i.first)
                                        ->messenger.push_back(std::move(msg));
                                }
                            } catch (std::exception &e) {
                                std::cerr << "Ping me exception " << i.first
                                          << " " << e.what() << std::endl;
                            }
                        }
                    }
                }
            }

            return true;
        } break;

        case IRCMessage::Type::PING: {
            // TODO: This needs to reply to the specific connection
            // handleMessage should take a shared_ptr to the connection the
            // message came from as a parameter
            // only relevant when we have more than one connection per channel
            //std::cout << "PING MESSAGE (" << static_cast<std::underlying_type<IRCMessage::Type>::type>(message.type) << ") :\n" << message << "\n" << std::endl;
            this->pong();
        } break;

        default: {
            //std::cout << "UNKNOWN MESSAGE (" << static_cast<std::underlying_type<IRCMessage::Type>::type>(message.type) << ") :\n" << message << "\n" << std::endl;
            // Unknown message type
        } break;
    }

    return true;
}

void
Channel::pong()
{
    auto &connection = this->connections.at(0);
    connection->pong();
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

    connection->writeMessage(message);
}

void
Channel::sendToAll(const std::string &message)
{
    if (messageCount >= 19) {
        // Too many messages sent recently
        return;
    }

    for (auto &connection : this->connections) {
        connection->writeMessage(message);
    }
}
