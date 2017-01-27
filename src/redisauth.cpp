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
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGETALL WNMA:reminders"));
    if (reply->type == REDIS_REPLY_NIL) {
        return reminders;
    }
    
    if (reply->type == REDIS_REPLY_ARRAY) {
        for(int i = 0; i < reply->elements; i += 2) {
            std::vector<Reminder> vec;
            pt::ptree tree;
            
            std::string user(reply->element[i]->str, reply->element[i]->len);
            
            std::string jsonString(reply->element[i+1]->str, reply->element[i+1]->len);
            std::stringstream ss(jsonString);
            pt::read_json(ss, tree);
            
            for(const auto& kv : tree) {
                vec.push_back({kv.first.data(), kv.second.get<int64_t>("when"), kv.second.get<std::string>("what")});
            }
            reminders.insert({user, vec});
        }
    }
    return reminders;
}










