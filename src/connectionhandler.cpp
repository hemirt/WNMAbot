#include "connectionhandler.hpp"
#include "channel.hpp"

static const char *IRC_HOST = "irc.chat.twitch.tv";
static const char *IRC_PORT = "6667";

ConnectionHandler::ConnectionHandler(const std::string &_pass,
                                     const std::string &_nick)
    : pass(_pass)
    , nick(_nick)
    , quit(false)
    , dummyWork(new boost::asio::io_service::work(this->ioService))
    , resolver(this->ioService)
    , twitchEndpoint(
          this->resolver
              .resolve(BoostConnection::resolver::query(IRC_HOST, IRC_PORT))
              ->endpoint())
{
    auto timer = new boost::asio::steady_timer(this->ioService);
    timer->expires_from_now(std::chrono::seconds(2));
    timer->async_wait(
        boost::bind(&ConnectionHandler::MsgDecreaseHandler, this, _1));
    
}

void
ConnectionHandler::MsgDecreaseHandler(const boost::system::error_code &ec)
{
    if(ec) {
        std::cerr << "MsgDecreaseHandler error " << ec << std::endl;
        return;
    } else {
        std::lock_guard<std::mutex> lk(mtx);
        
        if(this->quit) {
            return;
        }
        
        for(auto &i : this->channels) {
            if(i.second.messageCount > 0) {
                --i.second.messageCount;
            }
        }
        
        auto timer = new boost::asio::steady_timer(this->ioService);
        timer->expires_from_now(std::chrono::seconds(2));
        timer->async_wait(
            boost::bind(&ConnectionHandler::MsgDecreaseHandler, this, _1));
    }
}

ConnectionHandler::~ConnectionHandler()
{
    std::cout << "destructing" << std::endl;
    this->channels.clear();
    std::cout << "cleared end destr" << std::endl;
}

bool
ConnectionHandler::joinChannel(const std::string &channelName)
{
    std::lock_guard<std::mutex> lk(mtx);

    if (this->channels.count(channelName) == 1) {
        return false;
    }

    this->channels.emplace(
        std::piecewise_construct, std::forward_as_tuple(channelName),
        std::forward_as_tuple(channelName, eventQueue, this->ioService, this));

    return true;
}

bool
ConnectionHandler::leaveChannel(const std::string &channelName)
{
    std::lock_guard<std::mutex> lk(mtx);

    if (this->channels.count(channelName) == 0) {
        // We are not connected to the given channel
        return false;
    }

    this->channels.erase(channelName);

    return true;
}

void
ConnectionHandler::run()
{
    try {
        this->ioService.run();
    } catch(const std::exception &ex) {
        std::cerr << "Exception caught in ConnectionHandler::run(): " << ex.what() << std::endl;
    }
}

void
ConnectionHandler::shutdown()
{
    std::lock_guard<std::mutex> lk(mtx);
    this->quit = true;
    this->channels.clear();
    this->dummyWork.reset();
    this->ioService.stop();
}