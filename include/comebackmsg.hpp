#ifndef COMEBACKMSG_HPP
#define COMEBACKMSG_HPP

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include "network.hpp"  //asio
#include "redisclient.hpp"

class ConnectionHandler;

class ComeBackMsg
{
public:
    struct Msg {
        std::string from;
        std::chrono::time_point<std::chrono::steady_clock> when;
        std::string msg;
    };
    ComeBackMsg(boost::asio::io_service &_ioService, ConnectionHandler *_ch);
    void makeComeBackMsg(const std::string &from, const std::string &to,
                         const std::string &msg);
    void sendMsgs(const std::string &user);

    ConnectionHandler *owner;

private:
    RedisClient redisClient;
    boost::asio::io_service &ioService;
    std::multimap<std::string, Msg> cbMsgs;
    std::mutex mapMtx;
};

#endif