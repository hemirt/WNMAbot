#ifndef USERSIDS_HPP
#define USERSIDS_HPP

#include <curl/curl.h>
#include <hiredis/hiredis.h>
#include <mutex>
#include <string>

class UserIDs
{
public:
    void addUser(const std::string &user, const std::string &userid = std::string(), const std::string &displayname = std::string());
    bool isUser(std::string user);
    std::string getID(const std::string &user);
    std::string getUserName(const std::string &userIDstr);

    // Singleton
    static UserIDs &
    getInstance()
    {
        return instance;
    }
    UserIDs(UserIDs const &) = delete;
    void operator=(UserIDs const &) = delete;

private:
    UserIDs();
    ~UserIDs();
    static UserIDs instance;
    static std::mutex accessMtx;
    static std::mutex curlMtx;
    static redisContext *context;
    struct curl_slist *chunk = NULL;
    static CURL *curl;
};

#endif
