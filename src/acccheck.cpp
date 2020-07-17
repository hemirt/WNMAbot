#include "acccheck.hpp"

void AccCheck::init()
{
    AccCheck::curl = curl_easy_init();
    if (AccCheck::curl) {
        AccCheck::chunk = curl_slist_append(
            chunk, "Accept: application/vnd.twitchtv.v5+json");
        AccCheck::chunk = curl_slist_append(
            chunk, "Client-ID: i9nh09d5sv612dts3fmrccimhq7yb2");
    } else {
        std::cerr << "ACCCHECK: CURL ERROR" << std::endl;
    }
    
    AccCheck::context = redisConnect("127.0.0.1", 6379);
    if (AccCheck::context == NULL || AccCheck::context->err) {
        if (AccCheck::context) {
            std::cerr << "AccCheck error: " << AccCheck::context->errstr
                      << std::endl;
            redisFree(AccCheck::context);
        } else {
            std::cerr << "AccCheck can't allocate redis context"
                      << std::endl;
        }
    }
}

void AccCheck::deinit()
{
    curl_slist_free_all(AccCheck::chunk);
    curl_easy_cleanup(AccCheck::curl);
    if (AccCheck::context)
        redisFree(AccCheck::context);
}

bool AccCheck::isAllowed(const std::string &user)
{
    std::unique_lock lk(AccCheck::redisMtx);
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        AccCheck::context, "SISMEMBER WNMA:allowedUsers %b", user.c_str(), user.size()));
    if (reply == NULL && AccCheck::context->err) {
        std::cerr << "AccCheck redis error: " << AccCheck::context->errstr
                  << std::endl;
        freeReplyObject(reply);
        lk.unlock();
        AccCheck::reconnect();
        return false;
    }
    
    if (reply == nullptr) {
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

void AccCheck::addAllowed(const std::string &user)
{
    std::lock_guard lk(AccCheck::redisMtx);
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        AccCheck::context, "SADD WNMA:allowedUsers %b", user.c_str(), user.size()));
    freeReplyObject(reply);
}

void AccCheck::delAllowed(const std::string &user)
{
    std::lock_guard lk(AccCheck::redisMtx);
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        AccCheck::context, "SREM WNMA:allowedUsers %b", user.c_str(), user.size()));
    freeReplyObject(reply);
}

void AccCheck::reconnect()
{
    std::lock_guard lk(AccCheck::redisMtx);
    if (AccCheck::context) {
        redisFree(AccCheck::context);
    }

    AccCheck::context = redisConnect("127.0.0.1", 6379);
    if (AccCheck::context == NULL || AccCheck::context->err) {
        if (AccCheck::context) {
            std::cerr << "AccCheck error: " << AccCheck::context->errstr
                      << std::endl;
            redisFree(AccCheck::context);
        } else {
            std::cerr << "AccCheck can't allocate redis context"
                      << std::endl;
        }
    }
}