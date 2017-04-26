#include "randomquote.hpp"

#include <iostream>

std::mutex RandomQuote::curlMtx;
CURL *RandomQuote::curl = nullptr;
struct curl_slist *RandomQuote::chunk = nullptr;

void
RandomQuote::init()
{
    RandomQuote::curl = curl_easy_init();
    if (RandomQuote::curl) {
        RandomQuote::chunk = curl_slist_append(chunk, "Accept: text/plain");
    } else {
        std::cerr << "CURL ERROR, RANDOMQUOTE" << std::endl;
    }
}

void
RandomQuote::deinit()
{
    curl_slist_free_all(RandomQuote::chunk);
    curl_easy_cleanup(RandomQuote::curl);
}

static size_t
WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::string
RandomQuote::getRandomQuote(const std::string &channel, const std::string &user)
{
    std::string rawurl("https://api.gempir.com/channel/" + channel + "/user/" + user + "/random");
    
    std::lock_guard<std::mutex> lk(RandomQuote::curlMtx);
    std::string readBuffer;
    curl_easy_setopt(RandomQuote::curl, CURLOPT_HTTPHEADER, RandomQuote::chunk);
    curl_easy_setopt(RandomQuote::curl, CURLOPT_URL, rawurl.c_str());
    curl_easy_setopt(RandomQuote::curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(RandomQuote::curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(RandomQuote::curl);
    curl_easy_reset(curl);

    if (res) {
        std::cerr << "RandomQuote res: \"" << res << "\"" << std::endl;
        return std::string();
    }

    if (readBuffer.empty()) {
        std::cerr << "RandomQuote readBuffer empty" << std::endl;
        return readBuffer;
    }
    
    long response_code;
    curl_easy_getinfo(RandomQuote::curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code == 200) {
        return readBuffer;
    }
    
    return std::string();
}