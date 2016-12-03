#include "connhandler.hpp"
#include "channel.hpp"

ConnHandler::ConnHandler(const std::string &pss, const std::string &nck)
    : pass(pss)
    , nick(nck)
    , quit(false)
{
    boost::asio::ip::tcp::resolver resolver(io_s);
    boost::asio::ip::tcp::resolver::query query("irc.chat.twitch.tv", "6667");
    twitch_it = resolver.resolve(query);

    auto lambda = [this] {
        if (!(this->quit)) {
            return;
        }
        for (auto &i : currentChannels) {
            if (i.second->messageCount > 0) {
                --i.second->messageCount;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    };

    msgDecreaser = std::thread(lambda);
}

ConnHandler::~ConnHandler()
{
    std::cout << "destructing" << std::endl;
    msgDecreaser.join();
    std::cout << "joined" << std::endl;
    currentChannels.clear();
    std::cout << "cleared end destr" << std::endl;
}

void
ConnHandler::joinChannel(const std::string &chn)
{
    std::lock_guard<std::mutex> lk(mtx);

    if (currentChannels.count(chn) == 1)
        return;
    currentChannels.emplace(
        chn, std::make_unique<Channel>(chn, eventQueue, io_s, this));
}

void
ConnHandler::leaveChannel(const std::string &chn)
{
    std::lock_guard<std::mutex> lk(mtx);
    if (currentChannels.count(chn) == 0)
        return;
    currentChannels.erase(chn);
}

void
ConnHandler::run()
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
                    std::string channel = oneline.substr(
                        pos,
                        oneline.find(":", oneline.find("PRIVMSG #")) - pos - 1);
                    std::string ht_chn = "#" + channel;
                    std::string msg = oneline.substr(
                        oneline.find(":", oneline.find(ht_chn)) + 1,
                        std::string::npos);
                    std::string user = oneline.substr(
                        oneline.find(":") + 1,
                        oneline.find("!") - oneline.find(":") - 1);
                    {
                        if (user == "hemirt" && msg == "!quit") {
                            std::cout << "quiting irc" << std::endl;
                            this->quit = true;
                        }
                        handleCommands(user, channel, msg);
                    }
                } else if (oneline.find("PING") != std::string::npos) {
                    if (this->currentChannels.count(chn) == 1) {
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
ConnHandler::sendMsg(const std::string &channel, const std::string &message)
{
    std::lock_guard<std::mutex> lk(mtx);
    if (currentChannels.count(channel) != 1)
        return;
    currentChannels[channel]->sendMsg(message);
}

void
ConnHandler::handleCommands(std::string &user, const std::string &channel,
                            std::string &msg)
{
    if (user == "hemirt")
        sendMsg(channel, "EleGiggle");
    if (user == "hemirt" && msg == "!dung")
        quit = true;
    if (user == "hemirt" && msg == "!deng")
        leaveChannel(channel);
}
