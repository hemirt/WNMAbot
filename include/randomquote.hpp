#ifndef RANDOMQUOTE_HPP
#define RANDOMQUOTE_HPP

#include <curl/curl.h>
#include <mutex>
#include <string>

class RandomQuote
{
public:
    static void init();
    static void deinit();
    static std::string getRandomQuote(const std::string &channel, const std::string &user);

private:
    static std::mutex curlMtx;
    static struct curl_slist *chunk;
    static CURL *curl;
};

#endif