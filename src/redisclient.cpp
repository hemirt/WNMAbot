#include "redisclient.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

RedisClient::RedisClient()
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "RedisClient error: " << this->context->errstr
                      << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "RedisClient can't allocate redis context"
                      << std::endl;
        }
    }
}

RedisClient::~RedisClient()
{
    if (!this->context)
        return;
    redisFree(this->context);
}

void
RedisClient::reconnect()
{
    if (this->context) {
        redisFree(this->context);
    }

    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "RedisClient error: " << this->context->errstr
                      << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "RedisClient can't allocate redis context"
                      << std::endl;
        }
    }
}

void
RedisClient::setCommandTree(const std::string &trigger, const std::string &json)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SET WNMA:commands:%b %b", trigger.c_str(),
                     trigger.size(), json.c_str(), json.size()));
    freeReplyObject(reply);
}

void
RedisClient::deleteFullCommand(const std::string &trigger)
{
    this->deleteRedisKey("WNMA:commands:" + trigger);
}

void
RedisClient::deleteRedisKey(const std::string &key)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "DEL %b", key.c_str(), key.size()));
    freeReplyObject(reply);
}

boost::property_tree::ptree
RedisClient::getCommandTree(const std::string &trigger)
{
    pt::ptree tree;
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "GET WNMA:commands:%b", trigger.c_str(),
                     trigger.size()));

    if (reply == NULL && this->context->err) {
        std::cerr << "RedisClient error: " << this->context->errstr
                  << std::endl;
        freeReplyObject(reply);
        this->reconnect();
        return tree;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return tree;
    }

    std::string jsonString(reply->str, reply->len);
    std::stringstream ss(jsonString);

    pt::read_json(ss, tree);

    freeReplyObject(reply);
    return tree;
}

bool
RedisClient::addReminder(int timestamp, const std::string &user, int seconds,
                         const std::string &reminder)
{
    std::string timestring = std::to_string(timestamp);
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SADD WNMA:reminderSet %b:%s", user.c_str(),
                     user.size(), timestring.c_str()));

    if (reply == NULL && this->context->err) {
        std::cerr << "RedisClient error: " << this->context->errstr
                  << std::endl;
        freeReplyObject(reply);
        this->reconnect();
        return false;
    }

    freeReplyObject(reply);
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "SET WNMA:reminders:%b:%s %b", user.c_str(), user.size(),
        timestring.c_str(), reminder.c_str(), reminder.size()));

    if (reply == NULL && this->context->err) {
        std::cerr << "RedisClient error: " << this->context->errstr
                  << std::endl;
        freeReplyObject(reply);
        this->reconnect();
        return false;
    }

    freeReplyObject(reply);
    std::string secondsString = std::to_string(seconds);
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "EXPIRE WNMA:reminders:%b:%s %s", user.c_str(),
        user.size(), timestring.c_str(), secondsString.c_str()));

    if (reply == NULL && this->context->err) {
        std::cerr << "RedisClient error: " << this->context->errstr
                  << std::endl;
        freeReplyObject(reply);
        this->reconnect();
        return false;
    }

    freeReplyObject(reply);
    return true;
}