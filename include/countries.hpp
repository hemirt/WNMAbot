#ifndef COUNTRIES_HPP
#define COUNTRIES_HPP

#include "userids.hpp"

#include <hiredis/hiredis.h>
#include <experimental/optional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

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
    std::string getCountryName(const std::string &countryID);
    std::string createCountry(const std::string &country);
    bool existsCountry(const std::string &countryID);
    bool deleteCountry(const std::string &countryID);
    bool renameCountry(const std::string &countryID,
                       const std::string &newCountryName);
    bool createAlias(const std::string &countryID, const std::string &country);
    bool deleteAlias(const std::string &countryID, const std::string &country);

    std::string getFrom(std::string user);
    std::string getLive(std::string user);

    std::pair<std::string, std::vector<std::string>> usersFrom(
        const std::string &country);
    std::pair<std::string, std::vector<std::string>> usersLive(
        const std::string &country);

    static Countries &
    getInstance()
    {
        static Countries instance;
        return instance;
    }
    Countries(Countries const &) = delete;
    void operator=(Countries const &) = delete;

private:
    Countries();
    ~Countries();
    static redisContext *context;
    static std::mutex accessMtx;
    UserIDs &userIDs;

    enum class Type {
        UNKNOWN,
        FROM,
        LIVE,
    };

    std::experimental::optional<std::string> redisGetDisplayName(
        const std::string &countryIDstr);
    std::experimental::optional<std::string> redisGetUserCountry(
        const std::string &userID, const Countries::Type &type);
    void redisRemUserFromCountry(const std::string &userID,
                                 const std::string &countryID,
                                 const Countries::Type &type);
    void redisSetUserCountry(const std::string &userID,
                             const std::string &countryID,
                             const Countries::Type &type);
    void redisRemUserCountry(const std::string &userID,
                             const Countries::Type &type);
    bool redisExistsUserCountry(const std::string &userID,
                                const Countries::Type &type);
    void redisAddUserToCountrySet(const std::string &userID,
                                  const std::string &countryID,
                                  const Countries::Type &type);
    void deleteUserCountryByID(const std::string &userID,
                               const Countries::Type &type);
};

#endif
