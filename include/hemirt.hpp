#ifndef HEMIRT_HPP
#define HEMIRT_HPP

#include <curl/curl.h>
#include <mutex>
#include <string>

class Hemirt
{
public:
    static void init();
    static void deinit();
    static std::string getRandom(const std::string &page);
    static std::string getRaw(const std::string &page);

private:
    static std::mutex curlMtx;
    static struct curl_slist *chunk;
    static CURL *curl;
};

#endif