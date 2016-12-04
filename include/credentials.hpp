#ifndef CREDENTIALS_HPP
#define CREDENTIALS_HPP

#include <string>

class Credentials
{
public:
    Credentials(const std::string &_username, const std::string &_password)
        : username(_username)
        , password(_password)
    {
    }

    // Twitch username (login name)
    std::string username;

    // OAuth key
    std::string password;
};

#endif  // CREDENTIALS_HPP
