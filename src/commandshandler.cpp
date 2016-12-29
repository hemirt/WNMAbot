#include "commandshandler.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/optional/optional.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
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

    if (tokens[0] == "!raweditcmd") {
        return this->rawEditCommand(message, tokens);
    }
    if (tokens[0] == "!addcmd") {
        return this->addCommand(message, tokens);
    }
    if (tokens[0] == "!editcmd") {
        return this->editCommand(message, tokens);
    }

    pt::ptree commandTree = redisClient.getCommandTree(tokens[0]);

    boost::optional<std::string> responseString =
        commandTree.get_optional<std::string>("default.response");
    if (!responseString)
        return response;

    response.message = *responseString;
    response.valid = true;  // temp
    return response;
}

Response
CommandsHandler::addCommand(const IRCMessage &message,
                            std::vector<std::string> &tokens)
{
    Response response;
    if (!this->isAdmin(message.user)) {
        return response;
    }
    
    // toke[0] toke[1] toke[2] toke[3]
    // !addcmd trigger default fkfkfkfkfk kfs kfosd kfods kfods
    

    std::string valueString;
    for (size_t i = 3; i < tokens.size(); ++i) {
        valueString += tokens[i] + ' ';
    }
    valueString.pop_back();
    
    pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);
    
    boost::optional<pt::ptree&> child = commandTree.get_child_optional(tokens[2]);
    if(child) {
        response.message = "Command " + tokens[1] + " at " + tokens[2] + " already exists.";
        response.valid = true;
        return response;
    }
    
    int numParams = 0;
    for(size_t i = 3; i < tokens.size(); ++i) {
        if(tokens[i].front() != '{' || tokens[i].back() != '}') {
            continue;
        }
        if(std::all_of(tokens[i].begin()+1, tokens[i].end()-1, ::isdigit)) {
            ++numParams;
        }
    }
    
    commandTree.put(tokens[2] + ".response", valueString);
    commandTree.put(tokens[2] + ".numParams", numParams);
    commandTree.put(tokens[2] + ".cooldown", 0);
    
    
    std::stringstream ss;
    pt::write_json(ss, commandTree, false);

    redisClient.setCommandTree(tokens[1], ss.str());

    response.message =
        message.user + " added command " + tokens[1] + " at " + tokens[2] + "  SeemsGood";
    response.valid = true;
    return response;
}

Response
CommandsHandler::editCommand(const IRCMessage &message,
                            std::vector<std::string> &tokens)
{
    Response response;
    if (!this->isAdmin(message.user)) {
        return response;
    }
    
    // toke[0] toke[1] toke[2] toke[3]
    // !editcmd trigger default fkfkfkfkfk kfs kfosd kfods kfods
    

    std::string valueString;
    for (size_t i = 3; i < tokens.size(); ++i) {
        valueString += tokens[i] + ' ';
    }
    valueString.pop_back();
    
    pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);
    
    boost::optional<pt::ptree&> child = commandTree.get_child_optional(tokens[2]);
    if(!child) {
        response.message = "Command " + tokens[1] + " at " + tokens[2] + " doesn\'t exists.";
        response.valid = true;
        return response;
    }
    
    int numParams = 0;
    for(size_t i = 3; i < tokens.size(); ++i) {
        if(tokens[i].front() != '{' || tokens[i].back() != '}') {
            continue;
        }
        if(std::all_of(tokens[i].begin()+1, tokens[i].end()-1, ::isdigit)) {
            ++numParams;
        }
    }
    
    commandTree.put(tokens[2] + ".response", valueString);
    commandTree.put(tokens[2] + ".numParams", numParams);
    
    std::stringstream ss;
    pt::write_json(ss, commandTree, false);

    redisClient.setCommandTree(tokens[1], ss.str());

    response.message =
        message.user + " edited command " + tokens[1] + " at " + tokens[2] + "  SeemsGood";
    response.valid = true;
    return response;
}

Response
CommandsHandler::rawEditCommand(const IRCMessage &message,
                             std::vector<std::string> &tokens)
{
    Response response;
    if (!this->isAdmin(message.user)) {
        return response;
    }
    if (tokens.size() < 4) {
        return response;
    }
    // toke[0]   toke[1] toke[2] toke[3]
    // !editcmd trigger default response fkfkfkfkfk kfs kfosd kfods kfods
    // !editcmd trigger default parameters 0
    // !editcmd trigger default cooldown 4
    
    if (tokens[3].find('.') != std::string::npos) {
        return response;
    }
    std::string pathstring = tokens[2] + '.' + tokens[3];
    pt::ptree::path_type path(pathstring);
    pt::ptree commandTree = redisClient.getCommandTree(tokens[0]);
    
    std::string valueString;
    for (size_t i = 4; i < tokens.size(); ++i) {
        valueString += tokens[i] + ' ';
    }
    valueString.pop_back();

    commandTree.put(path, valueString);

    std::stringstream ss;

    pt::write_json(ss, commandTree, false);

    redisClient.setCommandTree(tokens[1], ss.str());

    response.message =
        message.user + " edited " + tokens[1] + " command SeemsGood";
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
    if (user == "hemirt")
        return true;
    return false;  // temp
}