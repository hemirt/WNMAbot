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
    
    // get old countryFrom value
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET WNMA:user:%b countryFrom", userIDstr.c_str(),
        userIDstr.size()));
    if (reply->type == REDIS_REPLY_STRING) {
        std::string prevCountryID(reply->str, reply->len);
        freeReplyObject(reply);
        
        // remove myself from old country:from set
        reply = static_cast<redisReply *>(redisCommand(
            this->context, "SREM WNMA:country:%b:from %b", prevCountryID.c_str(),
            prevCountryID.size(), userIDstr.c_str(), userIDstr.size()));
    }
    freeReplyObject(reply);

    // set new countryFrom value
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "HSET WNMA:user:%b countryFrom %b", userIDstr.c_str(),
        userIDstr.size(), countryIDstr.c_str(), countryIDstr.size()));
    freeReplyObject(reply);
    
    // do I live somewhere?
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "HEXISTS WNMA:user:%b countryLive",
        userIDstr.c_str(), userIDstr.size()));
    if(reply->integer == 0) {
        freeReplyObject(reply);
        
        // I dont live anywhere, then i'll live in the country where i'm from
        // adding myself to country:live set
        reply = static_cast<redisReply *>(redisCommand(
            this->context, "SADD WNMA:country:%b:live %b", countryIDstr.c_str(),
            countryIDstr.size(), userIDstr.c_str(), userIDstr.size()));    
        freeReplyObject(reply);
        
        // and setting countryLive to the same country
        reply = static_cast<redisReply *>(redisCommand(
            this->context, "HSET WNMA:user:%b countryLive %b", userIDstr.c_str(),
            userIDstr.size(), countryIDstr.c_str(), countryIDstr.size()));
    }
    freeReplyObject(reply);
    
    // add myself to country:from set
    this->addUserFromCountry(userIDstr, countryIDstr);
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

    // get old countryLive value
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET WNMA:user:%b countryLive", userIDstr.c_str(),
        userIDstr.size()));
    if (reply->type == REDIS_REPLY_STRING) {
        std::string prevCountryID(reply->str, reply->len);
        freeReplyObject(reply);
        
        // remove myself from old country:live set
        reply = static_cast<redisReply *>(redisCommand(
            this->context, "SREM WNMA:country:%b:live %b", prevCountryID.c_str(),
            prevCountryID.size(), userIDstr.c_str(), userIDstr.size()));
    }
    freeReplyObject(reply);
    
    // set new countryLive value
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "HSET WNMA:user:%b countryLive %b", userIDstr.c_str(),
        userIDstr.size(), countryIDstr.c_str(), countryIDstr.size()));
    freeReplyObject(reply);

    // add myself to new country:list set
    this->addUserLiveCountry(userIDstr, countryIDstr);
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
    
    // get old countryFrom value
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:user:%b countryFrom",
                     userIDstr.c_str(), userIDstr.size()));
    if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return;
    }
  
    std::string countryIDstr(reply->str, reply->len);
    freeReplyObject(reply);
    
    // delete countryFrom
    reply = static_cast<redisReply *>(
        redisCommand(this->context, "HDEL WNMA:user:%b countryFrom",
                     userIDstr.c_str(), userIDstr.size()));
    freeReplyObject(reply);
    
    // delete myself from the country:from set
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "SREM WNMA:country:%b:from %b", countryIDstr.c_str(), countryIDstr.size(),
                     userIDstr.c_str(), userIDstr.size()));
    freeReplyObject(reply);

    // get countryLive
    std::string countryLive;
    reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:user:%b countryLive",
        userIDstr.c_str(), userIDstr.size()));
    if (reply->type == REDIS_REPLY_STRING) {
        countryLive = std::string(reply->str, reply->len);
    }
    freeReplyObject(reply);
    
    // if i live where im from
    if (countryLive == countryIDstr) {
        reply = static_cast<redisReply *>(redisCommand(
            this->context, "SREM WNMA:country:%b:live %b", countryIDstr.c_str(),
            countryIDstr.size(), userIDstr.c_str(), userIDstr.size()));
        freeReplyObject(reply);
        
        reply = static_cast<redisReply *>(redisCommand(
            this->context, "HDEL WNMA:user:%b countryLive",
            userIDstr.c_str(), userIDstr.size()));
        freeReplyObject(reply);
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
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:user:%b countryLive",
                     userIDstr.c_str(), userIDstr.size()));
    if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return;
    }
    
    std::string countryIDstr(reply->str, reply->len);
    freeReplyObject(reply);
    
    // delete countryLive value
    reply = static_cast<redisReply *>(
        redisCommand(this->context, "HDEL WNMA:user:%b countryLive",
                     userIDstr.c_str(), userIDstr.size()));
    freeReplyObject(reply);
    
    // remove myself from country:live set
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "SREM WNMA:country:%b:live %b", countryIDstr.c_str(), countryIDstr.size(),
                     userIDstr.c_str(), userIDstr.size()));
    freeReplyObject(reply);
    
    // if i come from somewhere, i need to set my live to there too
    
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET WNMA:user:%b countryFrom",
        userIDstr.c_str(), userIDstr.size()));
    if(reply->type ==  REDIS_REPLY_STRING) {
        std::string countryIComeFrom(reply->str, reply->len);
        freeReplyObject(reply);
        
        // i come from somewhere
        // set new countryLive value
        
        reply = static_cast<redisReply *>(redisCommand(
            this->context, "HSET WNMA:user:%b countryLive %b", userIDstr.c_str(),
            userIDstr.size(), countryIComeFrom.c_str(), countryIComeFrom.size()));
        freeReplyObject(reply);
        
        // add myself to new country:list set
        this->addUserLiveCountry(userIDstr, countryIComeFrom);
    } else {
        freeReplyObject(reply);
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
        redisCommand(this->context, "GET WNMA:country:%b:displayname",
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

    std::string countryIDstr(reply->str, reply->len);
    freeReplyObject(reply);

    reply = static_cast<redisReply *>(
        redisCommand(this->context, "GET WNMA:country:%b:displayname",
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