#include "lotto/lottoimpl.hpp"
#include <iostream>

#include "rapidjson/document.h"

std::mutex LottoImpl::mtx;
CURL *LottoImpl::curl = nullptr;
struct curl_slist *LottoImpl::chunk = nullptr;

redisContext *LottoImpl::context = nullptr;
std::string LottoImpl::apiKey;

std::string LottoImpl::curlGetWhiteRequest;
std::string LottoImpl::curlGetRedRequest;

std::fstream LottoImpl::logFile;

std::string LottoImpl::apiURL= "https://api.random.org/json-rpc/1/invoke";
bool LottoImpl::valid = false;

void
LottoImpl::init()
{
    LottoImpl::curl = curl_easy_init();
    if (LottoImpl::curl) {
        LottoImpl::chunk = curl_slist_append(chunk, "Content-Type: application/json-rpc");
    } else {
        LottoImpl::valid = false;
        std::cerr << "lottoimpl curl error" << std::endl;
        return;
    }
    
    LottoImpl::context = redisConnect("127.0.0.1", 6379);
    if (LottoImpl::context == NULL || LottoImpl::context->err) {
        LottoImpl::valid = false;
        if (LottoImpl::context) {
            std::cerr << "lottoimpl redis error: " << LottoImpl::context->errstr
                      << std::endl;
            redisFree(LottoImpl::context);
        } else {
            std::cerr << "lottoimpl can't allocate redis context"
                      << std::endl;
        }
        return;
    } else {
        redisReply *reply = static_cast<redisReply *>(
            redisCommand(LottoImpl::context, "GET WNMA:randomorg"));
            
        if (!(reply == NULL && LottoImpl::context->err)) {
            if (reply->type != REDIS_REPLY_STRING) {
                LottoImpl::valid = false;
                freeReplyObject(reply);
                return;
            } else {
                LottoImpl::apiKey = std::string(reply->str, reply->len);
                freeReplyObject(reply);
            }
        } 
    }
    
    LottoImpl::curlGetWhiteRequest = "{\"jsonrpc\":\"2.0\",\"method\":\"generateSignedIntegers\",\"params\":{\"apiKey\":\"" + LottoImpl::apiKey + "\",\"n\":5,\"min\":1,\"max\":69,\"replacement\":false,\"base\":10},\"id\":";
    LottoImpl::curlGetRedRequest = "{\"jsonrpc\":\"2.0\",\"method\":\"generateSignedIntegers\",\"params\":{\"apiKey\":\"" + LottoImpl::apiKey + "\",\"n\":1,\"min\":1,\"max\":26,\"replacement\":false,\"base\":10},\"id\":";
    
    LottoImpl::logFile.open("LottoLogFile.txt", std::ios::out | std::ios::app);
    
    LottoImpl::valid = true;
}

namespace 
{
    std::string addID(std::string request, int64_t id) 
    {
        return request + std::to_string(id) + "}";
    }
}

void
LottoImpl::deinit()
{
    std::lock_guard<std::mutex> lock(LottoImpl::mtx);
    if (LottoImpl::context) {
        redisFree(LottoImpl::context);
    }
    curl_slist_free_all(LottoImpl::chunk);
    curl_easy_cleanup(LottoImpl::curl);
    LottoImpl::logFile.close();
}

int64_t
LottoImpl::getID()
{
    // prereq - called under mutex and valid
    redisReply *reply = static_cast<redisReply *>(
            redisCommand(LottoImpl::context, "INCR WNMA:randomorgID"));
            
    if (reply == NULL && LottoImpl::context->err) {
        return -1;
    }
    
    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return -2;
    }
    
    int64_t nm = reply->integer;
    freeReplyObject(reply);
    return nm;
}

static size_t
WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::vector<int>
LottoImpl::getNumbers()
{
    std::vector<int> ret;
    if (!LottoImpl::valid) {
        return ret;
    }
    std::string readBuffer;
    std::lock_guard<std::mutex> lock(LottoImpl::mtx);
    int64_t idWhite = LottoImpl::getID();
    int64_t idRed = LottoImpl::getID();
    if (idWhite < 0 || idRed < 0) {
        std::cerr << "LottoImpl id error: idWhite: " << idWhite << ", idRed: " << idRed << std::endl;
        return ret;
    }
    curl_easy_setopt(LottoImpl::curl, CURLOPT_HTTPHEADER, LottoImpl::chunk);
    curl_easy_setopt(LottoImpl::curl, CURLOPT_URL, LottoImpl::apiURL.c_str());
    curl_easy_setopt(LottoImpl::curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(LottoImpl::curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(LottoImpl::curl, CURLOPT_TIMEOUT, 3L);
    curl_easy_setopt(LottoImpl::curl, CURLOPT_CONNECTTIMEOUT , 5L);
    curl_easy_setopt(LottoImpl::curl, CURLOPT_CUSTOMREQUEST, "PUT");
    
    std::string req = "[" + addID(LottoImpl::curlGetWhiteRequest, idWhite) + ", " + 
                      addID(LottoImpl::curlGetRedRequest, idRed) + "]";

    curl_easy_setopt(LottoImpl::curl, CURLOPT_POSTFIELDS, req.c_str());
    
    CURLcode res = curl_easy_perform(LottoImpl::curl);
    long response_code;
    curl_easy_getinfo(LottoImpl::curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_reset(curl);
    if (res) {
        std::cerr << "LottoImpl res: \"" << res << "\"" << std::endl;
        return ret;
    }
    
    if (readBuffer.empty()) {
        std::cerr << "LottoImpl readBuffer empty" << std::endl;
        return ret;
    }
    
    if (response_code != 200) {
        std::cerr << "LottoImpl response code: " << response_code << std::endl;
        return ret;
    }
    
    LottoImpl::logFile << readBuffer << std::endl;;
    ret = LottoImpl::readData(readBuffer);
    return ret;
}

std::vector<int>
LottoImpl::readData(const std::string &readBuffer)
{
    std::vector<int> ret;
    using namespace rapidjson;
    Document doc;
    doc.Parse(readBuffer.c_str());
    if (!(doc.IsArray() && doc.Size() == 2)) {
        std::cerr << "doc is not array / size is not 2" << std::endl;
        return ret;
    }

    for (auto &obj : doc.GetArray()) {
        if(!obj.HasMember("result")) {
            return ret;
        }
        auto &result = obj["result"];
        if (!result.HasMember("random")) {
            return ret;
        }
        auto &random = result["random"];
        if (!random.HasMember("data")) {
            return ret;
        }
        auto &data = random["data"];
        for (auto &i : data.GetArray()) {
            if (!i.IsInt()) {
                return ret;
            }
            ret.push_back(i.GetInt());
        }
    }

    return ret;
}