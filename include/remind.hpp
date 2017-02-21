#ifndef REMIND_HPP
#define REMIND_HPP

#include <stdint.h>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

struct Reminder {
    std::string from;
    std::string to;
    std::string what;
    size_t id;
    std::chrono::time_point<std::chrono::system_clock> when;
    // i guess this has to be shared? check
    std::shared_ptr<boost::asio::steady_timer> timer;
};

class RemindsHandler
{
public:
    RemindsHandler(
        boost::asio::io_service &_ioService);  // load Reminders from redis
    size_t count(const std::string &from);
    std::vector<Reminder> getRemindersFrom(const std::string &from);
    std::vector<Reminder> getRemindersTo(const std::string &to);
    bool addReminder(const std::string &from, const std::string &to,
                     const std::string &what, int64_t seconds);
    void removeReminder(const std::string &from, const std::string &to,
                        size_t id);
    void removeAllRemindersFrom(const std::string &from);
    void removeAllRemindersTo(const std::string &to);
    void removeFromRedis(const Reminder &reminder);
    void saveToRedis(const Reminder &reminder);
    void loadFromRedis();

private:
    std::vector<Reminder> reminders;
    std::shared_mutex vecMtx;
    redisContext *redisContext;
    std::mutex redisMtx;
    boost::asio::io_service &ioService;
};

#endif  // REMIND_HPP