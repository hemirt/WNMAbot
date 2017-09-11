#ifndef COMEBACKMSG_HPP
#define COMEBACKMSG_HPP

#include <hiredis/hiredis.h>

#include <chrono>
#include <map>
#include <mutex>
#include <string>

#include "network.hpp"  //asio
#include "redisclient.hpp"
#include "messenger.hpp"

class ConnectionHandler;

class ComeBackMsg
{
public:
    struct Msg {
        std::string from;
        std::chrono::time_point<std::chrono::system_clock> when;
        std::string msg;
    };
    ComeBackMsg(boost::asio::io_service &_ioService, ConnectionHandler *_ch);
    ~ComeBackMsg();
    int makeComeBackMsg(const std::string &from, const std::string &to,
                         const std::string &msg, bool isAdmin);
    void sendMsgs(const std::string &user, Messenger &messenger);

    ConnectionHandler *owner;

private:
    struct User {
        std::string from;
        std::string to;
    };
    void removeVecUser(const std::string &from, const std::string &to);
    void addVecUser(const std::string &from, const std::string &to);
    int howMuchVec(const std::string &from, const std::string &to);
    int howMuchForVec(const std::string &to);
    RedisClient redisClient;
    boost::asio::io_service &ioService;
    std::multimap<std::string, Msg> cbMsgs;
    std::mutex mapMtx;
    std::vector<User> maxVec;
    std::mutex vecMtx;
    
    // redis
    redisContext *context;
    void setComeBackMsgRedis(const std::string &to, const Msg& msg);
};

#endif
