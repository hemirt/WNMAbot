#include "commandshandler.hpp"
#include "channel.hpp"
#include "connectionhandler.hpp"
#include "mtrandom.hpp"
#include "utilities.hpp"

#include <vector>

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

namespace pt = boost::property_tree;

CommandsHandler::CommandsHandler(boost::asio::io_service &_ioService,
                                 Channel *_channelObject)
    : ioService(_ioService)
    , channelObject(_channelObject)
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
    } else if (tokens[0] == "!joinchn") {
        return this->joinChannel(message, tokens);
    } else if (tokens[0] == "!leavechn") {
        return this->leaveChannel(message, tokens);
    } else if (tokens[0] == "!chns") {
        return this->printChannels(message, tokens);
    } else if (tokens[0] == "!remindme") {
        return this->remindMe(message, tokens);
    } else if (tokens[0] == "!remind") {
        return this->remind(message, tokens);
    } else if (tokens[0] == "!asay") {
        return this->say(message, tokens);
    } else if (tokens[0] == "!afk") {
        return this->afk(message, tokens);
    } else if (tokens[0] == "!isafk") {
        return this->isAfk(message, tokens);
    } else if (tokens[0] == "!comebackmsg") {
        return this->comeBackMsg(message, tokens);
    } else if (tokens[0] == "!addblacklist") {
        return this->addBlacklist(message, tokens);
    } else if (tokens[0] == "!removeblacklist") {
        return this->removeBlacklist(message, tokens);
    }

    pt::ptree commandTree = redisClient.getCommandTree(tokens[0]);
    if (commandTree.empty()) {
        return Response();
    }

    // channel user command
    boost::optional<std::string> responseString =
        commandTree.get_optional<std::string>("channels." + message.channel +
                                              "." + message.user + ".response");
    if (responseString) {
        return this->makeResponse(
            message, *responseString, tokens, commandTree,
            "channels." + message.channel + "." + message.user);
    }

    // global user command
    responseString =
        commandTree.get_optional<std::string>(message.user + ".response");
    if (responseString) {
        return this->makeResponse(message, *responseString, tokens, commandTree,
                                  message.user);
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
        return this->makeResponse(message, *responseString, tokens, commandTree,
                                  "default");
    }

    return Response();
}

Response
CommandsHandler::joinChannel(const IRCMessage &message,
                             std::vector<std::string> &tokens)
{
    Response response;
    if (this->isAdmin(message.user) == false) {
        this->channelObject->whisper("You are not an admin NaM", message.user);
        return response;
    }

    if (tokens.size() < 2) {
        this->channelObject->whisper("Usage: !joinchn <channel>", message.user);
        return response;
    }

    changeToLower(tokens[1]);

    if (this->channelObject->owner->joinChannel(tokens[1])) {
        response.message = "Joined the " + tokens[1] + " channel SeemsGood";
        response.type = Response::Type::MESSAGE;
    }
    return response;
}

Response
CommandsHandler::leaveChannel(const IRCMessage &message,
                              std::vector<std::string> &tokens)
{
    Response response;
    if (this->isAdmin(message.user) == false) {
        this->channelObject->whisper("You are not an admin NaM", message.user);
        return response;
    }

    if (tokens.size() < 2) {
        this->channelObject->whisper("Usage: !leavechn <channel>",
                                     message.user);
        return response;
    }

    changeToLower(tokens[1]);

    if (this->channelObject->owner->leaveChannel(tokens[1])) {
        response.message = "Left the " + tokens[1] + " channel SeemsGood";
        response.type = Response::Type::MESSAGE;
    }
    return response;
}

Response
CommandsHandler::printChannels(const IRCMessage &message,
                               std::vector<std::string> &tokens)
{
    Response response;

    std::string channels;
    for (const auto &i : this->channelObject->owner->channels) {
        channels += i.first + ", ";
    }
    channels.pop_back();
    channels.pop_back();

    response.message = "Currently active in channels " + channels;
    response.type = Response::Type::MESSAGE;
    return response;
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
                    tokens[i], boost::regex("{\\d+}|{user}|{channel}"))) {
                return response;
            }
            //boost::algorithm::replace_all(tokens[i], ".", ",");
            boost::algorithm::replace_all(responseString, ss.str(), tokens[i]);
        }
    }

    boost::algorithm::replace_all(responseString, "{user}", message.user);
    boost::algorithm::replace_all(responseString, "{channel}", message.channel);

    if (boost::algorithm::find_regex(responseString, boost::regex("{cirnd}"))) {
        boost::algorithm::replace_all(
            responseString, "{cirnd}",
            std::to_string(MTRandom::getInstance().getInt()));
    }

    while (
        boost::algorithm::find_regex(responseString, boost::regex("{irnd}"))) {
        boost::algorithm::replace_first(
            responseString, "{irnd}",
            std::to_string(MTRandom::getInstance().getInt()));
    }

    if (boost::algorithm::find_regex(responseString, boost::regex("{cdrnd}"))) {
        boost::algorithm::replace_all(
            responseString, "{cdrnd}",
            std::to_string(MTRandom::getInstance().getReal()));
    }

    while (
        boost::algorithm::find_regex(responseString, boost::regex("{drnd}"))) {
        boost::algorithm::replace_first(
            responseString, "{drnd}",
            std::to_string(MTRandom::getInstance().getReal()));
    }

    auto it = boost::algorithm::find_regex(
        responseString, boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"));
    bool success = MTRandom::getInstance().getBool();
    if (it) {
        do {
            std::string match(it.begin(), it.end());
            if (match.compare(0, strlen("{true:"), "{true:") == 0) {
                if (success) {
                    boost::algorithm::replace_regex(
                        responseString,
                        boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"),
                        std::string(match.begin() + sizeof("{true:") - 1,
                                    match.end() - 1),
                        boost::match_default | boost::format_first_only);
                } else {
                    boost::algorithm::erase_regex(
                        responseString,
                        boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"),
                        boost::match_default | boost::format_first_only);
                }
            } else if (match.compare(0, strlen("{false:"), "{false:") == 0) {
                if (!success) {
                    boost::algorithm::replace_regex(
                        responseString,
                        boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"),
                        std::string(match.begin() + sizeof("{false:") - 1,
                                    match.end() - 1),
                        boost::match_default | boost::format_first_only);
                } else {
                    boost::algorithm::erase_regex(
                        responseString,
                        boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"),
                        boost::match_default | boost::format_first_only);
                }
            } else {
                boost::algorithm::replace_regex(
                    responseString, boost::regex("\\{([^\\}:]+):([^\\}]*)\\}"),
                    std::string("{unknown}"),
                    boost::match_default | boost::format_first_only);
            }
        } while (
            it = boost::algorithm::find_regex(
                responseString, boost::regex("\\{([^\\}:]+):([^\\}]*)\\}")));
    }
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

    std::vector<int> vecNumParams{0};  // vector of {number} numbers
    for (size_t i = 3; i < tokens.size(); ++i) {
        auto it_range =
            boost::algorithm::find_regex(tokens[i], boost::regex("{\\d+}"));
        if (!it_range) {
            continue;
        }
        vecNumParams.push_back(boost::lexical_cast<int>(
            std::string(it_range.begin() + 1, it_range.end() - 1)));
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

    std::vector<int> vecNumParams{0};  // vector of {number} numbers
    for (size_t i = 3; i < tokens.size(); ++i) {
        auto it_range =
            boost::algorithm::find_regex(tokens[i], boost::regex("{\\d+}"));
        if (!it_range) {
            continue;
        }
        vecNumParams.push_back(boost::lexical_cast<int>(
            std::string(it_range.begin() + 1, it_range.end() - 1)));
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

auto
findLastIn(std::vector<std::string> &tokens) -> decltype(tokens.size())
{
    auto i = tokens.size();
    for (; i-- > 1;) {
        if (tokens[i] == "in" || tokens[i] == "IN" || tokens[i] == "iN" ||
            tokens[i] == "In") {
            break;
        }
    }
    return i;
}

long long
getSecondsCount(std::vector<std::string> &tokens, size_t inPos)
{
    long long seconds = 0;
    for (int i = inPos + 1; i < tokens.size(); i++) {
        if (tokens[i].back() == 'd') {
            seconds += std::atoll(tokens[i].c_str()) * 24 * 3600;
        } else if (tokens[i].back() == 'h') {
            seconds += std::atoll(tokens[i].c_str()) * 3600;
        } else if (tokens[i].back() == 'm') {
            seconds += std::atoll(tokens[i].c_str()) * 60;
        } else if (tokens[i].back() == 's') {
            seconds += std::atoll(tokens[i].c_str());
        }
    }
    return seconds;
}

std::string
makeReminderMsg(std::vector<std::string> &tokens, size_t inPos)
{
    std::string str;
    int j = 1;
    for (; j < inPos; j++) {
        str += tokens[j] + " ";
    }
    if (str.back() == ' ') {
        str.pop_back();
    }
    return str;
}

Response
CommandsHandler::remindMe(const IRCMessage &message,
                          std::vector<std::string> &tokens)
{
    Response response;
    auto inPos = findLastIn(tokens);
    if (inPos == 0 || inPos + 1 >= tokens.size()) {
        response.type = Response::Type::WHISPER;
        response.whisperReceiver = message.user;
        response.message = "Usage \"!remindme [your message] in 20s "
                           "15h 10d 9m (the order does not matter, "
                           "the number must be immediately followed "
                           "by an identifier)\"";
        return response;
    }

    long long seconds = getSecondsCount(tokens, inPos);
    if (seconds == 0) {
        response.type = Response::Type::WHISPER;
        response.message = "Usage \"!remindme [your message] in 20s "
                           "15h 10d 9m (the order does not matter, "
                           "the number must be immediately followed "
                           "by an identifier)\"";
        response.whisperReceiver = message.user;
        return response;
    } else if (seconds > 604800) {
        response.type = Response::Type::WHISPER;
        response.message = "Maximum amount of time is 7 days NaM";
        response.whisperReceiver = message.user;
        return response;
    } else if (seconds < 0) {
        response.type = Response::Type::WHISPER;
        response.message = "Negative amount of time "
                           "\xF0\x9F\xA4\x94, sorry I can't travel "
                           "back in time (yet)",
        response.whisperReceiver = message.user;
        return response;
    }

    std::string reminderMessage = makeReminderMsg(tokens, inPos);
    if (reminderMessage.empty()) {
        reminderMessage = "No reminder";
    }

    std::string whichReminder =
        this->redisClient.addReminder(message.user, seconds, reminderMessage);

    auto whisperMessage =
        reminderMessage + " - " + makeTimeString(seconds) + " ago";
    auto remindFunction =
        [
          owner = this->channelObject->owner, user = message.user,
          whisperMessage, whichReminder, seconds
        ](const boost::system::error_code &er)
            ->void
    {
        if (er) {
            std::cerr << "Timer error: " << er << std::endl;
            return;
        }
        if (owner->channels.count(owner->nick) == 0) {
            owner->joinChannel(owner->nick);
        }

        owner->channels.at(owner->nick).whisper(whisperMessage, user);
        owner->channels.at(owner->nick)
            .commandsHandler.redisClient.removeReminder(user, whichReminder);
    };

    auto t =
        new boost::asio::steady_timer(ioService, std::chrono::seconds(seconds));
    t->async_wait(remindFunction);
    //boost::algorithm::replace_all(reminderMessage, ".", ",");
    std::string msg = message.user + ", reminding you in " +
                      std::to_string(seconds) + " seconds (" + reminderMessage +
                      ") SeemsGood";

    response.type = Response::Type::MESSAGE;
    response.message = msg;
    return response;
}

Response
CommandsHandler::remind(const IRCMessage &message,
                        std::vector<std::string> &tokens)
{
    Response response;
    if (tokens.size() < 2) {
        response.type = Response::Type::WHISPER;
        response.whisperReceiver = message.user;
        response.message = "Usage \"!remind <who> [your message] in 20s "
                           "15h 10d 9m (the order does not matter, "
                           "the number must be immediately followed "
                           "by an identifier)\"";
        return response;
    }
    changeToLower(tokens[1]);
    std::string remindedUser = tokens[1];
    tokens.erase(tokens.begin() + 1);

    auto inPos = findLastIn(tokens);
    if (inPos == 0 || inPos + 1 >= tokens.size()) {
        response.type = Response::Type::WHISPER;
        response.whisperReceiver = message.user;
        response.message = "Usage \"!remind <who> [your message] in 20s "
                           "15h 10d 9m (the order does not matter, "
                           "the number must be immediately followed "
                           "by an identifier)\"";
        return response;
    }

    long long seconds = getSecondsCount(tokens, inPos);
    if (seconds == 0) {
        response.type = Response::Type::WHISPER;
        response.message = "Usage \"!remind <who> [your message] in 20s "
                           "15h 10d 9m (the order does not matter, "
                           "the number must be immediately followed "
                           "by an identifier)\"";
        response.whisperReceiver = message.user;
        return response;
    } else if (seconds > 604800) {
        response.type = Response::Type::WHISPER;
        response.message = "Maximum amount of time is 7 days NaM";
        response.whisperReceiver = message.user;
        return response;
    } else if (seconds < 0) {
        response.type = Response::Type::WHISPER;
        response.message = "Negative amount of time "
                           "\xF0\x9F\xA4\x94, sorry I can't travel "
                           "back in time (yet)",
        response.whisperReceiver = message.user;
        return response;
    }

    std::string reminderMessage =
        makeReminderMsg(tokens, inPos) + " - Reminder by " + message.user;
    if (reminderMessage.empty()) {
        reminderMessage = "Reminder by " + message.user;
    }

    std::string whichReminder =
        this->redisClient.addReminder(remindedUser, seconds, reminderMessage);

    int countOfReminders =
        this->channelObject->owner->userReminders.count(message.user);
    RemindUsers::Reminder pair;
    if (countOfReminders >= 2) {
        pair = this->channelObject->owner->userReminders.getFirst(message.user);
        this->channelObject->owner->userReminders.removeFirst(message.user);
        this->redisClient.removeReminder(pair.toUser, pair.whichReminder);
        pair.timer->cancel();
    }

    auto whisperMessage =
        reminderMessage + " " + makeTimeString(seconds) + " ago";
    auto remindFunction =
        [
          owner = this->channelObject->owner, user = remindedUser,
          whisperMessage, whichReminder, from = message.user, seconds
        ](const boost::system::error_code &er)
            ->void
    {
        if (er) {
            std::cerr << "Timer error: " << er << std::endl;
            return;
        }
        if (owner->channels.count(owner->nick) == 0) {
            owner->joinChannel(owner->nick);
        }

        owner->channels.at(owner->nick).whisper(whisperMessage, user);
        owner->channels.at(owner->nick)
            .commandsHandler.redisClient.removeReminder(user, whichReminder);
        owner->userReminders.remove(from, user, whichReminder);
    };

    auto t =
        new boost::asio::steady_timer(ioService, std::chrono::seconds(seconds));
    t->async_wait(remindFunction);
    this->channelObject->owner->userReminders.addReminder(
        message.user, remindedUser, whichReminder, t);
    //boost::algorithm::replace_all(reminderMessage, ".", ",");
    std::string msg = message.user + ", reminding " + remindedUser + " in " +
                      std::to_string(seconds) + " seconds (" + reminderMessage +
                      ") SeemsGood";
    if (countOfReminders >= 2) {
        msg += " Also removed old reminder for " + pair.toUser;
    }

    response.type = Response::Type::MESSAGE;
    response.message = msg;
    return response;
}

Response
CommandsHandler::say(const IRCMessage &message,
                     std::vector<std::string> &tokens)
{
    Response response;
    if (tokens.size() < 2) {
        return response;
    }

    if (this->isAdmin(message.user) == false) {
        return response;
    }

    tokens.erase(tokens.begin());
    for (const auto &i : tokens) {
        response.message += i + " ";
    }
    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::afk(const IRCMessage &message,
                     std::vector<std::string> &tokens)
{
    Response response;
    std::string msg;
    for (int i = 1; i < tokens.size(); ++i) {
        msg += tokens[i] + ' ';
    }
    //boost::algorithm::replace_all(msg, ".", ",");

    if (msg.back() == ' ') {
        msg.pop_back();
    }

    this->channelObject->owner->afkers.setAfker(message.user, msg);

    response.type = Response::Type::MESSAGE;
    response.message = message.user + " is now afk ResidentSleeper";
    if (msg.size() != 0) {
        response.message += " - " + msg;
    }

    return response;
}

Response
CommandsHandler::isAfk(const IRCMessage &message,
                       std::vector<std::string> &tokens)
{
    Response response;
    if (tokens.size() < 2) {
        return response;
    }

    changeToLower(tokens[1]);

    auto afk = this->channelObject->owner->afkers.getAfker(tokens[1]);

    if (afk.exists) {
        auto now = std::chrono::steady_clock::now();
        auto seconds =
            std::chrono::duration_cast<std::chrono::seconds>(now - afk.time)
                .count();
        std::string when = makeTimeString(seconds);
        if (when.size() == 0) {
            when = "0s";
        }
        response.message =
            message.user + ", " + tokens[1] + " went afk " + when + " ago";
        if (afk.message.size() != 0) {
            response.message += " - " + afk.message;
        }
    } else {
        response.message =
            message.user + ", user " + tokens[1] + " is not afk SeemsGood";
    }

    response.type = Response::Type::MESSAGE;
    return response;
}

Response
CommandsHandler::comeBackMsg(const IRCMessage &message,
                             std::vector<std::string> &tokens)
{
    Response response;
    if (this->isAdmin(message.user) == false) {
        return response;
    }
    if (tokens.size() < 3) {
        return response;
    }

    changeToLower(tokens[1]);

    std::string msg;
    for (int i = 2; i < tokens.size(); ++i) {
        msg += tokens[i] + ' ';
    }

    if (msg.back() == ' ') {
        msg.pop_back();
    }

    this->channelObject->owner->comebacks.makeComeBackMsg(message.user,
                                                          tokens[1], msg);

    response.type = Response::Type::MESSAGE;
    response.message =
        message.user + ", the user " + tokens[1] +
        " will get your message once he writes anything in chat SeemsGood";
    return response;
}

Response
CommandsHandler::addBlacklist(const IRCMessage &message,
                             std::vector<std::string> &tokens)
{
    Response response;
    if (this->isAdmin(message.user) == false) {
        return response;
    }    
    if (tokens.size() == 2) {
        tokens.push_back("*");
    } else if (tokens.size() < 3) {
        return response;
    }
    
    changeToLower(tokens[1]);
    
    this->channelObject->owner->addBlacklist(tokens[1], tokens[2]);
    
    response.type = Response::Type::MESSAGE;
    response.message =
        message.user + ", added blacklist for " + tokens[1] + " SeemsGood";
    return response;
}

Response
CommandsHandler::removeBlacklist(const IRCMessage &message,
                             std::vector<std::string> &tokens)
{
    Response response;
    if (this->isAdmin(message.user) == false) {
        return response;
    }    
    if (tokens.size() < 2) {
        return response;
    }
    
    changeToLower(tokens[1]);
    
    this->channelObject->owner->removeBlacklist(tokens[1]);
    response.type = Response::Type::MESSAGE;
    response.message =
        message.user + ", removed blacklist for " + tokens[1] + " SeemsGood";
    return response;
}