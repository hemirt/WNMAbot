#include "commandshandler.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <vector>

CommandsHandler::CommandsHandler()
{
}

CommandsHandler::~CommandsHandler()
{
}

Response
CommandsHandler::handle(const IRCMessage &message)
{
    Response response;

    std::vector<std::string> vector;
    boost::algorithm::split(vector, message.params,
                            boost::algorithm::is_space(),
                            boost::token_compress_on);

    redisClient.addCommand(vector);
    auto commandTree = redisClient.getCommandTree(vector[0]);

    std::string responseString =
        commandTree.get<std::string>("default.response", "NULL");
    if (responseString == "NULL")
        return response;

    response.message = responseString;
    response.valid = true;  // temp
    return response;
}

Response
CommandsHandler::addCommand()
{
}

bool
CommandsHandler::isAdmin(const std::string &user)
{
    // redisReply *reply = static_cast<redisReply *>(
    // redisCommand(this->redisC, "SISMEMBER WNMA:admins %s", user.c_str()));
    // if (!reply) {
    // return false;
    // }

    // if (reply->type != REDIS_REPLY_INTEGER) {
    // freeReplyObject(reply);
    // return false;
    // }

    // if (reply->integer == 1) {
    // freeReplyObject(reply);
    // return true;
    // }

    // else {
    // freeReplyObject(reply);
    // return false;
    // }
    return false;  // temp
}