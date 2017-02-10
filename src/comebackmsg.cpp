#include "comebackmsg.hpp"
#include <iostream>
#include "utilities.hpp"
#include "connectionhandler.hpp"
#include "channel.hpp"

ComeBackMsg::ComeBackMsg(boost::asio::io_service &_ioService, ConnectionHandler * _ch)
    : ioService(_ioService)
    , owner(_ch)
{
    //get cmsgs from redis
}

void
ComeBackMsg::makeComeBackMsg(const std::string &from, const std::string &to, const std::string &msg)
{
    //add msg to redis
    std::lock_guard<std::mutex> lock(mapMtx);
    this->cbMsgs.emplace(std::make_pair(to, ComeBackMsg::Msg{from, std::chrono::steady_clock::now(), msg}));
}

void
ComeBackMsg::sendMsgs(const std::string &user) 
{
    std::lock_guard<std::mutex> lock(mapMtx);

    auto range = this->cbMsgs.equal_range(user);
    if(range.first == range.second) {
        return;
    }

    int delay = 350;
    for (auto it = range.first; it != range.second; ++it) {
        auto whisperFunction = [to = it->first, MSG = it->second, this](const boost::system::error_code &er) {
            std::string whisperMsg = MSG.from + "("
                + makeTimeString(std::chrono::duration_cast<std::chrono::seconds>
                    (std::chrono::steady_clock::now() - MSG.when).count())
                + " ago): " + MSG.msg;

            if(er) {
                std::cerr << "ComeBackMsg::sendMsgs timer error: " << er << "\nfor: " << to << "\nwhisperMsg: " << whisperMsg << std::endl;
                return;
            }
            if (owner->ofChannelCount(owner->nick) == 0) {
                std::cout << "_2" << std::endl;
                owner->joinChannel(owner->nick);
                std::cout << "_3" << std::endl;
            }
            owner->channels.at(owner->nick).whisper(whisperMsg, to);
        };

        auto t = new boost::asio::steady_timer(ioService, std::chrono::milliseconds(delay));
        t->async_wait(whisperFunction);
        delay += 350;
    }
    this->cbMsgs.erase(range.first, range.second);
}