#include "countries.hpp"
#include <iostream>
#include "utilities.hpp"
#include <algorithm>
#include <random>

using std::experimental::optional;

redisContext *Countries::context = NULL;
std::mutex Countries::accessMtx;

Countries::Countries()
    : userIDs(UserIDs::getInstance())
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "Users error: " << this->context->errstr << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "Users can't allocate redis context" << std::endl;
        }
    }
}

Countries::~Countries()
{
    if (!this->context)
        return;
    redisFree(this->context);
}

Countries::Result
Countries::setFrom(std::string user, std::string from)
{
    changeToLower(user);

    std::string userIDstr = this->userIDs.getID(user);
    if (userIDstr.empty()) {
        return Countries::Result::NOUSER;
    }
    std::string countryIDstr = this->getCountryID(from);
    if (countryIDstr.empty()) {
        return Countries::Result::NOCOUNTRY;
    }

    this->deleteFrom(user);

    std::lock_guard<std::mutex> lock(this->accessMtx);

    auto prevCountryID =
        this->redisGetUserCountry(userIDstr, Countries::Type::FROM);
    if (prevCountryID) {
        this->redisRemUserFromCountry(userIDstr, prevCountryID.value(),
                                      Countries::Type::FROM);
    }

    this->redisSetUserCountry(userIDstr, countryIDstr, Countries::Type::FROM);
    this->redisAddUserToCountrySet(userIDstr, countryIDstr,
                                   Countries::Type::FROM);

    if (!this->redisExistsUserCountry(userIDstr, Countries::Type::LIVE)) {
        this->redisSetUserCountry(userIDstr, countryIDstr,
                                  Countries::Type::LIVE);
        this->redisAddUserToCountrySet(userIDstr, countryIDstr,
                                       Countries::Type::LIVE);
    }
    return Countries::Result::SUCCESS;
}

Countries::Result
Countries::setLive(std::string user, std::string living)
{
    changeToLower(user);

    std::string userIDstr = this->userIDs.getID(user);
    if (userIDstr.empty()) {
        return Countries::Result::NOUSER;
    }
    std::string countryIDstr = this->getCountryID(living);
    if (countryIDstr.empty()) {
        return Countries::Result::NOCOUNTRY;
    }

    this->deleteLive(user);

    std::lock_guard<std::mutex> lock(this->accessMtx);

    auto prevCountryID =
        this->redisGetUserCountry(userIDstr, Countries::Type::LIVE);
    if (prevCountryID) {
        this->redisRemUserFromCountry(userIDstr, prevCountryID.value(),
                                      Countries::Type::LIVE);
    }

    this->redisSetUserCountry(userIDstr, countryIDstr, Countries::Type::LIVE);
    this->redisAddUserToCountrySet(userIDstr, countryIDstr,
                                   Countries::Type::LIVE);
    return Countries::Result::SUCCESS;
}

std::pair<std::string, std::vector<std::string>>
Countries::usersFrom(const std::string &country)
{
    std::pair<std::string, std::vector<std::string>> pair;
    std::string countryIDstr = this->getCountryID(country);
    if (countryIDstr.empty()) {
        return pair;
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    auto display = this->redisGetDisplayName(countryIDstr);
    if (!display) {
        return pair;
    } else {
        pair.first = display.value();
    }

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SMEMBERS WNMA:country:%b:from",
                     countryIDstr.c_str(), countryIDstr.size()));

    if (reply == NULL || reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return pair;
    }
    for (decltype(reply->elements) i = 0; i < reply->elements; i++) {
        std::string username = this->userIDs.getUserName(
            std::string(reply->element[i]->str, reply->element[i]->len));
        if (!username.empty()) {
            pair.second.emplace_back(std::move(username));
        }
    }
    if (pair.second.size() > 5) {
        static std::random_device rd;
        static std::mt19937 g(rd());
        std::shuffle(pair.second.begin(), pair.second.end(), g);
        pair.second[5] = " in total " + std::to_string(pair.second.size()) + " users";
        pair.second.resize(6);
    }

    return pair;
}

std::pair<std::string, std::vector<std::string>>
Countries::usersLive(const std::string &country)
{
    std::pair<std::string, std::vector<std::string>> pair;
    std::string countryIDstr = this->getCountryID(country);
    if (countryIDstr.empty()) {
        return pair;
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    auto display = this->redisGetDisplayName(countryIDstr);
    if (!display) {
        return pair;
    } else {
        pair.first = display.value();
    }

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SMEMBERS WNMA:country:%b:live",
                     countryIDstr.c_str(), countryIDstr.size()));

    if (reply == NULL || reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return pair;
    }

    for (decltype(reply->elements) i = 0; i < reply->elements; i++) {
        std::string username = this->userIDs.getUserName(
            std::string(reply->element[i]->str, reply->element[i]->len));
        if (!username.empty()) {
            pair.second.emplace_back(std::move(username));
        }
    }

    return pair;
}

std::string
Countries::getCountryID(const std::string &country)
{
    std::string lcCountry = changeToLower_copy(country);

    std::lock_guard<std::mutex> lock(this->accessMtx);

    // get countries id
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:countries %b", lcCountry.c_str(),
                     lcCountry.size()));

    if (reply == NULL) {
        // error
        return std::string();
    } else if (reply->type == REDIS_REPLY_STRING) {
        std::string countryIDstr(reply->str, reply->len);
        freeReplyObject(reply);
        return countryIDstr;
    } else {
        return std::string();
    }
}

std::string
Countries::getCountryName(const std::string &countryID)
{
    std::lock_guard<std::mutex> lock(this->accessMtx);

    // get countries name
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:countryIDs %b",
                     countryID.c_str(), countryID.size()));

    if (reply == NULL) {
        // error
        return std::string();
    } else if (reply->type == REDIS_REPLY_STRING) {
        std::string countryName(reply->str, reply->len);
        freeReplyObject(reply);
        return countryName;
    } else {
        return std::string();
    }
}

std::string
Countries::createCountry(const std::string &country)
{
    std::lock_guard<std::mutex> lock(this->accessMtx);
    std::string lcCountry = changeToLower_copy(country);
    // get next countryID
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "INCR WNMA:nextcountryid"));
    if (reply == NULL || reply->type != REDIS_REPLY_INTEGER) {
        return std::string();
    }
    int countryID = reply->integer;
    freeReplyObject(reply);

    std::string countryIDstr = std::to_string(countryID);

    // add a country record to WNMA:countries name - id
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "HSET WNMA:countries %b %b", lcCountry.c_str(),
        lcCountry.size(), countryIDstr.c_str(), countryIDstr.size()));
    freeReplyObject(reply);

    // add id record
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "HSET WNMA:countryIDs %b %b", countryIDstr.c_str(),
        countryIDstr.size(), lcCountry.c_str(), lcCountry.size()));
    freeReplyObject(reply);

    // create WNMA:country:ID:displayname
    reply = static_cast<redisReply *>(
        redisCommand(this->context, "SET WNMA:country:%b:displayname %b",
                     countryIDstr.c_str(), countryIDstr.size(), country.c_str(),
                     country.size()));
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(redisCommand(
        this->context, "SADD WNMA:country:%b:alias %b", countryIDstr.c_str(),
        countryIDstr.size(), lcCountry.c_str(), lcCountry.size()));
    freeReplyObject(reply);

    return countryIDstr;
}

bool
Countries::createAlias(const std::string &countryID, const std::string &country)
{
    if (!this->existsCountry(countryID)) {
        return false;
    }

    std::string lcCountry = changeToLower_copy(country);

    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SADD WNMA:country:%b:alias %b", countryID.c_str(),
        countryID.size(), lcCountry.c_str(), lcCountry.size()));
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(redisCommand(
        this->context, "HSET WNMA:countries %b %b", lcCountry.c_str(),
        lcCountry.size(), countryID.c_str(), countryID.size()));
    freeReplyObject(reply);

    return true;
}

bool
Countries::deleteAlias(const std::string &countryID, const std::string &country)
{
    if (!this->existsCountry(countryID)) {
        return false;
    }

    std::string lcCountry = changeToLower_copy(country);

    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SREM WNMA:country:%b:alias %b", countryID.c_str(),
        countryID.size(), lcCountry.c_str(), lcCountry.size()));
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(redisCommand(
        this->context, "HDEL WNMA:countries %b %b", lcCountry.c_str(),
        lcCountry.size(), countryID.c_str(), countryID.size()));
    freeReplyObject(reply);

    return true;
}

void
Countries::deleteFrom(std::string user)
{
    changeToLower(user);
    std::string userIDstr = this->userIDs.getID(user);
    if (userIDstr.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    auto prevCountryID =
        this->redisGetUserCountry(userIDstr, Countries::Type::FROM);
    if (!prevCountryID) {
        return;
    }

    this->redisRemUserCountry(userIDstr, Countries::Type::FROM);
    this->redisRemUserFromCountry(userIDstr, prevCountryID.value(),
                                  Countries::Type::FROM);

    auto countryLive =
        this->redisGetUserCountry(userIDstr, Countries::Type::LIVE);
    if (countryLive && countryLive.value() == prevCountryID.value()) {
        this->redisRemUserFromCountry(userIDstr, countryLive.value(),
                                      Countries::Type::LIVE);
        this->redisRemUserCountry(userIDstr, Countries::Type::LIVE);
    }
}

void
Countries::deleteLive(std::string user)
{
    changeToLower(user);
    std::string userIDstr = this->userIDs.getID(user);
    if (userIDstr.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    // get old countryLive value

    auto countryIDstr =
        this->redisGetUserCountry(userIDstr, Countries::Type::LIVE);
    if (!countryIDstr) {
        return;
    }

    this->redisRemUserCountry(userIDstr, Countries::Type::LIVE);
    this->redisRemUserFromCountry(userIDstr, countryIDstr.value(),
                                  Countries::Type::LIVE);

    auto countryIComeFrom =
        this->redisGetUserCountry(userIDstr, Countries::Type::FROM);

    if (countryIComeFrom) {
        this->redisSetUserCountry(userIDstr, countryIComeFrom.value(),
                                  Countries::Type::LIVE);
    }
}

void
Countries::deleteUserCountryByID(const std::string &userID,
                                 const Countries::Type &type)
{
    std::string format = "HDEL WNMA:user:%b country";
    if (type == Countries::Type::FROM) {
        format += "From";
    } else if (type == Countries::Type::LIVE) {
        format += "Live";
    } else {
        return;
    }
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, format.c_str(), userID.c_str(), userID.size()));
    freeReplyObject(reply);
    return;
}

std::string
Countries::getFrom(std::string user)
{
    changeToLower(user);
    std::string userIDstr = this->userIDs.getID(user);
    if (userIDstr.empty()) {
        return std::string();
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    auto countryIDstr =
        this->redisGetUserCountry(userIDstr, Countries::Type::FROM);
    if (!countryIDstr) {
        return std::string();
    }

    auto display = this->redisGetDisplayName(countryIDstr.value());
    if (!display) {
        return std::string();
    }

    return display.value();
}

std::string
Countries::getLive(std::string user)
{
    changeToLower(user);
    std::string userIDstr = this->userIDs.getID(user);
    if (userIDstr.empty()) {
        return std::string();
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    auto countryIDstr =
        this->redisGetUserCountry(userIDstr, Countries::Type::LIVE);
    if (!countryIDstr) {
        return std::string();
    }

    auto display = this->redisGetDisplayName(countryIDstr.value());
    if (!display) {
        return std::string();
    }

    return display.value();
}

bool
Countries::existsCountry(const std::string &countryID)
{
    auto countryName = this->getCountryName(countryID);
    if (countryName.empty()) {
        return false;
    } else {
        return true;
    }
}

bool
Countries::deleteCountry(const std::string &countryID)
{
    auto countryName = this->getCountryName(countryID);
    if (countryName.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HDEL WNMA:countries %b",
                     countryName.c_str(), countryName.size()));
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(
        redisCommand(this->context, "HDEL WNMA:countryIDs %b",
                     countryID.c_str(), countryID.size()));
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(
        redisCommand(this->context, "SMEMBERS WNMA:country:%b:live",
                     countryID.c_str(), countryID.size()));

    if (reply != NULL && reply->type == REDIS_REPLY_ARRAY) {
        for (decltype(reply->elements) i = 0; i < reply->elements; i++) {
            std::string userID(reply->element[i]->str, reply->element[i]->len);
            this->deleteUserCountryByID(userID, Countries::Type::LIVE);
        }
    }
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(
        redisCommand(this->context, "SMEMBERS WNMA:country:%b:from",
                     countryID.c_str(), countryID.size()));

    if (reply != NULL && reply->type == REDIS_REPLY_ARRAY) {
        for (decltype(reply->elements) i = 0; i < reply->elements; i++) {
            std::string userID(reply->element[i]->str, reply->element[i]->len);
            this->deleteUserCountryByID(userID, Countries::Type::FROM);
        }
    }
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(
        redisCommand(this->context, "DEL WNMA:country:%b:live",
                     countryID.c_str(), countryID.size()));
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(
        redisCommand(this->context, "DEL WNMA:country:%b:from",
                     countryID.c_str(), countryID.size()));
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(
        redisCommand(this->context, "DEL WNMA:country:%b:displayname",
                     countryID.c_str(), countryID.size()));
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(
        redisCommand(this->context, "SMEMBERS WNMA:country:%b:alias",
                     countryID.c_str(), countryID.size()));

    if (reply != NULL && reply->type == REDIS_REPLY_ARRAY) {
        redisReply *reply2;
        for (decltype(reply->elements) i = 0; i < reply->elements; i++) {
            std::string alias(reply->element[i]->str, reply->element[i]->len);
            reply2 = static_cast<redisReply *>(
                redisCommand(this->context, "HDEL WNMA:countries %b",
                             alias.c_str(), alias.size()));
            freeReplyObject(reply2);
        }
    }
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(
        redisCommand(this->context, "DEL WNMA:country:%b:alias",
                     countryID.c_str(), countryID.size()));
    freeReplyObject(reply);

    return true;
}

bool
Countries::renameCountry(const std::string &countryID,
                         const std::string &newCountryName)
{
    if (this->existsCountry(countryID)) {
        std::lock_guard<std::mutex> lock(this->accessMtx);

        redisReply *reply = static_cast<redisReply *>(
            redisCommand(this->context, "SET WNMA:country:%b:displayname %b",
                         countryID.c_str(), countryID.size(),
                         newCountryName.c_str(), newCountryName.size()));
        freeReplyObject(reply);
        return true;
    } else {
        return false;
    }
}

// internal helper functions

optional<std::string>
Countries::redisGetDisplayName(const std::string &countryIDstr)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "GET WNMA:country:%b:displayname",
                     countryIDstr.c_str(), countryIDstr.size()));
    if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return std::experimental::nullopt;
    }
    std::string display(reply->str, reply->len);
    freeReplyObject(reply);
    return display;
}

optional<std::string>
Countries::redisGetUserCountry(const std::string &userID,
                               const Countries::Type &type)
{
    std::string format = "HGET WNMA:user:%b country";
    if (type == Countries::Type::FROM) {
        format += "From";
    } else if (type == Countries::Type::LIVE) {
        format += "Live";
    } else {
        return std::experimental::nullopt;
    }
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, format.c_str(), userID.c_str(), userID.size()));
    if (!reply) {
        return std::experimental::nullopt;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return std::experimental::nullopt;
    }

    std::string country(reply->str, reply->len);
    freeReplyObject(reply);
    return country;
}

void
Countries::redisRemUserFromCountry(const std::string &userID,
                                   const std::string &countryID,
                                   const Countries::Type &type)
{
    std::string format = "SREM WNMA:country:%b:";
    if (type == Countries::Type::FROM) {
        format += "from %b";
    } else if (type == Countries::Type::LIVE) {
        format += "live %b";
    } else {
        return;
    }

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, format.c_str(), countryID.c_str(),
                     countryID.size(), userID.c_str(), userID.size()));
    freeReplyObject(reply);
}

void
Countries::redisSetUserCountry(const std::string &userID,
                               const std::string &countryID,
                               const Countries::Type &type)
{
    std::string format = "HSET WNMA:user:%b country";
    if (type == Countries::Type::FROM) {
        format += "From %b";
    } else if (type == Countries::Type::LIVE) {
        format += "Live %b";
    } else {
        return;
    }
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, format.c_str(), userID.c_str(),
                     userID.size(), countryID.c_str(), countryID.size()));
    freeReplyObject(reply);
}

void
Countries::redisRemUserCountry(const std::string &userID,
                               const Countries::Type &type)
{
    std::string format = "HDEL WNMA:user:%b country";
    if (type == Countries::Type::FROM) {
        format += "From";
    } else if (type == Countries::Type::LIVE) {
        format += "Live";
    } else {
        return;
    }
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, format.c_str(), userID.c_str(), userID.size()));
    freeReplyObject(reply);
}

bool
Countries::redisExistsUserCountry(const std::string &userID,
                                  const Countries::Type &type)
{
    std::string format = "HEXISTS WNMA:user:%b country";
    if (type == Countries::Type::FROM) {
        format += "From";
    } else if (type == Countries::Type::LIVE) {
        format += "Live";
    } else {
        return false;
    }
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, format.c_str(), userID.c_str(), userID.size()));

    if (!reply) {
        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return false;
    }

    bool ret = reply->integer == 1;
    freeReplyObject(reply);
    return ret;
}

void
Countries::redisAddUserToCountrySet(const std::string &userID,
                                    const std::string &countryID,
                                    const Countries::Type &type)
{
    std::string format = "SADD WNMA:country:%b:";
    if (type == Countries::Type::FROM) {
        format += "from %b";
    } else if (type == Countries::Type::LIVE) {
        format += "live %b";
    } else {
        return;
    }

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, format.c_str(), countryID.c_str(),
                     countryID.size(), userID.c_str(), userID.size()));
    freeReplyObject(reply);
}

// std::string("\u05C4")