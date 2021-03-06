#include "redisauth.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

RedisAuth::RedisAuth()
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "RedisAuth error: " << this->context->errstr
                      << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "RedisAuth can't allocate redis context" << std::endl;
        }
        valid = false;
        return;
    }
    valid = true;

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "GET WNMA:auth:oauth"));
    if (reply->type != REDIS_REPLY_STRING) {
        auth = false;
        freeReplyObject(reply);
        return;
    } else {
        this->oauth = std::string(reply->str, reply->len);
        freeReplyObject(reply);
        reply = static_cast<redisReply *>(
            redisCommand(this->context, "GET WNMA:auth:name"));
        if (reply->type != REDIS_REPLY_STRING) {
            auth = false;
            freeReplyObject(reply);
            return;
        } else {
            auth = true;
            this->name = std::string(reply->str, reply->len);
            freeReplyObject(reply);
        }
    }
}

RedisAuth::~RedisAuth()
{
    redisFree(this->context);
}

void
RedisAuth::setOauth(const std::string &oauth)
{
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SET WNMA:auth:oauth %b", oauth.c_str(), oauth.size()));
    freeReplyObject(reply);
}
void
RedisAuth::setName(const std::string &name)
{
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SET WNMA:auth:name %b", name.c_str(), name.size()));
    freeReplyObject(reply);
}

std::string
RedisAuth::getOauth()
{
    return oauth;
}

std::string
RedisAuth::getName()
{
    return name;
}

bool
RedisAuth::isValid()
{
    return valid;
}

bool
RedisAuth::hasAuth()
{
    return auth;
}

std::map<std::string, std::vector<Reminder>>
RedisAuth::getAllReminders()
{
    std::map<std::string, std::vector<Reminder>> reminders;
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGETALL WNMA:reminders"));
    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return reminders;
    }

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (decltype(reply->elements) i = 0; i < reply->elements; i += 2) {
            std::vector<Reminder> vec;
            std::cout << __FILE__ << " " << __LINE__ << std::endl;
            pt::ptree tree;

            std::string user(reply->element[i]->str, reply->element[i]->len);

            std::string jsonString(reply->element[i + 1]->str,
                                   reply->element[i + 1]->len);
            std::stringstream ss(jsonString);
            pt::read_json(ss, tree);

            for (const auto &kv : tree) {
                vec.push_back({kv.first.data(), kv.second.get<int64_t>("when"),
                               kv.second.get<std::string>("what")});
            }
            reminders.insert({user, vec});
        }
    }
    freeReplyObject(reply);
    return reminders;
}

std::vector<std::string>
RedisAuth::getChannels()
{
    std::vector<std::string> channels;
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SMEMBERS WNMA:channels"));
    if (reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return channels;
    }

    for (decltype(reply->elements) i = 0; i < reply->elements; ++i) {
        channels.push_back(
            std::string(reply->element[i]->str, reply->element[i]->len));
    }

    freeReplyObject(reply);
    return channels;
}

void
RedisAuth::addChannel(const std::string &channel)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SADD WNMA:channels %b", channel.c_str(),
                     channel.size()));
    freeReplyObject(reply);
}
void
RedisAuth::removeChannel(const std::string &channel)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SREM WNMA:channels %b", channel.c_str(),
                     channel.size()));
    freeReplyObject(reply);
}

std::map<std::string, std::string>
RedisAuth::getBlacklist()
{
    std::map<std::string, std::string> blacklist;
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGETALL WNMA:blacklist"));
    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return blacklist;
    }

    if (reply->type == REDIS_REPLY_ARRAY) {
        for (decltype(reply->elements) i = 0; i < reply->elements; i += 2) {
            std::string search(reply->element[i]->str, reply->element[i]->len);
            std::string replace(reply->element[i + 1]->str,
                                reply->element[i + 1]->len);

            blacklist.emplace(std::make_pair(search, replace));
        }
    }
    freeReplyObject(reply);
    return blacklist;
}

void
RedisAuth::addBlacklist(const std::string &search, const std::string &replace)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HSET WNMA:blacklist %b %b", search.c_str(),
                     search.size(), replace.c_str(), replace.size()));
    freeReplyObject(reply);
}

void
RedisAuth::removeBlacklist(const std::string &search)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HDEL WNMA:blacklist %b", search.c_str(),
                     search.size()));
    freeReplyObject(reply);
}
