#include "afkers.hpp"
#include <stdint.h>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

namespace pt = boost::property_tree;

Afkers::Afkers()
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "Afkers error: " << this->context->errstr << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "Afkers can't allocate redis context" << std::endl;
        }
    }
    this->getAllAfkersRedis();
}

Afkers::~Afkers()
{
    if (!this->context)
        return;
    redisFree(this->context);
}

bool
Afkers::isAfker(const std::string &user)
{
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    return this->afkersMap.count(user) == 1;
}

void
Afkers::setAfker(const std::string &user, const std::string &message)
{
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    const Afk afk = {true, message, std::chrono::steady_clock::now()};
    this->afkersMap[user] = afk;
    this->setAfkerRedis(user, afk);
}

void
Afkers::updateAfker(const std::string &user, const Afkers::Afk &afk)
{
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    this->afkersMap[user] = afk;
    this->setAfkerRedis(user, afk);
}

void
Afkers::removeAfker(const std::string &user)
{
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    this->afkersMap.erase(user);
    this->removeAfkerRedis(user);
}

Afkers::Afk
Afkers::getAfker(const std::string &user)
{
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    if (this->afkersMap.count(user) == 0) {
        return Afkers::Afk();
    }
    return this->afkersMap.at(user);
}

std::string
Afkers::getAfkers()
{
    std::string afkers;
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    for (const auto &i : this->afkersMap) {
        afkers += i.first + ", ";
    }

    if (!afkers.empty()) {
        afkers.erase(afkers.end() - 2, afkers.end());
    }

    return afkers;
}

void
Afkers::setAfkerRedis(const std::string &user, const Afk &afk)
{
    pt::ptree tree;
    tree.put("message", afk.message);
    int64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(
                        afk.time.time_since_epoch())
                        .count();
    tree.put("time", epoch);
    std::stringstream ss;
    pt::write_json(ss, tree, false);
    std::string json = ss.str();

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HSET WNMA:afkers %b %b", user.c_str(),
                     user.size(), json.c_str(), json.size()));
    freeReplyObject(reply);
}

void
Afkers::removeAfkerRedis(const std::string &user)
{
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HDEL WNMA:afkers %b", user.c_str(), user.size()));
    freeReplyObject(reply);
}

void
Afkers::getAllAfkersRedis()
{
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGETALL WNMA:afkers"));

    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return;
    }

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (int i = 0; i < reply->elements; i += 2) {
            pt::ptree tree;

            std::string user(reply->element[i]->str, reply->element[i]->len);

            std::string jsonString(reply->element[i + 1]->str,
                                   reply->element[i + 1]->len);
            std::stringstream ss(jsonString);
            pt::read_json(ss, tree);

            int64_t time = tree.get<int64_t>("time", -1);
            if (time == -1) {
                continue;
            }
            std::string message = tree.get("message", "");

            auto dur = std::chrono::seconds(time);
            auto timepoint =
                std::chrono::time_point<std::chrono::steady_clock>(dur);

            this->afkersMap[user] = {true, message, timepoint};
        }
    }
    freeReplyObject(reply);
}