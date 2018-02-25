#include "userids.hpp"
#include "utilities.hpp"
#include "databasehandle.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <iostream>
#include <stdexcept>

namespace pt = boost::property_tree;

redisContext *UserIDs::context = nullptr;
std::mutex UserIDs::accessMtx;
std::mutex UserIDs::curlMtx;
CURL *UserIDs::curl = nullptr;
UserIDs UserIDs::instance;

UserIDs::UserIDs()
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "UserIDs error: " << this->context->errstr
                      << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "UserIDs can't allocate redis context" << std::endl;
        }
    }
    this->curl = curl_easy_init();
    if (curl) {
        this->chunk = curl_slist_append(
            chunk, "Accept: application/vnd.twitchtv.v5+json");
        this->chunk = curl_slist_append(
            chunk, "Client-ID: i9nh09d5sv612dts3fmrccimhq7yb2");
    } else {
        std::cerr << "CURL ERROR" << std::endl;
    }
    
    /* cannot get db, cause its inited on main, but this is a static instance
    auto& db = DatabaseHandle::get();
    {
        hemirt::DB::Query<hemirt::DB::MariaDB::Values> q(
            "CREATE TABLE IF NOT EXISTS `users` (`id` INT(8) UNIQUE NOT NULL UNSIGNED AUTO_INCREMENT, `userid` VARCHAR(64) UNIQUE NOT NULL, `username` VARCHAR(64) NOT NULL, `displayname` VARCHAR(128) NOT NULL, `level` INT(4) DEFAULT 0, PRIMARY KEY(`id`))");
        q.type = hemirt::DB::QueryType::RAWSQL;
        auto res = db.executeQuery(std::move(q));
        if (auto eval = res.error(); eval) {
            std::string err = "Creating table `users` error: ";
            err += eval->error();
            std::cerr << err << std::endl;
            throw std::runtime_error(err);
            return;
        }
    }
    */
}

UserIDs::~UserIDs()
{
    if (this->context) {
        redisFree(this->context);
    }
    curl_slist_free_all(this->chunk);
    curl_easy_cleanup(this->curl);
}

bool
UserIDs::isUser(std::string user)
{
    std::lock_guard<std::mutex> lock(this->accessMtx);

    changeToLower(user);
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HEXISTS WNMA:userids %b", user.c_str(), user.size()));
    if (reply == NULL && this->context->err) {
        std::cerr << "Userids error: " << this->context->errstr << std::endl;
        freeReplyObject(reply);
        return false;
    }
    
    if (reply == nullptr)
    {
        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return false;
    }

    if (reply->integer == 1) {
        freeReplyObject(reply);
        return true;
    } else {
        freeReplyObject(reply);
        return false;
    }
}

static size_t
WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

void
UserIDs::addUser(const std::string &user, const std::string &userid, const std::string &displayname)
{
    if (this->isUser(user)) {
        /*
        {
            if (userid.empty() || displayname.empty()) {
                return;
            }
            auto& db = DatabaseHandle::get();
            
            hemirt::DB::Query<hemirt::DB::MariaDB::Values> q("SELECT EXISTS(SELECT 1 FROM `users` WHERE `userid` = \'" + userid + "\'"
        }
        */
        return;
    }
    std::unique_lock<std::mutex> lock(this->curlMtx);

    char *escapedUser = curl_easy_escape(this->curl, user.c_str(), user.size());
    if (escapedUser == nullptr) {
        return;
    }
    std::string rawurl("https://api.twitch.tv/kraken/users?login=");
    rawurl += escapedUser;
    curl_free(escapedUser);

    std::string readBuffer;
    curl_easy_setopt(this->curl, CURLOPT_HTTPHEADER, this->chunk);
    curl_easy_setopt(this->curl, CURLOPT_URL, rawurl.c_str());
    curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(this->curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(this->curl);
    curl_easy_reset(curl);

    if (res) {
        std::cerr << "UserIDs::addUser(" + user + "): res: \"" << res << "\""
                  << std::endl;
        return;
    }

    if (readBuffer.empty()) {
        std::cerr << "UserIDs::addUser(" + user + ") readBuffer empty"
                  << std::endl;
        return;
    }

    pt::ptree tree;
    std::stringstream ss(readBuffer);
    pt::read_json(ss, tree);
    auto users = tree.get_child_optional("users");
    if (!users || users->empty()) {
        std::cout << "rawurl: " << rawurl
                  << "\njsontree child \"users\" empty:\n"
                  << readBuffer << std::endl;
        return;
    }
    std::string id;
    for (const auto &user : *users) {
        id = user.second.get("_id", "");
    }
    if (id.empty()) {
        std::cerr << "UserIDs::addUser(" + user +
                         ") couldnt find _id in tree:\n"
                  << readBuffer << std::endl;
        return;
    }

    lock = std::unique_lock<std::mutex>(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HSETNX WNMA:userids %b %b", user.c_str(),
                     user.size(), id.c_str(), id.size()));
    freeReplyObject(reply);
    reply = static_cast<redisReply *>(
        redisCommand(this->context, "HSET WNMA:user:%b username %b", id.c_str(),
                     id.size(), user.c_str(), user.size()));
    freeReplyObject(reply);
}

std::string
UserIDs::getID(const std::string &user)
{
    this->addUser(user);
    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(redisCommand(
        this->context, "HGET WNMA:userids %b", user.c_str(), user.size()));

    if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return std::string();
    }

    std::string id(reply->str, reply->len);
    freeReplyObject(reply);
    return id;
}

std::string
UserIDs::getUserName(const std::string &userIDstr)
{
    std::lock_guard<std::mutex> lock(this->accessMtx);

    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HGET WNMA:user:%b username",
                     userIDstr.c_str(), userIDstr.size()));

    if (reply == NULL || reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return std::string();
    }

    std::string username(reply->str, reply->len);
    freeReplyObject(reply);
    return username;
}