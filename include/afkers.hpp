#ifndef AFKERS_HPP
#define AFKERS_HPP

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include "utilities.hpp"

class Afkers
{
public:
    struct Afk {
        bool exists = false;
        std::string message;
        std::chrono::time_point<std::chrono::steady_clock> time;
    };

    bool isAfker(const std::string &user);
    void setAfker(const std::string &user, const std::string &message = "");
    void updateAfker(const std::string &user, const Afk &afk);
    void removeAfker(const std::string &user);
    Afk getAfker(const std::string &user);
    std::string getAfkers();

private:
    std::unordered_map<std::string, Afk> afkersMap;
    std::mutex afkersMapMtx;
};

#endif
