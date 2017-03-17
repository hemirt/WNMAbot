#ifndef BIBLE_HPP
#define BIBLE_HPP

#include <curl/curl.h>
#include <mutex>
#include <string>

class Bible
{
public:
    static void init();
    static void deinit();
    static std::string getRandomVerse();

private:
    static std::mutex curlMtx;
    static struct curl_slist *chunk;
    static CURL *curl;
};

#endif