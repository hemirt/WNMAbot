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
            std::cerr << "UserIDs error: " << this->context->errstr << std::endl;
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
        this->context, "SISMEMBER WNMA:userids %b", user.c_str(), user.size()));
    if (reply == NULL && this->context->err) {
        std::cerr << "SetOfUser error: " << this->context->errstr
                  << std::endl;
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
    
    // does redis has HADD or sth that will add only and not modify as HSET does?
    
    // if exists return
    // else get next id and add user
    
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HSET WNMA:userids %b", user.c_str(), user.size()));
    freeReplyObject(reply);
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "HSET WNMA:user:%i username %b", userID, user.c_str(), user.size()))
    freeReplyObject(reply);
}

int
UserIDs::getID(const std::string &user)
{
    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET WNMA:userids %b", user.c_str(), user.size()));

    if(reply == NULL || reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return -1;
    }

    int id = reply->integer;
    freeReplyObject(reply);
    return id;
}

int
UserIDs::getUserName(int userID)
{
    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET WNMA:user:%i username", userID));

    if(reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return std::string();
    }

    std::string username(reply->str, reply->len);
    freeReplyObject(reply);
    return username;
}