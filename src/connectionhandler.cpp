#include "connectionhandler.hpp"
#include <boost/algorithm/string/replace.hpp>
#include <boost/thread.hpp>
#include "ayah.hpp"
#include "bible.hpp"
#include "hemirt.hpp"
#include "channel.hpp"
#include "randomquote.hpp"
#include "lotto/lottoimpl.hpp"

//static const char *IRC_HOST = "irc.chat.twitch.tv";
//static const char *IRC_PORT = "6667";

ConnectionHandler::ConnectionHandler(const std::string &_pass,
                                     const std::string &_nick)
    : pass(_pass)
    , nick(_nick)
    , comebacks(this->ioService, this)
    , quit(false)
    , dummyWork(new boost::asio::io_service::work(this->ioService))
    , resolver(this->ioService)
    , twitchEndpoint(
          this->resolver
              .resolve(BoostConnection::resolver::query(Network::IRC_HOST, Network::IRC_PORT))
              ->endpoint())
{
    if (!this->authFromRedis.isValid()) {
        throw std::runtime_error("Redis connection error, see std::cerr");
    }
    this->authFromRedis.setOauth(this->pass);
    this->authFromRedis.setName(this->nick);

    this->start();
}

ConnectionHandler::ConnectionHandler()
    : comebacks(this->ioService, this)
    , quit(false)
    , dummyWork(new boost::asio::io_service::work(this->ioService))
    , resolver(this->ioService)
    , twitchEndpoint(
          this->resolver
              .resolve(BoostConnection::resolver::query(Network::IRC_HOST, Network::IRC_PORT))
              ->endpoint())
{
    if (this->authFromRedis.isValid() && this->authFromRedis.hasAuth()) {
        this->pass = this->authFromRedis.getOauth();
        this->nick = this->authFromRedis.getName();
    } else {
        throw std::runtime_error("No authentication provided");
    }

    this->start();
}

void
ConnectionHandler::start()
{
    this->msgDecreaserTimer = std::make_unique<boost::asio::steady_timer>(this->ioService);
    this->msgDecreaserTimer->expires_from_now(std::chrono::seconds(2));
    this->msgDecreaserTimer->async_wait(
        boost::bind(&ConnectionHandler::MsgDecreaseHandler, this, _1));
    
    this->reconnectTimer = std::make_unique<boost::asio::steady_timer>(this->ioService);
    this->reconnectTimer->expires_from_now(std::chrono::minutes(5));
    this->reconnectTimer->async_wait(
        boost::bind(&ConnectionHandler::OwnChannelReconnectHandler, this, _1));
        
    auto channels = this->authFromRedis.getChannels();
    for (const auto &i : channels) {
        joinChannel(i);
    }
    
    if (std::find(channels.begin(), channels.end(), this->nick) == channels.end()) {
        joinChannel(this->nick);
    }

    this->loadAllReminders();

    this->blacklist = this->authFromRedis.getBlacklist();

    Ayah::init();
    Bible::init();
    RandomQuote::init();
    LottoImpl::init();
    Hemirt::init();
}

void
ConnectionHandler::MsgDecreaseHandler(const boost::system::error_code &ec)
{
    this->joinChannel(this->nick);

    if (ec) {
        std::cerr << "MsgDecreaseHandler error " << ec << std::endl;
        
        this->msgDecreaserTimer = std::make_unique<boost::asio::steady_timer>(this->ioService);
        this->msgDecreaserTimer->expires_from_now(std::chrono::seconds(2));
        this->msgDecreaserTimer->async_wait(
            boost::bind(&ConnectionHandler::MsgDecreaseHandler, this, _1));
        return;
    }

    std::lock_guard<std::mutex> lk(channelMtx);

    if (this->quit) {
        this->msgDecreaserTimer.reset();
        return;
    }

    for (auto &i : this->channels) {
        if (i.second->messageCount > 0) {
            --i.second->messageCount;
        }
    }

    this->msgDecreaserTimer = std::make_unique<boost::asio::steady_timer>(this->ioService);
    this->msgDecreaserTimer->expires_from_now(std::chrono::seconds(2));
    this->msgDecreaserTimer->async_wait(
        boost::bind(&ConnectionHandler::MsgDecreaseHandler, this, _1));
}


void
ConnectionHandler::OwnChannelReconnectHandler(const boost::system::error_code &ec)
{
    this->joinChannel(this->nick);

    if (ec) {
        std::cerr << "OwnChannelReconnectHandler error " << ec << std::endl;
        this->reconnectTimer = std::make_unique<boost::asio::steady_timer>(this->ioService);
        this->reconnectTimer->expires_from_now(std::chrono::minutes(5));
        this->reconnectTimer->async_wait(
            boost::bind(&ConnectionHandler::OwnChannelReconnectHandler, this, _1));
        return;
    }

    std::lock_guard<std::mutex> lk(channelMtx);

    if (this->quit) {
        this->reconnectTimer.reset();
        return;
    }

    auto &connections = this->channels.at(this->nick)->connections;
    for (auto &conn : connections) {
        conn->reconnect();
    }

    this->reconnectTimer = std::make_unique<boost::asio::steady_timer>(this->ioService);
    this->reconnectTimer->expires_from_now(std::chrono::minutes(5));
    this->reconnectTimer->async_wait(
        boost::bind(&ConnectionHandler::OwnChannelReconnectHandler, this, _1));
}

void
ConnectionHandler::reconnectAllChannels(const std::string &chn)
{
    std::lock_guard<std::mutex> lk(channelMtx);

    for (auto &i : this->channels) {
        if (i.first == chn) {
            continue;
        }
        auto &connections = i.second->connections;
        for (auto &conn : connections) {
            conn->reconnect();
        }
    }
}

ConnectionHandler::~ConnectionHandler()
{
    std::cout << "destructing" << std::endl;
    this->channels.clear();
    Ayah::deinit();
    Bible::deinit();
    RandomQuote::deinit();
    LottoImpl::deinit();
    Hemirt::deinit();
    this->msgDecreaserTimer.reset();
    std::cout << "cleared end destr" << std::endl;
}

bool
ConnectionHandler::joinChannel(const std::string &channelName)
{
    std::lock_guard<std::mutex> lk(channelMtx);

    if (this->channels.count(channelName) == 1) {
        return false;
    }
    std::cout << "joining: " << channelName << std::endl;

    this->authFromRedis.addChannel(channelName);
    

    auto p = this->channels.emplace(
        std::piecewise_construct, std::forward_as_tuple(channelName),
        std::forward_as_tuple(std::make_shared<Channel>(channelName, this->ioService, this)));
    // p.first == iterator
    // p.first->first == channelName
    // p.first->second == shared_ptr<Channel>
    p.first->second->createConnection();

    return true;
}

bool
ConnectionHandler::leaveChannel(const std::string &channelName)
{
    std::lock_guard<std::mutex> lk(channelMtx);
    std::cout << "leaving: " << channelName << std::endl;

    if (this->channels.count(channelName) == 0) {
        // We are not connected to the given channel
        return false;
    }

    this->authFromRedis.removeChannel(channelName);

    auto shared = this->channels[channelName];
    this->channels.erase(channelName);
    shared->shutdown();
    std::cout << "left: " << channelName << std::endl;


    return true;
}

void
ConnectionHandler::run()
{
    boost::thread_group tg;
    auto hw_concurency_num = boost::thread::hardware_concurrency();
    std::cout << "ConnectionHandler::ioService running on " << hw_concurency_num << " threads" << std::endl;
    for (unsigned i = 0; i < hw_concurency_num; ++i) {
        tg.create_thread([this](){
            boost::system::error_code ec;
            //try {
                
                this->ioService.run(ec);
                std::cout << "IOSERVICE ec: " << ec << std::endl;
            //} catch (const std::exception &ex) {
            //    std::cerr << "IOSERVICE Exception caught in ConnectionHandler::run(): "
            //              << ex.what() << " && ec: " << ec << std::endl;
                if (!this->err) {
                    this->err = true;
                    this->shutdown();
                }
            //}
        });
    }
    tg.join_all();
}

void
ConnectionHandler::shutdown()
{
    std::lock_guard<std::mutex> lk(this->channelMtx);
    //try {
        std::cout << "ConnectionHandler::shutdown() START" << std::endl;
        this->quit = true;
        for (auto &p : this->channels) {
            p.second->shutdown();
        }
        this->channels.clear();
        this->dummyWork.reset();
        this->ioService.stop();
        std::cout << "ConnectionHandler::shutdown() END" << std::endl;

    //} catch (const std::exception &ex) {
    //    std::cerr << "Exception caught in ConnectionHandler::shutdown(): "
    //                      << ex.what() << std::endl;
    //}
}

void
ConnectionHandler::loadAllReminders()
{
    auto remindersMap = this->authFromRedis.getAllReminders();
    for (const auto &name_vec : remindersMap) {
        for (const auto &reminder : name_vec.second) {
            auto t = std::make_shared<boost::asio::basic_waitable_timer<std::chrono::system_clock>>(ioService, std::chrono::system_clock::from_time_t(reminder.timeStamp));
            
            auto remindFunction =
                [
                  this, user = name_vec.first,
                  reminderMessage = reminder.message,
                  whichReminder = reminder.id, t
                ](const boost::system::error_code &er)
                    ->void
            {
                if (er) {
                    std::cerr << "Timer error: " << er << std::endl;
                    return;
                }
                if (this->channels.count(this->nick) == 0) {
                    this->joinChannel(this->nick);
                }
                this->channels.at(this->nick)->whisper(reminderMessage, user);
                this->channels.at(this->nick)
                    ->commandsHandler.redisClient.removeReminder(user,
                                                                whichReminder);
            };

            
            t->async_wait(remindFunction);
        }
    }
}

unsigned int
ConnectionHandler::ofChannelCount(const std::string &channel)
{
    std::lock_guard<std::mutex> lock(channelMtx);
    return this->channels.count(channel);
}

void
ConnectionHandler::sanitizeMsg(std::string &msg)
{
    std::shared_lock<std::shared_mutex> lock(blacklistMtx);
    for (const auto &i : this->blacklist) {
        boost::algorithm::ireplace_all(msg, i.first, i.second);
    }
}

bool
ConnectionHandler::isBlacklisted(const std::string &msg)
{
    std::shared_lock<std::shared_mutex> lock(blacklistMtx);
    return this->blacklist.count(msg) == 1;
}

void
ConnectionHandler::addBlacklist(const std::string &search,
                                const std::string &replace)
{
    std::unique_lock<std::shared_mutex> lock(blacklistMtx);
    this->blacklist[search] = replace;
    this->authFromRedis.addBlacklist(search, replace);
}

void
ConnectionHandler::removeBlacklist(const std::string &search)
{
    std::unique_lock<std::shared_mutex> lock(blacklistMtx);
    this->blacklist.erase(search);
    this->authFromRedis.removeBlacklist(search);
}