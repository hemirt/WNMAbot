#ifndef LOTTOIMPL_HPP
#define LOTTOIMPL_HPP

#include <hiredis/hiredis.h>
#include <mutex>
#include <vector>
#include <curl/curl.h>
#include <stdint.h>
#include <fstream>

class LottoImpl
{
public:
    static std::vector<int> getNumbers();
    
    static void init();
    static void deinit();
    
private:
    static std::vector<int> readData(const std::string &readBuffer);
    static std::string apiKey;
    static std::string apiURL;
    
    static std::string curlGetWhiteRequest;
    static std::string curlGetRedRequest;
    
    static int64_t getID();
    
    static std::mutex mtx;
    static struct curl_slist *chunk;
    static CURL *curl;
 
    static redisContext *context;
    
    static bool valid;
    
    static std::fstream logFile;
};

#endif