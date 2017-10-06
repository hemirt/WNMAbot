#include "userdata.hpp"
#include <utility>

UserData::UserData(const std::string &_name)
    : moduleName(_name)
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "RedisClient error: " << this->context->errstr
                      << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "RedisClient can't allocate redis context"
                      << std::endl;
        }
    }
}

~UserData::Userdata()
{
    if (!this->context) {
        return;
    }
    redisFree(this->context);
}