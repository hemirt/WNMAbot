#ifndef AYAH_HPP
#define AYAH_HPP

#include <curl/curl.h>
#include <hiredis/hiredis.h>
#include <mutex>
#include <string>

class Ayah
{
public:
    static void init();
    static void deinit();
    static std::string getRandomAyah();
    static std::string getAyah(int number);

private:
    static std::mutex curlMtx;
    static struct curl_slist *chunk;
    static CURL *curl;
};

#endif