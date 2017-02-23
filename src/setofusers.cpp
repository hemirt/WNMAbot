#include "setofusers.hpp"
#include <iostream>
#include "utilities.hpp"

SetOfUsers::SetOfUsers()
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "SetOfUsers error: " << this->context->errstr << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "SetOfUsers can't allocate redis context" << std::endl;
        }
    }
}

SetOfUsers::~SetOfUsers()
{
    if (!this->context)
        return;
    redisFree(this->context);
}

bool
SetOfUsers::isUser(std::string user)
{
    changeToLower(user);
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SISMEMBER WNMA:users %b", user.c_str(), user.size()));
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
SetOfUsers::addUser(const std::string &user)
{
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SADD WNMA:users %b", user.c_str(), user.size()));
    freeReplyObject(reply);
}