#include "redisclient.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

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
        this->valid = false;
    } else {
        this->valid = true;
    }
}

RedisClient::~RedisClient()
{
    if (!this->valid)
        return;
    redisFree(this->context);
}

void
RedisClient::reconnect()
{
    if (this->valid) {
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
        this->valid = false;
    } else {
        this->valid = true;
    }
}

std::map<std::string, std::string>
RedisClient::getCommand(const std::string &channel, const std::string &user,
                        const std::string &command)
{
    std::unordered_map<std::string, std::string> commandMap;
    
    // command + ":" + user
    // command + ":default"
    // command + ":admin"
    // WNMA:channel:commands cmd1:default { cmd } cmd1:hemirt { cmd }
    //                       cmd1:default:admin { cmd }
    
    // todo
    // check admins, admin commands first
    
    redisReply *reply;
    reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:%s:commands %b:%s", channel.c_str(), command.c_str(), command.size(), user.c_str()));
    // stuff
    // return commandMap on match
    // if(!nil) process, free and return;

    reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:%s:commands %b:default", channel.c_str(), command.c_str(), command.size()));
    // stuff
    // return commandMap on match
    // if(!nil) process, free and return;
    
    reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:global:commands %b:%s", command.c_str(), command.size(), user.c_str()));
    // stuff
    // return commandMap on match
    // if(!nil) process, free and return;
    
    reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:global:commands %b:default", command.c_str(), command.size()));
    // stuff
    // return commandMap on match
    // if(!nil) process, free and return;
    
    // return default value, no command
    return commandMap;
}

void
RedisClient::addCommand(const std::string &channel, const std::string &user,
                        const std::string &command, const std::string &rest)
{
    namespace pt = boost::property_tree;
    pt::ptree tree;
    tree.put("response", "ZULUL TriEasy");

    std::stringstream ss;
    pt::write_json(ss, tree, false);

    std::string commandJson = ss.str();

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HSET WNMA:%s:commands %b:%s %b",
                     channel.c_str(), command.c_str(), command.size(), user.c_str(),
                     commandJson.c_str(), commandJson.size()));
}