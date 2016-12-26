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
RedisClient::addCommand(const std::vector<std::string> &message)
{
    pt::ptree tree;
    tree.put("default.response", "ZULUL TriEasy");

    std::stringstream ss;
    pt::write_json(ss, tree, false);

    std::string commandJson = ss.str();

    std::cout << "commandjson: " << commandJson << std::endl;

    // redisReply *reply = static_cast<redisReply *>(
    //    redisCommand(this->context, "HSET WNMA:commands:%b %b",
    //                 message[0].c_str(), message[0].size(),
    //                 commandJson.c_str(), commandJson.size()));

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SET WNMA:commands:%s %b", "ZULUL",
                     commandJson.c_str(), commandJson.size()));
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
        this->reconnect();
        return tree;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        return tree;
    }

    std::string jsonString(reply->str, reply->len);
    std::stringstream ss(jsonString);

    pt::read_json(ss, tree);

    return tree;
}
