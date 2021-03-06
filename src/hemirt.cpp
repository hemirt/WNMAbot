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
    curl_easy_setopt(Hemirt::curl, CURLOPT_TIMEOUT, 5L);
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
    curl_easy_setopt(Hemirt::curl, CURLOPT_TIMEOUT, 4L);
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

namespace
{
    
int countSubstring(const std::string &str, const std::string sub)
{
    if (sub.length() == 0) return 0;
    int count = 0;
    for (std::size_t offset = str.find(sub); offset != std::string::npos; offset = str.find(sub, offset + sub.length()))
    {
        ++count;
    }
    return count;
}

}
bool
Hemirt::banned(const std::string &msg, const std::string& channel)
{    
    std::string page;
    if (channel == "forsen") page = "https://forsen.tv/api/v1/banphrases/test";
    else if (channel == "nymn") page = "https://nymn.pajbot.com/api/v1/banphrases/test";
    else if (channel == "nani") page = "https://nani.pajbot.com/api/v1/banphrases/test";
    else if (channel == "pajlada") page = "https://paj.pajlada.se/api/v1/banphrases/test";
    else if (channel == "narwhal_dave") page = "https://narwhaldave.pajbot.com/api/v1/banphrases/test";
    //else if (channel == "infinitegachi") page = "https://bot.infinitegachi.com/api/v1/banphrases/test";
    else return false;
    
    std::lock_guard<std::mutex> lk(Hemirt::curlMtx);
    std::string readBuffer;
    
    CURL *easy = curl_easy_init();
    curl_mime *mime;
    curl_mimepart *part;
    
    mime = curl_mime_init(easy);
    part = curl_mime_addpart(mime);
    
    curl_mime_data(part, msg.c_str(), msg.size());
    curl_mime_name(part, "message");
    
    curl_easy_setopt(easy, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(easy, CURLOPT_URL, page.c_str());
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(easy, CURLOPT_TIMEOUT, 2L);
    
    CURLcode res = curl_easy_perform(easy);
    curl_easy_cleanup(easy);
    curl_mime_free(mime);

    if (res) {
        std::cerr << "Hemirt banned res: \"" << res << "\"" << std::endl;
        return true;
    }

    if (readBuffer.empty()) {
        std::cerr << "Hemirt banned readBuffer empty" << std::endl;
        return true;
    }
    if (readBuffer.find("\"banned\": true") != std::string::npos) {
        std::cout << "banned true: " << msg << std::endl;
        return true;
    }
    return false;
}