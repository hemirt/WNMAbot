#ifndef COUNTRIES_HPP
#define COUNTRIES_HPP

#include "userids.hpp"

#include <hiredis/hiredis.h>
#include <mutex>
#include <string>
#include <vector>


class Countries
{
public:
    void setFrom(std::string user, std::string country);
    void setLive(std::string user, std::string living);
    
    void deleteFrom(std::string user);
    void deleteLive(std::string user);
    
    int getCountryID(const std::string &country);
    std::string getFrom(std::string user);
    std::string getLive(std::string user);
    
    std::vector<std::string> usersFrom(const std::string &country);
    std::vector<std::string> usersLive(const std::string &country);
    
    static Countries &
    getInstance()
    {
        return this->instance;
    }
    Countries(Countries const &) = delete;
    void operator=(Countries const &) = delete;

private:
    Countries();
    ~Countries();
    static redisContext *context;
    static std::mutex accessMtx;
    static Countries instance;
    UsersIDs &userIDs;
    
    void addUserFromCountry(const std::string &userIDstr, const std::string &countryIDstr)
    void addUserLiveCountry(const std::string &userIDstr, const std::string &countryIDstr)

};

#endif
