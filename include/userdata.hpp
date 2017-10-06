#ifndef USERDATA_HPP
#define USERDATA_HPP

#include <hiredis/hiredis.h>

#include <mutex>
#include <string>

class UserData
{
public:
    virtual ~UserData();

    virtual getOneData(const std::string &);
    virtual getMultipleData(const std::string &);
    const std::string &
    getName() const
    {
        return this->moduleName;
    }

protected:
    UserData(const std::string &_name);
    redisContext *context;
    std::mutex redisMtx;
    const std::string moduleName;

private:
}

#endif