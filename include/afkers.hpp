#ifndef AFKERS_HPP
#define AFKERS_HPP

#include <hiredis/hiredis.h>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

class Afkers
{
public:
    struct Afk {
        bool exists = false;
        std::string message;
        std::chrono::time_point<std::chrono::system_clock> time;
        bool announce = true;
    };

    Afkers();
    ~Afkers();
    bool isAfker(const std::string &user);
    void setAfker(const std::string &user, const std::string &message = "");
    void setShadowAfker(const std::string &user, const std::string &message = "");
    void updateAfker(const std::string &user, const Afk &afk);
    void removeAfker(const std::string &user);
    Afk getAfker(const std::string &user);
    std::string getAfkers();

private:
    std::unordered_map<std::string, Afk> afkersMap;
    std::mutex afkersMapMtx;
    redisContext *context;
    void setAfkerRedis(const std::string &user, const Afk &afk);
    void removeAfkerRedis(const std::string &user);
    void getAllAfkersRedis();
};

#endif
