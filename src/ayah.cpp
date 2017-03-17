#include "ayah.hpp"
#include "utilities.hpp"
#include "mtrandom.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

namespace pt = boost::property_tree;

std::mutex Ayah::curlMtx;
CURL *Ayah::curl = nullptr;
struct curl_slist *Ayah::chunk = nullptr;

void
Ayah::init()
{
    Ayah::curl = curl_easy_init();
    if (Ayah::curl) {
        Ayah::chunk = curl_slist_append(
            chunk, "Accept: application/json");
    } else {
        std::cerr << "CURL ERROR, AYAH" << std::endl;
    }
}

void
Ayah::deinit()
{
    curl_slist_free_all(Ayah::chunk);
    curl_easy_cleanup(Ayah::curl);
}

static size_t
WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::string
Ayah::getRandomAyah()
{
    return Ayah::getAyah(MTRandom::getInstance().getInt(1, 6236));
}

std::string
Ayah::getAyah(int number)
{
    if (number <= 0 || number > 6236) {
        return "Pick Ayah between 1 - 6236 including ANELE";
    }
    std::string rawurl("http://api.alquran.cloud/ayah/" + std::to_string(number) + "/en.asad");
    
    std::lock_guard<std::mutex> lk(Ayah::curlMtx);
    std::string readBuffer;
    curl_easy_setopt(Ayah::curl, CURLOPT_HTTPHEADER, Ayah::chunk);
    curl_easy_setopt(Ayah::curl, CURLOPT_URL, rawurl.c_str());
    curl_easy_setopt(Ayah::curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(Ayah::curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(Ayah::curl);
    curl_easy_reset(curl);
    
    if (res) {
        std::cerr << "Ayah res: \"" << res << "\""
                  << std::endl;
        return std::string();
    }

    if (readBuffer.empty()) {
        std::cerr << "Ayah readBuffer empty"
                  << std::endl;
        return readBuffer;
    }

    pt::ptree tree;
    std::stringstream ss(readBuffer);
    pt::read_json(ss, tree);
    
    std::string text = tree.get("data.text", "");
    if (text.empty()) {
        std::cerr << "Ayah text empty\n" << readBuffer
                  << std::endl;
        return text;
    }
    
    return "ANELE Ayah " + std::to_string(number) + ": " + text + " ANELE";
}