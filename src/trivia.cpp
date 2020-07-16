#include "trivia.hpp"
#include "rapidjson/document.h"
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "utilities.hpp"
#include <random>

void Trivia::init()
{
    Trivia::curl = curl_easy_init();
    if (!Trivia::curl) {
        std::cerr << "Trivia: CURL ERROR" << std::endl;
    }
    
    Trivia::context = redisConnect("127.0.0.1", 6379);
    if (Trivia::context == NULL || Trivia::context->err) {
        if (Trivia::context) {
            std::cerr << "Trivia error: " << Trivia::context->errstr
                      << std::endl;
            redisFree(Trivia::context);
        } else {
            std::cerr << "Trivia can't allocate redis context"
                      << std::endl;
        }
    }
    
    {
        redisReply *reply = static_cast<redisReply *>(
            redisCommand(Trivia::context, "HGETALL WNMA:triviascore"));
        if (reply->type == REDIS_REPLY_NIL) {
            freeReplyObject(reply);
            return;
        }

        if (reply->type == REDIS_REPLY_ARRAY) {
            for (decltype(reply->elements) i = 0; i < reply->elements; i += 2) {
                std::string user(reply->element[i]->str, reply->element[i]->len);
                long long pts = std::atoi(std::string(reply->element[i + 1]->str, reply->element[i + 1]->len).c_str());
                
                std::cout << "user: " << user << " pts " << pts << std::endl;
                
                Trivia::users.emplace(std::make_pair(std::move(user), pts));
            }
        }
        freeReplyObject(reply);
    }
}

void Trivia::deinit()
{
    curl_slist_free_all(Trivia::chunk);
    curl_easy_cleanup(Trivia::curl);
    if (Trivia::context)
        redisFree(Trivia::context);
}

void Trivia::reconnect()
{
    std::lock_guard lk(Trivia::redisMtx);
    if (Trivia::context) {
        redisFree(Trivia::context);
    }

    Trivia::context = redisConnect("127.0.0.1", 6379);
    if (Trivia::context == NULL || Trivia::context->err) {
        if (Trivia::context) {
            std::cerr << "Trivia error: " << Trivia::context->errstr
                      << std::endl;
            redisFree(Trivia::context);
        } else {
            std::cerr << "Trivia can't allocate redis context"
                      << std::endl;
        }
    }
}

void Trivia::addPoint(const std::string& username)
{
    std::unique_lock lk(Trivia::redisMtx);
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        Trivia::context, "HINCRBY WNMA:triviascore %b 1", username.c_str(), username.size()));
    freeReplyObject(reply);
    lk = std::unique_lock(Trivia::mtx);
    Trivia::users[username]++;
}

long long Trivia::getPoints(const std::string& username)
{
    std::unique_lock lk(Trivia::redisMtx);
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        Trivia::context, "HGET WNMA:triviascore %b", username.c_str(), username.size()));
    if (reply == NULL && Trivia::context->err) {
        std::cerr << "Trivia redis error: " << Trivia::context->errstr
                  << std::endl;
        freeReplyObject(reply);
        lk.unlock();
        Trivia::reconnect();
        return 0;
    }
    
    if (reply == nullptr) {
        return 0;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return 0;
    }
    
    long long i = reply->integer;
    freeReplyObject(reply);
    return i;
}

bool Trivia::allowedToStart(const std::string &user)
{
    std::unique_lock lk(Trivia::redisMtx);
    redisReply *reply = static_cast<redisReply *>(redisCommand(
        Trivia::context, "SISMEMBER WNMA:triviaAllowedUsers %b", user.c_str(), user.size()));
    if (reply == NULL && Trivia::context->err) {
        std::cerr << "Trivia redis error: " << Trivia::context->errstr
                  << std::endl;
        freeReplyObject(reply);
        lk.unlock();
        Trivia::reconnect();
        return false;
    }
    
    if (reply == nullptr) {
        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return false;
    }

    if (reply->integer == 1) {
        freeReplyObject(reply);
        return true;
    } else {
        freeReplyObject(reply);
        return false;
    }
}

std::string Trivia::top5()
{
    std::lock_guard lk(Trivia::mtx);
    std::vector<std::pair<std::string, long long>> results(5);
    std::partial_sort_copy(
        Trivia::users.begin(), Trivia::users.end(), 
        results.begin(), results.end(), 
        [](const auto &lhs, const auto &rhs) { return lhs.second > rhs.second; }
    );
    std::stringstream ss;
    for (const auto& i : results) {
        ss << i.first << " " << i.second << ", ";
    }
    auto res = ss.str();
    res.pop_back();
    res.pop_back();
    
    return res;
}

namespace {
    static size_t
    WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        ((std::string *)userp)->append((char *)contents, size * nmemb);
        return size * nmemb;
    }
    
    static std::string decode(std::string&& str) 
    {
        using namespace boost::archive::iterators;

        typedef 
            transform_width<binary_from_base64<std::string::const_iterator>, 8, 6> bin_base64;
        
        int pad=0;
        while(str.back() == '=') {
            str.pop_back();
            pad++;
        }
        for (int i = 0; i < pad; ++i) {
            str.push_back('A');
        }
        
        auto res = std::string(bin_base64(str.begin()), bin_base64(str.end()));
        for (int i = 0; i < pad; ++i) {
            res.pop_back();
        }
        return res;
    }
}

bool Trivia::getQuestions()
{
    std::string readBuffer;
    {
        
        
        std::string rawurl = "https://opentdb.com/api.php?amount=50&encode=base64";
        
        curl_easy_setopt(Trivia::curl, CURLOPT_URL, rawurl.c_str());
        curl_easy_setopt(Trivia::curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(Trivia::curl, CURLOPT_WRITEDATA, &readBuffer);
        
        CURLcode res = curl_easy_perform(Trivia::curl);
        curl_easy_reset(curl);

        if (res) {
            std::cerr << "Trivia::getQuestions() res: \"" << res << "\""
                      << std::endl;
            return false;
        }
        
        if (readBuffer.empty()) {
            std::cerr << "Trivia::getQuestions() readBuffer empty"
                      << std::endl;
            return false;
        }
    }
    
    rapidjson::Document d;
    d.Parse(readBuffer.c_str());
    auto it = d.FindMember("results");
    if (it == d.MemberEnd() || !it->value.IsArray()) {
        std::cerr << "Trivia::getQuestions results NOT AN array" << std::endl;
        return false;
    }
    
    const auto& arr = it->value.GetArray();
    for (const auto& v : arr) {
        Question q;
        q.category = decode(v["category"].GetString());
        q.question = decode(v["question"].GetString());
        q.answer = decode(v["correct_answer"].GetString());
        q.displayAnswer = q.answer;
        changeToLower(q.answer);
        if (std::string(v["type"].GetString()) == "Ym9vbGVhbg==") {
            q.question += " True or False?";
        }
        Trivia::questions.push_back(std::move(q));
    }

    return true;
}

Question Trivia::pop()
{
    std::unique_lock lk(Trivia::mtx);
    if (Trivia::questions.empty()) {
        Trivia::getQuestions();
    }
    auto q = std::move(Trivia::questions.back());
    Trivia::questions.pop_back();
    lk.unlock();
    return q;
}