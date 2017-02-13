#include "afkers.hpp"

bool
Afkers::isAfker(const std::string &user)
{
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    return this->afkersMap.count(user) == 1;
}

void
Afkers::setAfker(const std::string &user, const std::string &message)
{
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    this->afkersMap[user] = {true, message, std::chrono::steady_clock::now()};
}

void
Afkers::updateAfker(const std::string &user, const Afkers::Afk &afk)
{
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    this->afkersMap[user] = afk;
}

void
Afkers::removeAfker(const std::string &user)
{
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    this->afkersMap.erase(user);
}

Afkers::Afk
Afkers::getAfker(const std::string &user)
{
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    if (this->afkersMap.count(user) == 0) {
        return Afkers::Afk();
    }
    return this->afkersMap.at(user);
}

std::string
Afkers::getAfkers()
{
    std::string afkers;
    std::lock_guard<std::mutex> lock(this->afkersMapMtx);
    for (const auto &i : this->afkersMap) {
        afkers += i.first + ", ";
    }

    if (!afkers.empty()) {
        afkers.erase(afkers.end() - 2, afkers.end());
    }

    return afkers;
}