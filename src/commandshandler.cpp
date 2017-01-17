#include "commandshandler.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
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

CommandsHandler::CommandsHandler(boost::asio::io_service &_ioService)
    : ioService(_ioService)
{
}

CommandsHandler::~CommandsHandler()
{
}

Response
CommandsHandler::handle(const IRCMessage &message)
{
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, message.params,
                            boost::algorithm::is_space(),
                            boost::token_compress_on);

    changeToLower(tokens[0]);

    if (tokens[0] == "!raweditcmd") {
        return this->rawEditCommand(message, tokens);
    } else if (tokens[0] == "!addcmd") {
        return this->addCommand(message, tokens);
    } else if (tokens[0] == "!editcmd") {
        return this->editCommand(message, tokens);
    } else if (tokens[0] == "!deletecmd") {
        return this->deleteCommand(message, tokens);
    } else if (tokens[0] == "!deletefullcommand") {
        return this->deleteFullCommand(message, tokens);
    }

    pt::ptree commandTree = redisClient.getCommandTree(tokens[0]);

    // channel user command
    boost::optional<std::string> responseString =
        commandTree.get_optional<std::string>("channels." + message.channel +
                                              "." + message.user + ".response");
    if (responseString) {
        return this->makeResponse(message, *responseString, tokens, commandTree,
                     "channels." + message.channel + "." + message.user);
    }

    // global user command
    responseString =
        commandTree.get_optional<std::string>(message.user + ".response");
    if (responseString) {
        return this->makeResponse(message, *responseString, tokens, commandTree, message.user);
    }

    // default channel command
    responseString = commandTree.get_optional<std::string>(
        "channels." + message.channel + ".default.response");
    if (responseString) {
        return this->makeResponse(message, *responseString, tokens, commandTree, 
                     "channels." + message.channel + ".default");
    }

    // default global command
    responseString = commandTree.get_optional<std::string>("default.response");
    if (responseString) {
        return this->makeResponse(message, *responseString, tokens, commandTree, "default");
    }

    return Response();
}

Response
CommandsHandler::makeResponse(const IRCMessage &message,
                              std::string &responseString,
                              std::vector<std::string> &tokens,
                              const boost::property_tree::ptree &commandTree,
                              const std::string &path)
{
    Response response;
    boost::optional<int> cooldown =
        commandTree.get_optional<int>(path + ".cooldown");
    if (cooldown || *cooldown != 0) {
        auto it = this->cooldownsMap.find(path);
        auto now = std::chrono::steady_clock::now();
        if (it != cooldownsMap.end()) {
            if (std::chrono::duration_cast<std::chrono::seconds>(now -
                                                                 it->second)
                    .count() < *cooldown) {
                return response;
            }
        }
        cooldownsMap[path] = now;
    }

    boost::optional<int> numParams =
        commandTree.get_optional<int>(path + ".numParams");
    if (numParams) {
        if (tokens.size() <= *numParams) {
            return response;
        }

        std::stringstream ss;
        for (int i = 1; i <= *numParams; ++i) {
            ss.str(std::string());
            ss.clear();
            ss << '{' << i << '}';
            if (boost::algorithm::find_regex(
                    tokens[i], boost::regex("{[0-9]+}|{user}|{channel}"))) {
                return response;
            }
            boost::algorithm::replace_all(responseString, ss.str(),
                                          tokens[i]);
        }
    }
    boost::algorithm::replace_all(responseString, "{user}", message.user);
    boost::algorithm::replace_all(responseString, "{channel}",
                                  message.channel);
    response.message = responseString;
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::addCommand(const IRCMessage &message,
                            std::vector<std::string> &tokens)
{
    Response response;
    if (!this->isAdmin(message.user)) {
        response.message = message.user + ", you are not an admin NaM";
        response.type = Response::Type::MESSAGE;
        return response;
    }
    if (tokens.size() < 4) {
        response.message = "Invalid use of command NaM";
        response.type = Response::Type::MESSAGE;
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
        response.type = Response::Type::MESSAGE;
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
    response.type = Response::Type::MESSAGE;
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
        response.type = Response::Type::MESSAGE;
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
        response.type = Response::Type::MESSAGE;
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
    response.type = Response::Type::MESSAGE;
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
        response.type = Response::Type::MESSAGE;
        return response;
    }
    changeToLower(tokens[1]);
    changeToLower(tokens[2]);
    changeToLower(tokens[3]);
    // toke[0]   toke[1] toke[2] toke[3]
    // !editcmd trigger default response fkfkfkfkfk kfs kfosd kfods kfods
    // !editcmd trigger default parameters 0
    // !editcmd trigger default cooldown 4

    if (tokens[3].find('.') != std::string::npos) {
        return response;
    }
    std::string pathstring = tokens[2] + '.' + tokens[3];
    pt::ptree::path_type path(pathstring);
    pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);

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
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::deleteCommand(const IRCMessage &message,
                               std::vector<std::string> &tokens)
{
    Response response;
    if (!this->isAdmin(message.user)) {
        response.message = message.user + ", you are not an admin NaM";
        response.type = Response::Type::MESSAGE;
        return response;
    }
    if (tokens.size() < 3) {
        response.message = "Invalid use of command NaM";
        response.type = Response::Type::MESSAGE;
        return response;
    }
    changeToLower(tokens[1]);
    changeToLower(tokens[2]);
    // tokens[0]     1       2
    //! deletecmd trigger default

    pt::ptree commandTree = redisClient.getCommandTree(tokens[1]);

    boost::optional<pt::ptree &> child =
        commandTree.get_child_optional(tokens[2]);
    if (child) {
        child->clear();
        std::vector<std::string> pathtokens;
        boost::algorithm::split(pathtokens, tokens[2],
                                boost::algorithm::is_any_of("."),
                                boost::token_compress_on);
        std::string last;
        while (pathtokens.size()) {
            last = pathtokens.back();
            pathtokens.pop_back();
            auto &node =
                commandTree.get_child(boost::algorithm::join(pathtokens, "."));
            node.erase(last);

            if (!node.empty()) {
                break;
            }
        }
    }

    if (commandTree.empty()) {
        redisClient.deleteFullCommand(tokens[1]);
    } else {
        std::stringstream ss;
        pt::write_json(ss, commandTree, false);
        redisClient.setCommandTree(tokens[1], ss.str());
        ;
    }

    response.message = message.user + " deleted " + tokens[1] + " command at " +
                       tokens[2] + " FeelsBadMan";
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::deleteFullCommand(const IRCMessage &message,
                                   std::vector<std::string> &tokens)
{
    Response response;
    if (!this->isAdmin(message.user)) {
        response.message = message.user + ", you are not an admin NaM";
        response.type = Response::Type::MESSAGE;
        return response;
    }
    if (tokens.size() < 2) {
        response.message = "Invalid use of command NaM";
        response.type = Response::Type::MESSAGE;
        return response;
    }
    // tokens[0]          tokens[1]
    //! deletefullcommand trigger
    changeToLower(tokens[1]);

    redisClient.deleteFullCommand(tokens[1]);
    response.message = message.user + " deleted everything related to the " +
                       tokens[1] + " command BibleThump";
    response.type = Response::Type::MESSAGE;
    return response;
}

bool
CommandsHandler::isAdmin(const std::string &user)
{
    return this->redisClient.isAdmin(user);
}