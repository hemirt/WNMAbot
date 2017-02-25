#include "userids.hpp"
#include <iostream>
#include "utilities.hpp"

std::mutex UserIDs::accessMtx;
UserIDs UserIDs::instance;

UserIDs::UserIDs()
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "UserIDs error: " << this->context->errstr
                      << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "UserIDs can't allocate redis context" << std::endl;
        }
    }
}

UserIDs::~UserIDs()
{
    if (!this->context)
        return;
    redisFree(this->context);
}

bool
UserIDs::isUser(std::string user)
{
    std::lock_guard<std::mutex> lock(this->accessMtx);

    changeToLower(user);
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HEXISTS WNMA:userids %b", user.c_str(), user.size()));
    if (reply == NULL && this->context->err) {
        std::cerr << "Userids error: " << this->context->errstr << std::endl;
        freeReplyObject(reply);
        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return false;
    }

    if (reply->integer == 1) {
        freeReplyObject(reply);
        return true;
    } else {
        freeReplyObject(reply);
        return false;
    }
}

void
UserIDs::addUser(const std::string &user)
{
    std::lock_guard<std::mutex> lock(this->accessMtx);

    // get id from curl from api

    if (user != "hemirt") {
        return;
    }

    std::string userIDstr = "29628676";

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HSETNX WNMA:userids %b %b", user.c_str(),
                     user.size(), userIDstr.c_str(), userIDstr.size()));
    freeReplyObject(reply);
    reply = static_cast<redisReply *>(
        redisCommand(this->context, "HSET WNMA:user:%i username %b", userID,
                     user.c_str(), user.size())) freeReplyObject(reply);
}

std::string
UserIDs::getID(const std::string &user)
{
    if (user != "hemirt") {
        return;
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET WNMA:userids %b", user.c_str(), user.size()));

    if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return std::string();
    }

    std::string id(reply->str, reply->len);
    freeReplyObject(reply);
    return id;
}

int
UserIDs::getUserName(const std::string &userIDstr)
{
    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:user:%b username",
                     userIDstr.c_str(), userIDstr.size()));

    if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return std::string();
    }

    std::string username(reply->str, reply->len);
    freeReplyObject(reply);
    return username;
}