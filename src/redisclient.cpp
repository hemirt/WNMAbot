#include "redisclient.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>

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
RedisClient::isAdmin(const std::string &user)
{
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SISMEMBER WNMA:admins %b", user.c_str(), user.size()));
    if (reply == NULL && this->context->err) {
        std::cerr << "RedisClient error: " << this->context->errstr
                  << std::endl;
        freeReplyObject(reply);
        this->reconnect();
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

boost::property_tree::ptree
RedisClient::getRemindersOfUser(const std::string &user)
{
    pt::ptree tree;
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET WNMA:reminders %b", user.c_str(), user.size()));
    if (reply == NULL && this->context->err) {
        std::cerr << "RedisClient error getRemindersOfUser(" << user
                  << "): " << this->context->errstr << std::endl;
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

std::string
RedisClient::addReminder(const std::string &user, int seconds,
                         const std::string &reminder)
{
    std::string value;
    pt::ptree reminderTree = this->getRemindersOfUser(user);

    auto now = std::chrono::time_point_cast<std::chrono::seconds>(
        std::chrono::system_clock::now());

    int i = 0;
    do {
        value = std::to_string(i);
        ++i;
    } while (reminderTree.count(value) != 0);

    std::chrono::seconds timeToAdd(seconds);

    decltype(now) when = now + std::chrono::seconds{seconds};

    reminderTree.put(value + ".when",
                     std::to_string(when.time_since_epoch().count()));
    reminderTree.put(value + ".what", reminder);

    std::stringstream ss;
    pt::write_json(ss, reminderTree, false);

    this->setReminder(user, ss.str());

    return value;
}

void
RedisClient::removeReminder(const std::string &user, const std::string &which)
{
    pt::ptree reminderTree = this->getRemindersOfUser(user);
    if (reminderTree.empty()) {
        return;
    }

    reminderTree.erase(which);
    if (reminderTree.empty()) {
        this->deleteAllReminders(user);
        return;
    }

    std::stringstream ss;
    pt::write_json(ss, reminderTree, false);

    this->setReminder(user, ss.str());
}

void
RedisClient::setReminder(const std::string &user, const std::string &json)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HSET WNMA:reminders %b %b", user.c_str(),
                     user.size(), json.c_str(), json.size()));
    freeReplyObject(reply);
}

void
RedisClient::deleteAllReminders(const std::string &user)
{
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HDEL WNMA:reminders %b", user.c_str(), user.size()));
    freeReplyObject(reply);
}

std::unique_ptr<redisReply, decltype(&freeReplyObject)>
RedisClient::rawCommand(const std::string &cmd)
{
    std::unique_ptr<redisReply, decltype(&freeReplyObject)> ptr(
        static_cast<redisReply *>(
            redisCommand(this->context, "%b", cmd.c_str(), cmd.size())),
        &freeReplyObject);
    return ptr;
}