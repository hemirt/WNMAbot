#ifndef IGNORE_HPP
#define IGNORE_HPP

#include <unordered_set>
#include <string>
#include <mutex>

class Ignore
{
public:
    Ignore();
    void addUser(std::string user);
    void removeUser(std::string user);
    bool isIgnored(std::string user);
};

#endif