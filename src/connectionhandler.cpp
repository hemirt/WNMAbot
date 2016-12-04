#include "connectionhandler.hpp"
#include "channel.hpp"

static const char *IRC_HOST = "irc.chat.twitch.tv";
static const char *IRC_PORT = "6667";

ConnectionHandler::ConnectionHandler(const std::string &_pass,
                                     const std::string &_nick)
    : pass(_pass)
    , nick(_nick)
    , quit(false)
    , dummyWork(this->ioService)
{
    boost::asio::ip::tcp::resolver resolver(this->ioService);
    boost::asio::ip::tcp::resolver::query query(IRC_HOST, IRC_PORT);
    this->twitchResolverIterator = resolver.resolve(query);

    auto lambda = [this] {
        if (!(this->quit)) {
            return;
        }
        for (auto &i : this->channels) {
            if (i.second.messageCount > 0) {
                --i.second.messageCount;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    };

    msgDecreaser = std::thread(lambda);
}

ConnectionHandler::~ConnectionHandler()
{
    std::cout << "destructing" << std::endl;
    msgDecreaser.join();
    std::cout << "joined" << std::endl;
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
    while (!(this->quit)) {
        this->eventQueue.wait();
        while (!(this->eventQueue.empty()) && !(this->quit)) {
            // std::cout << "event" << std::endl;
            auto pair = this->eventQueue.pop();
            std::unique_ptr<boost::asio::streambuf> b(std::move(pair.first));
            std::string chn = pair.second;

            std::istream is(&(*b));
            std::string line(std::istreambuf_iterator<char>(is), {});

            std::string delimiter = "\r\n";
            std::vector<std::string> vek;

            size_t pos = 0;
            while ((pos = line.find(delimiter)) != std::string::npos) {
                vek.push_back(line.substr(0, pos));
                line.erase(0, pos + delimiter.length());
            }

            for (int i = 0; i < vek.size(); ++i) {
                std::string oneline = vek.at(i);

                if (oneline.find("PRIVMSG") != std::string::npos) {
                    size_t pos =
                        oneline.find("PRIVMSG #") + strlen("PRIVMSG #");
                    std::string channelName = oneline.substr(
                        pos,
                        oneline.find(":", oneline.find("PRIVMSG #")) - pos - 1);
                    std::string ht_chn = "#" + channelName;
                    std::string msg = oneline.substr(
                        oneline.find(":", oneline.find(ht_chn)) + 1,
                        std::string::npos);
                    std::string user = oneline.substr(
                        oneline.find(":") + 1,
                        oneline.find("!") - oneline.find(":") - 1);
                    {
                        this->handleMessage(user, channelName, msg);
                    }
                } else if (oneline.find("PING") != std::string::npos) {
                    if (this->channels.count(chn) == 1) {
                        std::cout << "PONGING" << chn << std::endl;
                        std::string pong = "PONG :tmi.twitch.tv\r\n";
                        // this->channelSockets[chn].sock->async_send(boost::asio::buffer(pong),
                        // handler); //define hnadler
                    }
                }
                /*else if(oneline.find("PONG") != std::string::npos)
                {
                        {
                                std::cout << "got ping " << chn << std::endl;
                                std::lock_guard<std::mutex>
                lk(this->pingMap[chn]->mtx);
                                this->pingMap[chn]->pinged = true;
                        }
                        this->pingMap[chn]->cv.notify_all();
                }
                */
            }
        }
    }
}

void
ConnectionHandler::sendMsg(const std::string &channel,
                           const std::string &message)
{
    std::lock_guard<std::mutex> lk(mtx);

    if (this->channels.count(channel) != 1) {
        return;
    }

    this->channels.at(channel).sendMsg(message);
}

bool
ConnectionHandler::handleMessage(const std::string &user,
                                 const std::string &channelName,
                                 const std::string &msg)
{
    auto chanIt = this->channels.find(channelName);
    if (chanIt == this->channels.end()) {
        // Can't resolve channel
        return false;
    }

    return chanIt->second.handleMessage(user, msg);
}
