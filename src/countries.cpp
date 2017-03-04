#include "countries.hpp"
#include <iostream>
#include "utilities.hpp"

redisContext *Countries::context = NULL;
std::mutex Countries::accessMtx;
Countries Countries::instance;

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
    changeToLower(user);
    
    this->deleteFrom(user);

    std::string userIDstr = this->userIDs.getID(user);
    if (userIDstr.empty()) {
        return;
    }
    std::string countryIDstr = this->getCountryID(from);
    if (countryIDstr.empty()) {
        countryIDstr = this->createCountry(from);
        if (countryIDstr.empty()) {
            return;
        }
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);
    
    auto prevCountryID = this->redisGetUserCountry(userIDstr, Countries::Type::FROM);
    if (prevCountryID) {
        this->redisRemUserFromCountry(userIDstr, prevCountryID.value(), Countries::Type::FROM); 
    }

    this->redisSetUserCountry(userIDstr, countryIDstr, Countries::Type::FROM);
    this->redisAddUserToCountrySet(userIDstr, countryIDstr, Countries::Type::FROM);
    
    if(!this->redisExistsUserCountry(userIDstr, Countries::Type::LIVE)) {
        this->redisSetUserCountry(userIDstr, countryIDstr, Countries::Type::LIVE);
        this->redisAddUserToCountrySet(userIDstr, countryIDstr, Countries::Type::LIVE);
    }   
}

void
Countries::setLive(std::string user, std::string living)
{
    changeToLower(user);
    
    this->deleteLive(user);

    std::string userIDstr = this->userIDs.getID(user);
    if (userIDstr.empty()) {
        return;
    }
    std::string countryIDstr = this->getCountryID(living);
    if (countryIDstr.empty()) {
        countryIDstr = this->createCountry(living);
        if (countryIDstr.empty()) {
            return;
        }
    }

    std::lock_guard<std::mutex> lock(this->accessMtx);

    auto prevCountryID = this->redisGetUserCountry(userIDstr, Countries::Type::LIVE);
    if (prevCountryID) {
        this->redisRemUserFromCountry(userIDstr, prevCountryID.value(), Countries::Type::LIVE); 
    }
    
    this->redisSetUserCountry(userIDstr, countryIDstr, Countries::Type::LIVE);
    this->redisAddUserToCountrySet(userIDstr, countryIDstr, Countries::Type::LIVE);
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
    for (int i = 0; i < reply->elements; i++) {
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

    for (int i = 0; i < reply->elements; i++) {
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
    } else if (reply->type == REDIS_REPLY_STRING) {
        std::string countryIDstr(reply->str, reply->len);
        freeReplyObject(reply);
        return countryIDstr;
    } else {
        return std::string();
    }
}

std::string
Countries::createCountry(const std::string &country)
{
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

    // create WNMA:country:ID:displayname
    reply = static_cast<redisReply *>(
        redisCommand(this->context, "SET WNMA:country:%b:displayname %b",
                     countryIDstr.c_str(), countryIDstr.size(),
                     country.c_str(), country.size()));
    freeReplyObject(reply);

    return countryIDstr;
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
    
    
    auto prevCountryID = this->redisGetUserCountry(userIDstr, Countries::Type::FROM);
    if (!prevCountryID) {
        return;
    }
    
    this->redisRemUserCountry(userIDstr, Countries::Type::FROM);
    this->redisRemUserFromCountry(userIDstr, prevCountryID.value(), Countries::Type::FROM);
    
    auto countryLive = this->redisGetUserCountry(userIDstr, Countries::Type::LIVE);
    if (countryLive && countryLive.value() == prevCountryID.value()) {
        this->redisRemUserFromCountry(userIDstr, countryLive.value(), Countries::Type::LIVE);
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
    
    auto countryIDstr = this->redisGetUserCountry(userIDstr, Countries::Type::LIVE);
    if (!countryIDstr) {
        return;
    }
    
    this->redisRemUserCountry(userIDstr, Countries::Type::LIVE);
    this->redisRemUserFromCountry(userIDstr, countryIDstr.value(), Countries::Type::LIVE);
    
    auto countryIComeFrom = this->redisGetUserCountry(userIDstr, Countries::Type::FROM);
    
    if(countryIComeFrom) {
        this->redisSetUserCountry(userIDstr, countryIComeFrom.value(), Countries::Type::LIVE);
    }
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
    
    auto countryIDstr = this->redisGetUserCountry(userIDstr, Countries::Type::FROM);
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

    auto countryIDstr = this->redisGetUserCountry(userIDstr, Countries::Type::LIVE);    
    if (!countryIDstr) {
        return std::string();
    }
    
    auto display = this->redisGetDisplayName(countryIDstr.value());
    if (!display) {
        return std::string();
    }
    
    return display.value();
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
Countries::redisGetUserCountry(const std::string &userID, const Countries::Type &type)
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
        this->context, format.c_str(), userID.c_str(),
        userID.size()));
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
Countries::redisRemUserFromCountry(const std::string &userID, const std::string &countryID, const Countries::Type &type)
{    
    std::string format = "SREM WNMA:country:%b:";
    if (type == Countries::Type::FROM) {
        format += "from %b";
    } else if (type == Countries::Type::LIVE) {
        format += "live %b";
    } else {
        return;
    }
    
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, format.c_str(), countryID.c_str(),
        countryID.size(), userID.c_str(), userID.size()));
    freeReplyObject(reply);
}

void
Countries::redisSetUserCountry(const std::string &userID, const std::string &countryID, const Countries::Type &type)
{
    std::string format = "HSET WNMA:user:%b country";
    if (type == Countries::Type::FROM) {
        format += "From %b";
    } else if (type == Countries::Type::LIVE) {
        format += "Live %b";
    } else {
        return;
    }
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, format.c_str(), userID.c_str(),
        userID.size(), countryID.c_str(), countryID.size()));
    freeReplyObject(reply);
}

void
Countries::redisRemUserCountry(const std::string &userID, const Countries::Type &type)
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
Countries::redisExistsUserCountry(const std::string &userID, const Countries::Type &type)
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
Countries::redisAddUserToCountrySet(const std::string &userID, const std::string &countryID, const Countries::Type &type)
{
    std::string format = "SADD WNMA:country:%b:";
    if (type == Countries::Type::FROM) {
        format += "from %b";
    } else if (type == Countries::Type::LIVE) {
        format += "live %b";
    } else {
        return;
    }
    
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, format.c_str(), countryID.c_str(),
        countryID.size(), userID.c_str(), userID.size()));
    freeReplyObject(reply);
}

// std::string("\u05C4")