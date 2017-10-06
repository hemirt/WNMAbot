#ifndef USERAGE_HPP
#define USERAGE_HPP
#include "userdata.hpp"

#include <hiredis/hiredis.h>

#include <mutex>

class UserAge : UserData
{
public:
    UserAge();
    virtual ~UserAge() final;

protected:
private:
};

#endif