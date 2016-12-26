#include "commandshandler.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <vector>

namespace pt = boost::property_tree;

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

    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, message.params,
                            boost::algorithm::is_space(),
                            boost::token_compress_on);

    
    if(tokens[0] == "!editcomm") {
        return this->editCommand(message, tokens);
    }
    
    pt::ptree commandTree = redisClient.getCommandTree(tokens[0]);

    std::string responseString =
        commandTree.get<std::string>("default.response", "NULL");
    if (responseString == "NULL")
        return response;

    response.message = responseString;
    response.valid = true;  // temp
    return response;
}

Response
CommandsHandler::addCommand(const IRCMessage &message, std::vector<std::string> &tokens)
{
    Response response;
    if(!this->isAdmin(message.user)){
        return response;
    }
    //!editcomm trigger default response 
}

Response
CommandsHandler::editCommand(const IRCMessage &message, std::vector<std::string> &tokens)
{
    Response response;
    if(!this->isAdmin(message.user)){
        return response;
    }
    if(tokens.size() < 4) {
        return response;
    }
    // toke[0]   toke[1] toke[2] toke[3]
    // !editcmd trigger default response fkfkfkfkfk kfs kfosd kfods kfods
    // !editcmd trigger default parameters 0
    // !editcmd trigger default cooldown 4
    pt::ptree commandTree = redisClient.getCommandTree(tokens[0]);
    if(tokens[1].find('.') != std::string::npos || tokens[2].find('.') != std::string::npos)
    {
        return response;
    }
    std::string pathstring = tokens[2] + '.' + tokens[3];
    pt::ptree::path_type path(pathstring);    
    
    std::string responseString;
    for(size_t i = 4; i < tokens.size(); ++i) {
        responseString += tokens[i] + ' ';
    }
    responseString.pop_back();
    
    commandTree.put(path, responseString);
    
    std::stringstream ss;
    
    pt::write_json(ss, commandTree, false);
    
    redisClient.addCommand(tokens[1], ss.str());
    
    response.message = message.user + " edited " + tokens[1] + " command SeemsGood";
    response.valid = true;
    return response;
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
    if(user == "hemirt") return true;
    return false;  // temp
}