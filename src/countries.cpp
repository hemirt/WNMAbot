#include "countries.hpp"
#include "utilities.hpp"

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

void
Countries::setFrom(std::string user, std::string from)
{
    changeToLower(country);
    changeToLower(user);

    std::string userIDstr = this->userIDs.getID(user);
    if (userIDstr.empty()) {
        return;
    }
    std::string countryIDstr = this->getCountryID(from);
    if (countryIDstr.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HSET WNMA:user:%b countryFrom %b", userIDstr.c_str(),
        userIDstr.size(), countryIDstr.c_str(), countryIDstr.size()))
        freeReplyObject(reply);

    this->addUserFromCountry(userIDstr, countryIDstr);
}

void
Countries::setLive(std::string user, std::string living)
{
    changeToLower(living);
    changeToLower(user);

    std::string userIDstr = this->userIDs.getID(user);
    if (userIDstr.empty()) {
        return;
    }
    std::string countryIDstr = this->getCountryID(living);
    if (countryIDstr.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HSET WNMA:user:%b countryLive %b", userIDstr.c_str(),
        userIDstr.size(), countryIDstr.c_str(), countryIDstr.size()))
        freeReplyObject(reply);

    this->addUserLiveCountry(userIDstr, countryIDstr)
}

void
Countries::addUserFromCountry(const std::string &userIDstr,
                              const std::string &countryIDstr)
{
    // must be already called under lock(this->accessMtx)
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SADD WNMA:country:%b:from %b", countryIDstr.c_str(),
        countryIDstr.size(), userIDstr.c_str(), userIDstr.size()));
    freeReplyObject(reply);
}

void
Countries::addUserLiveCountry(const std::string &userIDstr,
                              const std::string &countryIDstr)
{
    // must be already called under lock(this->accessMtx)
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SADD WNMA:country:%b:live %b", countryIDstr.c_str(),
        countryIDstr.size(), userIDstr.c_str(), userIDstr.size()));
    freeReplyObject(reply);
}

std::vector<std::string>
Countries::usersFrom(const std::string &country)
{
    std::vector<std::string> users;
    std::string countryIDstr = this->getCountryID(country);
    if (countryIDstr.empty()) {
        return users;
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SMEMBERS WNMA:country:%b:from",
                     countryIDstr.c_str(), countryIDstr.size()));

    if (reply == NULL || reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return users;
    }

    for (int i = 0; i < reply->len; i++) {
        std::string username = this->userIDs.getUserName(
            std::string(reply->element[i]->str, reply->element[i]->len));
        if (!username.empty()) {
            users.emplace_back(std::move(username));
        }
    }

    return users;
}

std::vector<std::string>
Countries::usersLive(const std::string &country)
{
    std::vector<std::string> users;
    std::string countryIDstr = this->getCountryID(country);
    if (countryIDstr.empty()) {
        return users;
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "SMEMBERS WNMA:country:%b:live",
                     countryIDstr.c_str(), countryIDstr.size()));

    if (reply == NULL || reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return users;
    }

    for (int i = 0; i < reply->len; i++) {
        std::string username = this->userIDs.getUserName(
            std::string(reply->element[i]->str, reply->element[i]->len));
        if (!username.empty()) {
            users.emplace_back(std::move(username));
        }
    }

    return users;
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
    } else if (reply->type != REDIS_REPLY_STRING) {
        // country is not in the database, create it
        freeReplyObject(reply);

        // get next countryID
        reply = static_cast<redisReply *>(
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

        // create WNMA:country:ID:displayname
        reply = static_cast<redisReply *>(
            redisCommand(this->context, "SET WNMA:country:%b:displayname %b",
                         countryIDstr.c_str(), countryIDstr.size(),
                         country.c_str(), country.size()));
        freeReplyObject(reply);

        return countryIDstr;
    } else {
        // country exists, return id
        std::string countryIDstr(reply->str, reply->len);
        freeReplyObject(reply);
        return countryIDstr;
    }
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

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HDEL WNMA:user:%b countryFrom",
                     userIDstr.c_str(), userIDstr.size()));
    freeReplyObject(reply);
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

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HDEL WNMA:user:%b countryLive",
                     userIDstr.c_str(), userIDstr.size()));
    freeReplyObject(reply);
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

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:user:%b countryFrom",
                     userIDstr.c_str(), userIDstr.size()));
    if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return std::string();
    }

    std::string countryIDstr(reply->str, reply->len);
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET:country:%b:displayname",
                     countryIDstr.c_str(), countryIDstr.size()));
    if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return std::string();
    }

    std::string countryDisplayName(reply->str, reply->len);
    freeReplyObject(reply);
    return countryDisplayName;
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

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:user:%b countryLive",
                     userIDstr.c_str(), userIDstr.size()));
    if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return std::string();
    }

    std::string countryIDstr(reply->str, reply->len) freeReplyObject(reply);

    reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET:country:%b:displayname",
                     countryIDstr.c_str(), countryIDstr.size()));
    if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return std::string();
    }

    std::string countryDisplayName(reply->str, reply->len);
    freeReplyObject(reply);
    return countryDisplayName;
}

// std::string("\u05C4")