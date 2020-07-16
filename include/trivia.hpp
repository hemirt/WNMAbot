#ifndef TRIVIA_HPP
#define TRIVIA_HPP

#include <hiredis/hiredis.h>
#include <curl/curl.h>

#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

struct Question
{
    std::string category;
    std::string question;
    std::string displayAnswer;
    std::string answer;
    bool hint1;
    bool hint2;
    std::string hint1str;
    std::string hint2str;
};

class Trivia
{
public:
    static void init();
    static void deinit();
    
    static Question pop();
    static void addPoint(const std::string& username);
    static long long getPoints(const std::string& username);
    static std::string top5();
    static bool allowedToStart(const std::string& username);

private:
    static void reconnect();
    static bool getQuestions();
    static inline std::mutex redisMtx = std::mutex{};
    static inline curl_slist *chunk = nullptr;
    static inline CURL *curl = nullptr;
    static inline redisContext * context = nullptr;
    static inline std::mutex mtx = std::mutex{};
    static inline std::vector<Question> questions = std::vector<Question>{};
    static inline std::unordered_map<std::string, long long> users = std::unordered_map<std::string, long long>{};
    
};

#endif // TRIVIA_HPP