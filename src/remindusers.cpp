#include "remindusers.hpp"

void
RemindUsers::addReminder(const std::string &fromUser, const std::string &toUser,
                         const std::string &which,
                         std::shared_ptr<boost::asio::steady_timer> timer)
{
    std::lock_guard<std::mutex> lk(this->mtx);
    this->reminders.emplace(std::pair<std::string, RemindUsers::Reminder>(
        fromUser, {toUser, which, timer}));
    return;
}

int
RemindUsers::count(const std::string &fromUser)
{
    std::lock_guard<std::mutex> lk(this->mtx);
    return this->reminders.count(fromUser);
}

void
RemindUsers::removeFirst(const std::string &fromUser)
{
    std::lock_guard<std::mutex> lk(this->mtx);
    if (this->reminders.count(fromUser)) {
        auto range = this->reminders.equal_range(fromUser);
        this->reminders.erase(range.first);
    }
}

RemindUsers::Reminder
RemindUsers::getFirst(const std::string &fromUser)
{
    Reminder retValue;
    std::lock_guard<std::mutex> lk(this->mtx);
    if (this->reminders.count(fromUser)) {
        auto range = this->reminders.equal_range(fromUser);
        retValue = range.first->second;
    }
    return retValue;
}

void
RemindUsers::remove(const std::string &fromUser, const std::string &toUser,
                    const std::string &which)
{
    std::lock_guard<std::mutex> lk(this->mtx);
    if (this->reminders.count(fromUser)) {
        auto range = this->reminders.equal_range(fromUser);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->first == fromUser && it->second.toUser == toUser &&
                it->second.whichReminder == which) {
                this->reminders.erase(it);
                return;
            }
        }
    }
}

bool
RemindUsers::exists(const std::string &fromUser, const std::string &toUser,
                    const std::string &which)
{
    std::lock_guard<std::mutex> lk(this->mtx);
    if (this->reminders.count(fromUser)) {
        auto range = this->reminders.equal_range(fromUser);
        for (auto it = range.first; it != range.second; ++it) {
            if (it->first == fromUser && it->second.toUser == toUser &&
                it->second.whichReminder == which) {
                return true;
            }
        }
    }
    return false;
}