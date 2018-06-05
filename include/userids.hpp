#ifndef USERSIDS_HPP
#define USERSIDS_HPP

#include <curl/curl.h>
#include <hiredis/hiredis.h>
#include <mutex>
#include <string>
#include <cstddef>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include "date/tz.h"
#include <forward_list>


#include <iostream>

struct User
{
    User(std::string uid, std::string uname, std::string dname)
        : userid(std::move(uid))
        , username(std::move(uname))
        , displayname(std::move(dname))
        , channels()
    {}
    User(User&& u)
        : userid(std::move(const_cast<std::string&>(u.userid)))
        , username(std::move(u.username))
        , displayname(std::move(u.displayname))
        , channels(std::move(u.channels))
    {}
    User(const User& u)
        : userid(u.userid)
        , username(u.username)
        , displayname(u.displayname)
        , channels(u.channels)
    {}
    User& operator=(User&&) = delete;
    User& operator=(const User&) = delete;
    ~User() = default;
    
    const std::string userid;
    mutable std::string username;
    mutable std::string displayname;
    struct ChannelActivity
    {
        ChannelActivity(const std::string& chn, date::utc_clock::time_point tp)
            : channelName(chn)
            , time(std::move(tp))
        {}
        ChannelActivity(std::string&& chn, date::utc_clock::time_point tp)
            : channelName(std::move(chn))
            , time(std::move(tp))
        {}
        ChannelActivity(ChannelActivity&& cha)
            : channelName(std::move(const_cast<std::string&>(cha.channelName)))
            , time(std::move(cha.time.load()))
        {}
        ChannelActivity(const ChannelActivity& cha)
            : channelName(cha.channelName)
            , time(cha.time.load())
        {}
        ChannelActivity& operator=(ChannelActivity&&) = delete;
        ChannelActivity& operator=(const ChannelActivity&) = delete;
        ~ChannelActivity() = default;
        
        const std::string channelName;
        mutable std::atomic<date::utc_clock::time_point> time;
        
        bool operator==(const ChannelActivity& rhs) const
        {
            return channelName == rhs.channelName;
        }
    };
    mutable std::forward_list<ChannelActivity> channels;
    
    void updateChannelActivity(std::string&& channelName, date::utc_clock::time_point tp) const
    {
        if (auto it = std::find_if(this->channels.begin(), this->channels.end(), [&channelName](const ChannelActivity& cha) {
            return cha.channelName == channelName;
        }); it != this->channels.end()) {
            it->time = std::move(tp);
        } else {
            this->channels.push_front({std::move(channelName), std::move(tp)});
        }
    }
    
    void updateChannelActivity(const std::string& channelName, date::utc_clock::time_point tp) const
    {
        this->updateChannelActivity(channelName, std::move(tp));
    }
    
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
