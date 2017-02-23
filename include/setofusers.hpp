#ifndef SETOFUSERS_HPP
#define SETOFUSERS_HPP

#include <hiredis/hiredis.h>
#include <string>

class SetOfUsers
{
public:
    SetOfUsers();
    ~SetOfUsers();
    void addUser(const std::string &user);
    bool isUser(std::string &user);

private:
    redisContext *context;

};

#endif
