#ifndef REMINDUSERS_HPP
#define REMINDUSERS_HPP

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <memory>

class RemindUsers
{
public:
    struct Reminder {
        std::string toUser;
        std::string whichReminder;
        std::shared_ptr<boost::asio::steady_timer> timer;
    };
    RemindUsers() = default;
    ~RemindUsers();
    void addReminder(const std::string &fromUser, const std::string &toUser,
                     const std::string &which,
                     std::shared_ptr<boost::asio::steady_timer> timer);
    int count(const std::string &fromUser);
    void removeFirst(const std::string &fromUser);
    RemindUsers::Reminder getFirst(const std::string &fromUser);
    void remove(const std::string &fromUser, const std::string &toUser,
                const std::string &which);
    bool exists(const std::string &fromUser, const std::string &toUser,
                const std::string &which);

private:
    std::multimap<std::string, Reminder> reminders;
    std::mutex mtx;
};

#endif
