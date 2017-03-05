#ifndef COUNTRIES_HPP
#define COUNTRIES_HPP

#include "userids.hpp"

#include <hiredis/hiredis.h>
#include <mutex>
#include <string>
#include <vector>
#include <experimental/optional>

using std::experimental::optional;

class Countries
{
public:
    enum class Result {
        UNKNOWN,
        SUCCESS,
        NOUSER,
        NOCOUNTRY,
    };
    Result setFrom(std::string user, std::string country);
    Result setLive(std::string user, std::string living);

    void deleteFrom(std::string user);
    void deleteLive(std::string user);

    std::string getCountryID(const std::string &country);
    std::string createCountry(const std::string &country);
    
    std::string getFrom(std::string user);
    std::string getLive(std::string user);

    std::vector<std::string> usersFrom(const std::string &country);
    std::vector<std::string> usersLive(const std::string &country);

    static Countries &
    getInstance()
    {
        return instance;
    }
    Countries(Countries const &) = delete;
    void operator=(Countries const &) = delete;

private:

    Countries();
    ~Countries();
    static redisContext *context;
    static std::mutex accessMtx;
    static Countries instance;
    UserIDs &userIDs;

    enum class Type {
        UNKNOWN,
        FROM,
        LIVE,
    };
    
    optional<std::string> redisGetDisplayName(const std::string &countryIDstr);
    optional<std::string> redisGetUserCountry(const std::string &userID, const Countries::Type &type);
    void redisRemUserFromCountry(const std::string &userID, const std::string &countryID, const Countries::Type &type);
    void redisSetUserCountry(const std::string &userID, const std::string &countryID, const Countries::Type &type);
    void redisRemUserCountry(const std::string &userID, const Countries::Type &type);
    bool redisExistsUserCountry(const std::string &userID, const Countries::Type &type);
    void redisAddUserToCountrySet(const std::string &userID, const std::string &countryID, const Countries::Type &type);

};

#endif
