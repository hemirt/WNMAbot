#ifndef USERSIDS_HPP
#define USERSIDS_HPP

#include <curl/curl.h>
#include <hiredis/hiredis.h>
#include <mutex>
#include <string>
#include <cstddef>
#include <unordered_set>
#include <vector>

#include <iostream>

struct User
{
    std::string userid;
    mutable std::string username;
    mutable std::string displayname;
    
    bool operator==(const User& rhs) const
    {
        return userid == rhs.userid;
    }
};

namespace std
{
    template<>
    struct hash<User>
    {
        std::size_t operator()(const User& user) const noexcept
        {
            return std::hash<std::string>()(user.userid);
        }
    };
}

struct UserList
{
    std::string channelName;
    std::unordered_set<User> userList;
    bool operator<(const UserList& rhs) const
    {
        return channelName < rhs.channelName;
    }
};

class UserIDs
{
public:
    void addUser(const std::string &user, const std::string &userid, const std::string &displayname, const std::string &channelname);
    bool isUser(std::string user);
    int insertUpdateUser(const User& user, const std::string& channelname);
    int insertUpdateUserDB();
    bool exists(const std::string& userid);
    std::string getID(const std::string &user);
    std::string getUserName(const std::string &userIDstr);

    // Singleton
    static UserIDs &
    getInstance()
    {
        static UserIDs instance;
        return instance;
    }
    UserIDs(UserIDs const &) = delete;
    void operator=(UserIDs const &) = delete;

private:
    UserIDs();
    ~UserIDs();
    static std::mutex accessMtx;
    static std::mutex curlMtx;
    static redisContext *context;
    struct curl_slist *chunk = NULL;
    static CURL *curl;
    static std::mutex channelsULMtx;
    static std::vector<UserList> channelsUserList;
    static std::mutex usersPendingMtx;
    static std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> usersPending;
};

#endif
