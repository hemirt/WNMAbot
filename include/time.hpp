#ifndef TIME_HPP
#define TIME_HPP

#include <hiredis/hiredis.h>
#include <mutex>



class Time
{
public:
    static Time &getInstance()
    {
        static Time instance;
        return instance;
    }
        
    bool setTimeZone(const std::string &userid, const std::string &timezone);
    void delTimeZone(const std::string &userid);
    std::string getTimeZone(const std::string &userid);
    std::string getCurrentTime(const std::string &timezone);
    std::string getUsersCurrentTime(const std::string &userid);
    

private:    
    Time();
    ~Time();
    redisContext *context = nullptr;
    std::mutex accessMtx;
};

#endif // TIME_HPP
