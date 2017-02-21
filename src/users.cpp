#include "users.hpp"
#include <algorithm>
#include <iostream>
#include "utilities.hpp"

Users::Users()
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

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGETALL WNMA:users"));
    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return;
    } else if (reply->type == REDIS_REPLY_ARRAY) {
        for (int i = 0; i < reply->elements; i += 2) {
            std::string user(reply->element[i]->str, reply->element[i]->len);

            std::string country(reply->element[i + 1]->str,
                                reply->element[i + 1]->len);
            this->countries.insert(country);
            this->users[user] = country;
        }
    }
    freeReplyObject(reply);
}

Users::~Users()
{
    if (!this->context)
        return;
    redisFree(this->context);
}

void
Users::setUser(std::string &user, std::string &country)
{
    changeToLower(country);
    changeToLower(user);
    std::unique_lock<std::shared_mutex> lock(containersMtx);
    this->countries.insert(country);
    auto search = this->users.find(user);
    if (search != this->users.end()) {
        std::string oldCountry = search->second;
        this->users[user] = country;
        auto found = std::find_if(
            users.begin(), users.end(),
            [oldCountry](const std::pair<std::string, std::string> &p) -> bool {
                return p.second == oldCountry;
            });
        if (found == users.end()) {
            countries.erase(oldCountry);
        }
    } else {
        this->users[user] = country;
    }

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HSET WNMA:users %b %b", user.c_str(),
                     user.size(), country.c_str(), country.size()));
    freeReplyObject(reply);
}

std::string
Users::getUsersCountry(std::string &user)
{
    changeToLower(user);
    std::shared_lock<std::shared_mutex> lock(containersMtx);
    auto it = users.find(user);
    if (it != users.end()) {
        return it->second;
    } else {
        return "NONE";
    }
}

std::string
Users::getCountries()
{
    std::string countriesString;
    std::shared_lock<std::shared_mutex> lock(containersMtx);
    for (const auto &i : this->countries) {
        countriesString += i + ", ";
    }

    if (countriesString.back() == ' ') {
        countriesString.pop_back();
        countriesString.pop_back();
    }

    return countriesString;
}

std::string
Users::getUsersFrom(std::string &country)
{
    changeToLower(country);
    std::string usersString;
    std::shared_lock<std::shared_mutex> lock(containersMtx);
    for (const auto &i : users) {
        if (i.second == country) {
            usersString += i.first + ", ";
        }
    }

    if (usersString.back() == ' ') {
        usersString.pop_back();
        usersString.pop_back();
    }

    return usersString;
}

void
Users::printAllCout()
{
    std::shared_lock<std::shared_mutex> lock(containersMtx);
    std::cout << "User Country" << std::endl;
    for (const auto &i : users) {
        std::cout << i.first << " " << i.second << std::endl;
    }
}
