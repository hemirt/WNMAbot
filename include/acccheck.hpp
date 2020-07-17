#ifndef ACCCHECK_HPP
#define ACCCHECK_HPP

#include <chrono>
#include <string>
#include <mutex>

#include "rapidjson/document.h"
#include "date/date.h"
#include <iostream>
#include "userids.hpp"
#include <hiredis/hiredis.h>

#include <curl/curl.h>

class AccCheck
{
public:
    static void init();
    static void deinit();
    
    template<typename T = std::chrono::seconds>
    static T ageByUsername(const std::string &username);
    template<typename T = std::chrono::seconds>
    static T ageById(const std::string &id);
    
    static bool isAllowed(const std::string &username);
    static void addAllowed(const std::string &username);
    static void delAllowed(const std::string &username);

private:
    static void reconnect();
    static inline std::mutex redisMtx = std::mutex{};
    static inline curl_slist *chunk = nullptr;
    static inline std::mutex mtx = std::mutex{};
    static inline CURL *curl = nullptr;
    static inline redisContext * context = nullptr;
    
};

namespace {
    static size_t
    WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        ((std::string *)userp)->append((char *)contents, size * nmemb);
        return size * nmemb;
    }

    static date::sys_seconds
    parse(const std::string& str)
    {
        std::istringstream in(str);
        date::sys_seconds tp;
        in >> date::parse("%FT%T", tp);
        return tp;
    }
}

template<typename T>
T AccCheck::ageByUsername(const std::string& username)
{
    static auto& userids = UserIDs::getInstance();
    auto user = userids.getID(username);
    if (user.empty() || user == "error") {
        return T{0};
    }
    return AccCheck::ageById<T>(user);
}

template<typename T>
T AccCheck::ageById(const std::string& user)
{
    std::unique_lock lk(AccCheck::mtx);
    
    std::string rawurl = "https://api.twitch.tv/kraken/users/" + user;
    std::string readBuffer;
    curl_easy_setopt(AccCheck::curl, CURLOPT_HTTPHEADER, AccCheck::chunk);
    curl_easy_setopt(AccCheck::curl, CURLOPT_URL, rawurl.c_str());
    curl_easy_setopt(AccCheck::curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(AccCheck::curl, CURLOPT_WRITEDATA, &readBuffer);
    
    CURLcode res = curl_easy_perform(AccCheck::curl);
    curl_easy_reset(curl);

    if (res) {
        std::cerr << "AccCheck::age(" + user + "): res: \"" << res << "\""
                  << std::endl;
        return T{0};
    }
    
    if (readBuffer.empty()) {
        std::cerr << "AccCheck::age(" + user + ") readBuffer empty"
                  << std::endl;
        return T{0};
    }
    
    lk.unlock();
    
    rapidjson::Document d;
    d.Parse(readBuffer.c_str());
    auto it = d.FindMember("created_at");
    if (it == d.MemberEnd() || !it->value.IsString()) {
        std::cerr << "AccCheck::age created_at NOT A string" << std::endl;
        return T{0};
    }
    std::string time = it->value.GetString();
    
    auto t1 = parse(time);
    date::sys_seconds t0 = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
    
    return std::chrono::duration_cast<T>(t0 - t1);
}

#endif // ACCCHECK_HPP