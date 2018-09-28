#include "time.hpp"

#include <chrono>
#include <iostream>
#include <locale>
#include "date/tz.h"

Time::Time()
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "Users error: " << this->context->errstr << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "Users can't allocate redis context" << std::endl;
        }
    }
}

Time::~Time()
{
    if (!this->context)
        return;
    redisFree(this->context);
}

std::string
Time::getCurrentTime(const std::string &timezone)
{
    if (timezone.empty()) {
        return std::string();
    }
    
    try {
        return date::format("%A, %B %d, %Y at %H:%M:%S %Z, %z", date::make_zoned(timezone, date::floor<std::chrono::seconds>(std::chrono::system_clock::now())));
    } catch (std::exception &e) {
        std::cerr << "getCurrentTime " << timezone << ": " << e.what() << std::endl;
        return std::string();
    }
}

std::string
Time::getTimeZone(const std::string &userid)
{
    if (userid.empty()) {
        return std::string();
    }
    std::lock_guard<std::mutex> lock(this->accessMtx);
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "GET WNMA:time:%b", userid.c_str(), userid.size()));
    if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
        std::cerr << "Redis Error: " << std::string(reply->str, reply->len) << std::endl;
        freeReplyObject(reply);
        return std::string();
    }
    std::string timezone(reply->str, reply->len);
    freeReplyObject(reply);
    return timezone;
}

void
Time::delTimeZone(const std::string &userid)
{
    if (userid.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(this->accessMtx);
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "DEL WNMA:time:%b", userid.c_str(), userid.size()));
    freeReplyObject(reply);
}

std::string
Time::getUsersCurrentTime(const std::string &userid)
{
    return this->getCurrentTime(this->getTimeZone(userid));
}

bool
Time::setTimeZone(const std::string &userid, const std::string &timezone)
{
    try {
        auto zone = date::locate_zone(timezone);
    } catch (std::exception &e) {
        return false;
    }
    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(
            redisCommand(this->context, "SET WNMA:time:%b %b",
                         userid.c_str(), userid.size(),
                         timezone.c_str(), timezone.size()));
    freeReplyObject(reply);
    return true;
}














