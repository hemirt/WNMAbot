#include "commandshandler.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional/optional.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>
#include <vector>
#include "utilities.hpp"

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

    changeToLower(tokens[0]);

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

    // formats the response according to parameters
    auto makeResponse = [&response, &commandTree, &tokens, &message](
        std::string &responseString, const std::string &path) -> void {
        // TODO: cooldown
        boost::optional<int> numParams =
            commandTree.get_optional<int>(path + ".numParams");
        if (numParams) {
            if (tokens.size() <= *numParams) {
                return;
            }

            std::stringstream ss;
            for (int i = 1; i <= *numParams; ++i) {
                ss.str(std::string());
                ss.clear();
                ss << '{' << i << '}';
                if (boost::algorithm::find_regex(
                        tokens[i], boost::regex("{[0-9]+}|{user}|{channel}"))) {
                    return;
                }
                boost::algorithm::replace_all(responseString, ss.str(),
                                              tokens[i]);
            }
        }
        boost::algorithm::replace_all(responseString, "{user}", message.user);
        boost::algorithm::replace_all(responseString, "{channel}",
                                      message.channel);
        response.message = responseString;
        response.valid = true;
    };

    // channel user command
    boost::optional<std::string> responseString =
        commandTree.get_optional<std::string>("channels." + message.channel +
                                              "." + message.user + ".response");
    if (responseString) {
        makeResponse(*responseString,
                     "channels." + message.channel + "." + message.user);
        return response;
    }

    // global user command
    responseString =
        commandTree.get_optional<std::string>(message.user + ".response");
    if (responseString) {
        makeResponse(*responseString, message.user);
        return response;
    }

    // default channel command
    responseString = commandTree.get_optional<std::string>(
        "channels." + message.channel + ".default.response");
    if (responseString) {
        makeResponse(*responseString,
                     "channels." + message.channel + ".default");
        return response;
    }

    // default global command
    responseString = commandTree.get_optional<std::string>("default.response");
    if (responseString) {
        makeResponse(*responseString, "default");
        return response;
    }

    return response;
}

Response
CommandsHandler::addCommand(const IRCMessage &message,
                            std::vector<std::string> &tokens)
{
    Response response;
    if (!this->isAdmin(message.user)) {
        response.message = message.user + ", you are not an admin NaM";
        response.valid = true;
        return response;
    }
    if (tokens.size() < 4) {
        response.message = "Invalid use of command NaM";
        response.valid = true;
        return response;
    }

    changeToLower(tokens[1]);
    changeToLower(tokens[2]);

    // toke[0] toke[1] toke[2] toke[3]
    // !addcmd trigger default fkfkfkfkfk kfs kfosd kfods kfods

    std::string valueString;
    for (size_t i = 3; i < tokens.size(); ++i) {
        valueString += tokens[i] + ' ';
    }
    valueString.pop_back();

    pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);

    boost::optional<pt::ptree &> child =
        commandTree.get_child_optional(tokens[2]);
    if (child) {
        response.message =
            "Command " + tokens[1] + " at " + tokens[2] + " already exists.";
        response.valid = true;
        return response;
    }

    std::vector<int> vecNumParams{0};  // vector of {number} numbers,
    for (size_t i = 3; i < tokens.size(); ++i) {
        if (tokens[i].front() != '{' || tokens[i].back() != '}') {
            continue;
        }
        if (std::all_of(tokens[i].begin() + 1, tokens[i].end() - 1,
                        ::isdigit)) {
            vecNumParams.push_back(boost::lexical_cast<int>(
                std::string(tokens[i].begin() + 1, tokens[i].end() - 1)));
        }
    }
    int numParams = *std::max_element(vecNumParams.begin(), vecNumParams.end());

    commandTree.put(tokens[2] + ".response", valueString);
    commandTree.put(tokens[2] + ".numParams", numParams);
    commandTree.put(tokens[2] + ".cooldown", 0);

    std::stringstream ss;
    pt::write_json(ss, commandTree, false);

    redisClient.setCommandTree(tokens[1], ss.str());

    response.message = message.user + " added command " + tokens[1] + " at " +
                       tokens[2] + "  SeemsGood";
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

    if (tokens.size() < 4) {
        response.message = "Invalid use of command NaM";
        response.valid = true;
        return response;
    }

    changeToLower(tokens[1]);
    changeToLower(tokens[2]);

    // toke[0] toke[1] toke[2] toke[3]
    // !editcmd trigger default fkfkfkfkfk kfs kfosd kfods kfods

    std::string valueString;
    for (size_t i = 3; i < tokens.size(); ++i) {
        valueString += tokens[i] + ' ';
    }
    valueString.pop_back();

    pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);

    boost::optional<pt::ptree &> child =
        commandTree.get_child_optional(tokens[2]);
    if (!child) {
        response.message =
            "Command " + tokens[1] + " at " + tokens[2] + " doesn\'t exists.";
        response.valid = true;
        return response;
    }

    std::vector<int> vecNumParams{0};  // vector of {number} numbers,
    for (size_t i = 3; i < tokens.size(); ++i) {
        if (tokens[i].front() != '{' || tokens[i].back() != '}') {
            continue;
        }
        if (std::all_of(tokens[i].begin() + 1, tokens[i].end() - 1,
                        ::isdigit)) {
            vecNumParams.push_back(boost::lexical_cast<int>(
                std::string(tokens[i].begin() + 1, tokens[i].end() - 1)));
        }
    }
    int numParams = *std::max_element(vecNumParams.begin(), vecNumParams.end());

    commandTree.put(tokens[2] + ".response", valueString);
    commandTree.put(tokens[2] + ".numParams", numParams);

    std::stringstream ss;
    pt::write_json(ss, commandTree, false);

    redisClient.setCommandTree(tokens[1], ss.str());

    response.message = message.user + " edited command " + tokens[1] + " at " +
                       tokens[2] + "  SeemsGood";
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
    if (tokens.size() < 5) {
        response.message = "Invalid use of command NaM";
        response.valid = true;
        return response;
    }
    changeToLower(tokens[1]);
    changeToLower(tokens[2]);
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