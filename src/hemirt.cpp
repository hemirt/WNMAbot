#include "hemirt.hpp"
#include "utilities.hpp"

#include <iostream>

std::mutex Hemirt::curlMtx;
CURL *Hemirt::curl = nullptr;
struct curl_slist *Hemirt::chunk = nullptr;

void
Hemirt::init()
{
    Hemirt::curl = curl_easy_init();
    if (Hemirt::curl) {
        Hemirt::chunk = curl_slist_append(chunk, "Accept: text/plain");
    } else {
        std::cerr << "CURL ERROR, HEMIRT" << std::endl;
    }
}

void
Hemirt::deinit()
{
    curl_slist_free_all(Hemirt::chunk);
    curl_easy_cleanup(Hemirt::curl);
}

static size_t
WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}


std::string
Hemirt::getRandom(const std::string &page)
{

    std::string rawurl("https://hemirt.com/" + page);
    std::lock_guard<std::mutex> lk(Hemirt::curlMtx);
    std::string readBuffer;
    curl_easy_setopt(Hemirt::curl, CURLOPT_HTTPHEADER, Hemirt::chunk);
    curl_easy_setopt(Hemirt::curl, CURLOPT_URL, rawurl.c_str());
    curl_easy_setopt(Hemirt::curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(Hemirt::curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(Hemirt::curl);
    curl_easy_reset(curl);

    if (res) {
        std::cerr << "Hemirt res: \"" << res << "\"" << std::endl;
        return std::string();
    }

    if (readBuffer.empty()) {
        std::cerr << "Hemirt readBuffer empty" << std::endl;
        return readBuffer;
    }

    return readBuffer;
}

std::string
Hemirt::getRaw(const std::string &page)
{

    std::string rawurl(page);
    std::lock_guard<std::mutex> lk(Hemirt::curlMtx);
    std::string readBuffer;
    curl_easy_setopt(Hemirt::curl, CURLOPT_HTTPHEADER, Hemirt::chunk);
    curl_easy_setopt(Hemirt::curl, CURLOPT_URL, rawurl.c_str());
    curl_easy_setopt(Hemirt::curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(Hemirt::curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(Hemirt::curl);
    curl_easy_reset(curl);

    if (res) {
        std::cerr << "Hemirt raw res: \"" << res << "\"" << std::endl;
        return std::string();
    }

    if (readBuffer.empty()) {
        std::cerr << "Hemirt readBuffer empty" << std::endl;
        return readBuffer;
    }

    return readBuffer;
}