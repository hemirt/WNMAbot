#include "userids.hpp"
#include "utilities.hpp"
#include "databasehandle.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <chrono>
#include <numeric>
namespace pt = boost::property_tree;

redisContext *UserIDs::context = nullptr;
std::mutex UserIDs::accessMtx;
std::mutex UserIDs::curlMtx;
CURL *UserIDs::curl = nullptr;

std::mutex UserIDs::channelsULMtx;
std::vector<UserList> UserIDs::channelsUserList;
std::mutex UserIDs::usersPendingMtx;
std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> UserIDs::usersPending;


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
    
    /* moved into TablesInitialize::initTables in ConnectionHandler::start();
    auto& db = DatabaseHandle::get();
    {
        hemirt::DB::Query<hemirt::DB::MariaDB::Values> q(
            "CREATE TABLE IF NOT EXISTS `users` (`id` INT(8) UNSIGNED UNIQUE NOT NULL AUTO_INCREMENT, `userid` VARCHAR(64) UNIQUE NOT NULL, `username` VARCHAR(64) NOT NULL, `displayname` VARCHAR(128) NOT NULL, `level` INT(4) NOT NULL DEFAULT 0, PRIMARY KEY(`id`))");
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
    /**/
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

bool
UserIDs::exists(const std::string &userid)
{
    return false;
    // not yet implemented
    
    auto& db = DatabaseHandle::get();
    hemirt::DB::Query q("SELECT EXISTS(SELECT 1 FROM `users` WHERE `userid` = \'" + userid + "\')");
    q.type = hemirt::DB::QueryType::RAWSQL;
    
    auto res = db.executeQuery(q);
    if (auto rval = res.returned(); rval) {
        if (rval->data.empty()) {
            std::cout << "UserIDs::exists userid: " << userid << ": rval empty" <<std::endl;
            return false;
        }
        /* template:
        for (const auto& row : rval->data) {
            for (const auto& column : row) {
                if (column.first) {
                    std::cout << column.second << " ";
                } else {
                    std::cout << "NULL";
                }
            }
        }
        */
        if (rval->data[0].empty()) {
            std::cout << "UserIDs::exists userid: " << userid << ": rval row has no columns" <<std::endl;
            return false;
        }
        if (!rval->data[0][0].first) {
            std::cout << "UserIDs::exists userid: " << userid << ": column is null" <<std::endl;
            return false;
        }
        return std::atoi(rval->data[0][0].second.c_str());
    } else if (auto eval = res.error(); eval) { 
        std::cout << "UserIDs::exists userid: " << userid << " error: " << eval->error() << std::endl;
    }
    return false;
}

int
UserIDs::insertUpdateUser(const User& user, const std::string &channelname)
{
    int ret = -1;
    {
        std::lock_guard<std::mutex> l(this->channelsULMtx);
        bool ret = false;
        for (auto& userlist : this->channelsUserList) {
            if (std::unordered_set<User>::iterator indb = userlist.userList.find(user); indb != userlist.userList.end()) {
                if (indb->username == user.username && indb->displayname == user.displayname) {
                    ret = true;
                } else {
                    indb->username = user.username;
                    indb->displayname = user.displayname;
                }
            }
        }
        
        if (auto ul = std::find_if(this->channelsUserList.begin(), this->channelsUserList.end(), [channelname](const auto& userlist) {
            return userlist.channelName == channelname;
        }); ul != this->channelsUserList.end()) {
            ul->userList.insert(user);
        } else {
            UserList usl;
            usl.channelName = channelname;
            usl.userList.insert(user);
            this->channelsUserList.push_back(std::move(usl));        
        }
        
        if (ret) {
            return 0;
        }
    }
    {
        std::lock_guard<std::mutex> l(this->usersPendingMtx);
        auto& first = std::get<0>(this->usersPending);
        auto& second = std::get<1>(this->usersPending);
        auto& third = std::get<2>(this->usersPending);
        first.push_back(user.userid);
        second.push_back(user.username);
        third.push_back(user.displayname);
        if (first.size() > 100) {
            ret = this->insertUpdateUserDB();
            first.clear();
            second.clear();
            third.clear();
        }
    }
    return ret;
}

int
UserIDs::insertUpdateUserDB()
{
    auto& db = DatabaseHandle::get();
    hemirt::DB::Query q("INSERT INTO `users` (`userid`, `username`, `displayname`) VALUES (?, ?, ?) ON DUPLICATE KEY UPDATE `userid` = VALUES(`userid`), `username` = VALUES(`username`), `displayname` = VALUES(`displayname`)");
    q.type = hemirt::DB::QueryType::PARAMETER;
    std::cout << std::get<0>(this->usersPending).size();
    q.setBuffer(std::get<0>(this->usersPending), std::get<1>(this->usersPending), std::get<2>(this->usersPending));
    
    auto res = db.executeQuery(q);
    if (auto eval = res.error(); eval) {
        std::string err = "Inserting into `users` error: ";
        err += eval->error();
        std::cerr << err << std::endl;
        return -1;
    } else if (auto aval = res.affected(); aval) {
        return aval->affected();
    } else {
        std::string err = "Inserting into `users` error.";
        std::cerr << err << std::endl;
        return -2;
    }
}

void
UserIDs::addUser(const std::string &user, const std::string &userid, const std::string &displayname, const std::string &channelname)
{
    if (!userid.empty() && !displayname.empty()) {
        // add to mysql
        auto t1 = std::chrono::steady_clock::now();
        insertUpdateUser(User(userid, user, displayname), channelname);
        auto t2 = std::chrono::steady_clock::now();
        std::cout << "user: " << user << " took: " << std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count() << " ns" << std::endl;
    }
    
    if (this->isUser(user)) {
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
    if (!this->isUser(user)) {
        return "error";
    }
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