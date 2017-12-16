#include "bible.hpp"
#include "utilities.hpp"

#include <boost/algorithm/string/erase.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>

namespace pt = boost::property_tree;

std::mutex Bible::curlMtx;
CURL *Bible::curl = nullptr;
struct curl_slist *Bible::chunk = nullptr;

void
Bible::init()
{
    Bible::curl = curl_easy_init();
    if (Bible::curl) {
        Bible::chunk = curl_slist_append(chunk, "Accept: application/json");
    } else {
        std::cerr << "CURL ERROR, BIBLE" << std::endl;
    }
}

void
Bible::deinit()
{
    curl_slist_free_all(Bible::chunk);
    curl_easy_cleanup(Bible::curl);
}

static size_t
WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::string
Bible::getRandomVerse()
{
    std::string rawurl("http://labs.bible.org/api/?passage=random&type=json");

    std::lock_guard<std::mutex> lk(Bible::curlMtx);
    std::string readBuffer;
    curl_easy_setopt(Bible::curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(Bible::curl, CURLOPT_HTTPHEADER, Bible::chunk);
    curl_easy_setopt(Bible::curl, CURLOPT_URL, rawurl.c_str());
    curl_easy_setopt(Bible::curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(Bible::curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(Bible::curl);
    curl_easy_reset(curl);

    if (res) {
        std::cerr << "Bible res: \"" << res << "\"" << std::endl;
        return std::string();
    }

    if (readBuffer.empty()) {
        std::cerr << "Bible readBuffer empty" << std::endl;
        return readBuffer;
    }

    std::cout << __FILE__ << " " << __LINE__ << std::endl;
    pt::ptree tree;
    if (readBuffer.back() == ']') {
        readBuffer.pop_back();
    }
    std::stringstream ss(readBuffer.substr(1, std::string::npos));
    pt::read_json(ss, tree);

    std::string bookname = tree.get("bookname", "");
    std::string chapter = tree.get("chapter", "");
    std::string verse = tree.get("verse", "");
    std::string text = tree.get("text", "");
    if (text.empty()) {
        std::cerr << "Bible text empty\n" << readBuffer << std::endl;
        return text;
    }

    boost::algorithm::erase_all(text, "<b>");
    boost::algorithm::erase_all(text, "</b>");

    std::string ret;

    if (!bookname.empty()) {
        ret = bookname + " ";
    }

    if (!chapter.empty()) {
        ret += chapter;
        if (!verse.empty()) {
            ret += ":" + verse + " " + text;
        }
    } else if (!verse.empty()) {
        ret += verse + " " + text;
    } else {
        ret += text;
    }

    return "riPepperonis " + ret + " riPepperonis";
}