#include "ignore.hpp"
#include "utilities.hpp"

void
Ignore::addUser(std::string user)
{
    std::lock_guard<std::mutex> lock(this->access);
    changeToLower(user);
    if (user == "hemirt") {
        // NaM
        return;
    }
    this->ignoredUsers.insert(user);
}

void
Ignore::removeUser(std::string user)
{
    std::lock_guard<std::mutex> lock(this->access);
    changeToLower(user);
    this->ignoredUsers.erase(user);
}

bool
Ignore::isIgnored(std::string user)
{
    std::lock_guard<std::mutex> lock(this->access);
    changeToLower(user);
    return this->ignoredUsers.count(user) == 1;
}