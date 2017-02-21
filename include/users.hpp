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
    Users();
    ~Users();
    void setUser(std::string &user, std::string &country);
    std::string getUsersCountry(std::string &user);
    std::string getCountries();
    std::string getUsersFrom(std::string &country);
    void printAllCout();

private:
    redisContext *context;
    std::shared_mutex containersMtx;
    std::map<std::string, std::string> users;
    std::set<std::string> countries;
};

#endif
