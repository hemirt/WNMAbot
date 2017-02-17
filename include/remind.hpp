#ifndef REMIND_HPP
#define REMIND_HPP

#include <stdint.h>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <shared_mutex>

struct Reminder
{
    std::string from;
    std::string to;
    std::string what;
    size_t id;
    std::chrono::time_point<std::chrono::system_clock> when;
};

class RemindsHandler
{
public: 
    RemindsHandler(); // load Reminders from redis
    size_t count(const std::string &from);
    std::vector<Reminder> getRemindersFrom(const std::string &from);
    std::vector<Reminder> getRemindersTo(const std::string &to);
    bool addReminder(const std::string &from, const std::string &to, const std::string &what, int64_t seconds);
    void removeReminder(const std::string& from, const std::string &to, size_t id);
    void removeAllRemindersFrom(const std::string &from);
    void removeAllRemindersTo(const std::string &to);
    
private:
    std::vector<Reminder> reminders;
    std::shared_mutex vecMtx;
    redisContext *redisContext;
    std::mutex redisMtx;
};

#endif //REMIND_HPP