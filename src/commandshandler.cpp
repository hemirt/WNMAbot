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

    // std::string s = "HGETALL WNMA:hemirt:" + vector[0];
    // redisReply *reply =
        // static_cast<redisReply *>(redisCommand(this->redisC, s.c_str()));

    // if (!reply) {
        // return Response();
    // }

    // if (reply->type != REDIS_REPLY_ARRAY) {
        // freeReplyObject(reply);
        // return Response();
    // }

    // for (int j = 0; j < reply->elements; j++) {
        // std::cout << "\"" << reply->element[j]->str << "\"" << std::endl;
        // if (strncmp(reply->element[j]->str, "response",
                    // strlen(reply->element[j]->str)) == 0 &&
            // j + 1 < reply->elements) {
            // std::cout << "dsaijdsaji" << reply->element[j + 1]->str
                      // << std::endl;
            // response.message = reply->element[j + 1]->str;
            // response.valid = true;
        // }
    // }
    redisClient.addCommand(message.channel, "default", "!eleldeg", "");
    auto map = redisClient.getCommand(message.channel, message.user, vector[0]);
    std::cout << "our channel: " << message.channel << " " << vector[0] << std::endl;
    std::cout << "middle: " << message.middle << std::endl << "host: " << message.host << std::endl;
    for(auto i : map)
    {
        std::cout << i.first << " " << i.second << std::endl;
    }
    response.message = map["response"];
    response.valid = true; //temp
    return response;
    // lookup command in db

    // if no command return an empty Response (do nothing)

    // check if message can be formatted into a response
    // ex. if a command takes 2 parameters but only 1 was provided
    // then return empty Response

    // simple text command should end here

    // command that does stuff (add commands, edits..)
    // doStuff();
    // return response

    // once a commandshandler has returned a response back to the channel,
    // channel will print out a response (send the msg)
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
    return false; //temp
}