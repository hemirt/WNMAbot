#ifndef USERS_HPP
#define USERS_HPP

#include <hiredis/hiredis.h>
#include <map>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>

class Users
{
public:
    struct Account {
        std::string from;
        std::string living;
    };
    Users();
    ~Users();
    void setUser(std::string &user, std::string &country);
    void setUserLiving(std::string &user, std::string &country);
    void deleteUser(std::string &user);
    std::string getUsersCountry(std::string &user);
    std::string getUsersLiving(std::string &country);
    std::string getUsersFrom(std::string &country);
    void printAllCout();

private:
    redisContext *context;
    std::shared_mutex containersMtx;
    std::map<std::string, Account> users;
};

#endif
