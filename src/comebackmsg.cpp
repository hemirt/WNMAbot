#include "comebackmsg.hpp"
#include <iostream>
#include "channel.hpp"
#include "connectionhandler.hpp"
#include "utilities.hpp"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/writer.h>
#include <stdint.h>

ComeBackMsg::ComeBackMsg(boost::asio::io_service &_ioService,
                         ConnectionHandler *_ch)
    : owner(_ch)
    , ioService(_ioService)
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "ComeBackMsg error: " << this->context->errstr << std::endl;
            redisFree(this->context);
            return;
        } else {
            std::cerr << "ComeBackMsg can't allocate redis context" << std::endl;
            return;
        }
    }

    {
        redisReply* reply = static_cast<redisReply*>(
            redisCommand(this->context, "SMEMBERS WNMA:comebackmsgs"));
        if (!reply) {
            std::cerr << "ComeBackMsg error nullptr SMEMBERS: " << this->context->errstr << std::endl;
            return;
        }
        if (reply->type == REDIS_REPLY_ARRAY) {
            std::cout << __FILE__ << " " <<__LINE__ << std::endl;
            for (decltype(reply->elements) i = 0; i < reply->elements; ++i) {
                std::string name(reply->element[i]->str, reply->element[i]->len);
                redisReply* reply2 = static_cast<redisReply*>(
                    redisCommand(this->context, "LRANGE WNMA:comebackmsgs:%b 0 -1", name.c_str(), name.size()));
                if (!reply2) {
                    std::cerr << "ComeBackMsg error nullptr LRANGE " << name << ": " << this->context->errstr << std::endl;
                    continue;
                }
                for (decltype(reply2->elements) i = 0; i < reply2->elements; ++i) {
                    std::string json(reply2->element[i]->str, reply2->element[i]->len);
                    rapidjson::Document document;
                    document.Parse(json.c_str());
                    
                    auto it = document.FindMember("when");
                    if (it == document.MemberEnd() || !it->value.IsInt64()) {
                        std::cerr << "when NOT A number" << std::endl;
                        continue;
                    }
                    int64_t time = it->value.GetInt64();
                    
                    auto it2 = document.FindMember("msg");
                    if (it2 == document.MemberEnd() || !it2->value.IsString()) {
                        std::cerr << "msg NOT A string" << std::endl;
                        continue;
                    }
                    std::string msg = it2->value.GetString();
                    
                    auto it3 = document.FindMember("from");
                    if (it3 == document.MemberEnd() || !it3->value.IsString()) {
                        std::cerr << "from NOT A string" << std::endl;
                        continue;
                    }
                    std::string from = it3->value.GetString();

                    const std::chrono::system_clock::duration dur = std::chrono::seconds(time);

                    auto timepoint = std::chrono::time_point<std::chrono::system_clock>(dur);
                    
                    Msg mmsg{from, timepoint, msg};
                    this->cbMsgs.emplace(std::make_pair(name, mmsg));
                    this->addVecUser(from, name);
                }
                freeReplyObject(reply2);
            }
        }
        freeReplyObject(reply);
    }
}

ComeBackMsg::~ComeBackMsg()
{
    if (!this->context)
        return;
    redisFree(this->context);
}

int
ComeBackMsg::makeComeBackMsg(const std::string &from, const std::string &to,
                             const std::string &msg, bool isAdmin = false)
{
    // implement add msg to redis
    if (howMuchVec(from, to) > 2 && !isAdmin) {
        return 1;
    }
    if (howMuchForVec(to) > 5 && !isAdmin) {
        return 2;
    }
    {
        std::lock_guard<std::mutex> lock(mapMtx);
        Msg msgstruct{from, std::chrono::system_clock::now(), msg};
        this->cbMsgs.emplace(std::make_pair(
            to, msgstruct));
        this->setComeBackMsgRedis(to, msgstruct);
    }
    std::lock_guard<std::mutex> lock(vecMtx);
    this->addVecUser(from, to);
    return 0;
}

int
ComeBackMsg::howMuchVec(const std::string &from, const std::string &to)
{
    std::lock_guard<std::mutex> lock(vecMtx);
    return std::count_if(this->maxVec.begin(), this->maxVec.end(), [&](const User &user) {
        return (user.from == from && user.to == to);
    });
}

int
ComeBackMsg::howMuchForVec(const std::string &to)
{
    std::lock_guard<std::mutex> lock(vecMtx);
    return std::count_if(this->maxVec.begin(), this->maxVec.end(), [&](const User &user) {
        return (user.to == to);
    });
}

void
ComeBackMsg::addVecUser(const std::string &from, const std::string &to)
{
    this->maxVec.push_back({from, to});
}

void
ComeBackMsg::removeVecUser(const std::string &from, const std::string &to)
{
    while(true) {
        auto it = std::find_if(this->maxVec.begin(), this->maxVec.end(), [&](const User &user) {
                                return (user.from == from && user.to == to);});
        if (it == this->maxVec.end()) {
            break;
        }
        this->maxVec.erase(it);
        
    }
}

void
ComeBackMsg::sendMsgs(const std::string &user, Messenger &messenger, ConnectionHandler* cnh)
{
    std::lock(mapMtx, vecMtx);
    std::lock_guard<std::mutex> lock(mapMtx, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(vecMtx, std::adopt_lock);
    
    auto range = this->cbMsgs.equal_range(user);
    if (range.first == this->cbMsgs.end()) {
        return;
    }
    if (range.first == range.second) {
        return;
    }


    std::string send = user + ", ";
    for (auto it = range.first; it != range.second; ++it) {
        
        auto &to = it->first;
        auto &MSG = it->second;
        this->removeVecUser(MSG.from, to);
        
        if (!this->redisClient.isAdmin(MSG.from)) {
            cnh->sanitizeMsg(MSG.msg);
        }
        
        send += MSG.from + "(-" + makeTimeString(std::chrono::duration_cast<std::chrono::seconds>(
                                   std::chrono::system_clock::now() - MSG.when)
                                   .count()) + " ch: " + MSG.msg + " ;";
    }
    
    if (!send.empty()) {
        send.pop_back();
        send.pop_back();
    }
    
    auto vec = splitIntoChunks(std::move(send));
    for (auto &i : vec) {
        messenger.push_back(std::move(i));
    }

    this->cbMsgs.erase(range.first, range.second);
    {
        redisReply *reply = static_cast<redisReply *>(
            redisCommand(this->context, "SREM WNMA:comebackmsgs %b", user.c_str(),
                         user.size()));
        freeReplyObject(reply);
        reply = nullptr;
        reply = static_cast<redisReply *>(
            redisCommand(this->context, "DEL WNMA:comebackmsgs:%b", user.c_str(), user.size()));
        freeReplyObject(reply);
    }
}

void
ComeBackMsg::setComeBackMsgRedis(const std::string &to [[maybe_unused]], const Msg &msg)
{
    std::cout << std::endl << msg.from << std::endl << msg.msg << std::endl;
    std::string json;
    {
        rapidjson::Document d;
        d.SetObject();
        auto &alloc = d.GetAllocator();
        //d.AddMember(rapidjson::Value("to", alloc).Move(), rapidjson::Value(to.c_str(), alloc).Move(), alloc); // not really needed
        d.AddMember(rapidjson::Value("from", alloc).Move(), rapidjson::Value(msg.from.c_str(), alloc).Move(), alloc);
        d.AddMember(rapidjson::Value("when", alloc).Move(), std::chrono::duration_cast<std::chrono::seconds>(msg.when.time_since_epoch()).count(), alloc);
        d.AddMember(rapidjson::Value("msg", alloc).Move(), rapidjson::Value(msg.msg.c_str(), alloc).Move(), alloc);
        
        rapidjson::StringBuffer bf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(bf);
        d.Accept(writer);
        
        json = std::string(bf.GetString());
    }
    // {"to":"hemirt","from":"hemirt","when":1499117859,"msg":"Kappa Test Keepo"}
    std::cout << json << std::endl;

    
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SADD WNMA:comebackmsgs %b", to.c_str(),
                     to.size()));
    freeReplyObject(reply);
    reply = nullptr;
    reply = static_cast<redisReply *>(
        redisCommand(this->context, "RPUSH WNMA:comebackmsgs:%b %b", to.c_str(), to.size(), json.c_str(), json.size()));
    freeReplyObject(reply);
}





























