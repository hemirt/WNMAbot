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
    
    int userID = this->userIDs.getID(user);
    if (userID == -1) {
        return;
    }
    int countryID = this->getCountryID(from);
    if (countryID == -1) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(this->accessMtx);
    
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HSET WNMA:user:%i countryFrom %i", userID, countryID))
    freeReplyObject(reply);
    
    this->addUserFromCountry(userID, countryID);
}

void
Countries::setLive(std::string user, std::string living)
{
    changeToLower(living);
    changeToLower(user);
    
    int userID = this->userIDs.getID(user);
    if (userID == -1) {
        return;
    }
    int countryID = this->getCountryID(living);
    if (countryID == -1) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HSET WNMA:user:%i countryLive %i", id, countryID))
    freeReplyObject(reply);
    
    this->addUserLiveCountry(userID, countryID)
}

void
Countries::addUserFromCountry(int userID, int countryID)
{
    // must be already called under lock(this->accessMtx)
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SADD WNMA:country:%i:from %i", countryID, userID));
    freeReplyObject(reply);
}

void
Countries::addUserLiveCountry(int userID, int countryID)
{
    // must be already called under lock(this->accessMtx)
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SADD WNMA:country:%i:live %i", countryID, userID));
    freeReplyObject(reply);
}


std::vector<std::string>
Countries::usersFrom(const std::string &country)
{
    std::vector<std::string> users;
    int countryID = this->getCountryID(country);
    if (countryID == -1) {
        return users;
    }
    
    std::lock_guard<std::mutex> lock(this->accessMtx);
    
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SMEMBERS WNMA:country:%i:from"));
    
    if (reply == NULL || reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return users;
    }
    
    for (int i = 0; i < reply->len; i++) {
        std::string username = this->userIDs.getUserName(reply->integer);
        if (!username.empty()) {
            // how to tell vector that it can steal the username string? TriHard
            users.push_back(username);
        }
    }
    
    return users;
}

std::vector<std::string>
Countries::usersLive(const std::string &country)
{
    std::vector<std::string> users;
    int countryID = this->getCountryID(country);
    if (countryID == -1) {
        return users;
    }
    
    std::lock_guard<std::mutex> lock(this->accessMtx);
    
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "SMEMBERS WNMA:country:%i:live"));
    
    if (reply == NULL || reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return users;
    }
    
    for (int i = 0; i < reply->len; i++) {
        std::string username = this->userIDs.getUserName(reply->integer);
        if (!username.empty()) {
            // how to tell vector that it can steal the username string? TriHard
            users.push_back(username);
        }
    }
    
    return users;
}

int
Countries::getCountryID(const std::string &country)
{
    int countryID;
    std::string lcCountry = changeToLower_copy(country);
    
    std::lock_guard<std::mutex> lock(this->accessMtx);
    
    // get countries id
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET WNMA:countries %b", lcCountry.c_str(), lcCountry.size()));
    
    if(reply == NULL) {
        // error
        return -1;
    } else if(reply->type != REDIS_REPLY_INTEGER) {
        // country is not in the database, create it
        freeReplyObject(reply);
        
        // get next countryID
        // probably like this?
        reply = static_cast<redisReply *>(redisCommand(
            this->context, "INCR WNMA:nextcountryid"));
        if (reply == NULL || reply->type != REDIS_REPLY_INTEGER) {
            return -1;
        }
        countryID = reply->integer;
        freeReplyObject(reply);
        
        // add a country record to WNMA:countries name - id
        reply = static_cast<redisReply *>(redisCommand(
            this->context, "HSET WNMA:countries %b %i", lcCountry.c_str(), lcCountry.size(), countryID));
        freeReplyObject(reply);
        
        // create WNMA:country:ID:displayname
        reply = static_cast<redisReply *>(redisCommand(
            this->context, "HSET WNMA:country:%i:displayname %b", country.c_str(), country.size()));
        freeReplyObject(reply);
        
        return countryID;
    } else {
        // country exists, return id
        countryID = reply->integer;
        freeReplyObject(reply);
        return countryID;
    }    
}

void
Countries::deleteFrom(std::string user)
{
    changeToLower(user);
    int userID = this->userIDs.getID(user);
    if (userID == -1) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(this->accessMtx);
    
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HDEL WNMA:user:%i countryFrom", userID));
    freeReplyObject(reply);
}

void
Countries::deleteLive(std::string user)
{
    changeToLower(user);
    int userID = this->userIDs.getID(user);
    if (userID == -1) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(this->accessMtx);
    
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HDEL WNMA:user:%i countryLive", userID));
    freeReplyObject(reply);
}

std::string
Countries::getFrom(std::string user)
{
    changeToLower(user);
    int userID = this->userIDs.getID(user);
    if (userID == -1) {
        return std::string();
    }
    
    std::lock_guard<std::mutex> lock(this->accessMtx);
    
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET WNMA:user:%i countryFrom", userID));
    if (reply == NULL || reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return std::string();
    }
    
    int countryID = reply->integer;
    freeReplyObject(reply);
    
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET:country:%i:displayname", countryID));
    if(reply == NULL || reply->type != REDIS_REPLY_STRING) {
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
    int userID = this->userIDs.getID(user);
    if (userID == -1) {
        return std::string();
    }
    
    std::lock_guard<std::mutex> lock(this->accessMtx);
    
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET WNMA:user:%i countryLive", userID));
    if (reply == NULL || reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return std::string();
    }
    
    int countryID = reply->integer;
    freeReplyObject(reply);
    
    reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET:country:%i:displayname", countryID));
    if(reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return std::string();
    }
    
    std::string countryDisplayName(reply->str, reply->len);
    freeReplyObject(reply);
    return countryDisplayName;
}

// std::string("\u05C4")