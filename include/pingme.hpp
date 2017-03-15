#ifndef PINGME_HPP
#define PINGME_HPP

#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include "hiredis/hiredis.h"

class PingMe
{
public:
    static PingMe &
    getInstance()
    {
        return instance;
    }
    ~PingMe();
    bool isPinger(const std::string &name);
    std::map<std::string, std::vector<std::string>> ping(
        const std::string &bywhom);
    void addPing(const std::string &who, const std::string &bywhom,
                 const std::string &channel);

    PingMe(PingMe const &) = delete;
    void operator=(PingMe const &) = delete;
    static std::map<std::string, std::map<std::string, std::string>> pingMap;

private:
    PingMe();
    static PingMe instance;
    static redisContext *context;
    static std::mutex access;
};

#endif