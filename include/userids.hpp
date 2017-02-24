#ifndef USERSIDS_HPP
#define USERSIDS_HPP

#include <hiredis/hiredis.h>
#include <string>
#include <mutex>

class UserIDs
{
public:
    void addUser(const std::string &user);
    bool isUser(std::string user);
    int getID(const std::string &user);
    std::string getUserName(int userID);
    
    // Singleton
    static UserIDs &
    getInstance()
    {
        return this->instance;
    }
    UserIDs(UserIDs const &) = delete;
    void operator=(UserIDs const &) = delete;
    
private:
    UserIDs();
    ~UserIDs();
    static UserIDs instance;
    static std::mutex accessMtx;
    static redisContext *context;

};

#endif
