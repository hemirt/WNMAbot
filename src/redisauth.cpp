#include "redisauth.hpp"

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
        auth = true;
        this->oauth = std::string(reply->str, reply->len);
        freeReplyObject(reply);
    }

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