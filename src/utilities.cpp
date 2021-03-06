#include "utilities.hpp"

#include <algorithm>
#include <iomanip>
#include <locale>
#include <sstream>
#include <mutex>

#include <boost/algorithm/string/replace.hpp>

void
changeToLower(std::string &str)
{
    static std::vector<std::string> vekcharup{"Á", "Č", "Ď", "É", "Ě",
                                              "Í", "Ň", "Ó", "Ř", "Š",
                                              "Ť", "Ú", "Ů", "Ý", "Ž"};
    static std::vector<std::string> vekchardown{"á", "č", "ď", "é", "ě",
                                                "í", "ň", "ó", "ř", "š",
                                                "ť", "ú", "ů", "ý", "ž"};

    for (std::vector<std::string>::size_type i = 0; i < vekcharup.size(); ++i) {
        size_t pos = 0;
        while ((pos = str.find(vekcharup[i], 0)) != std::string::npos) {
            str.replace(pos, vekcharup[i].size(), vekchardown[i], 0,
                        vekchardown[i].size());
        }
    }
    std::transform(str.begin(), str.end(), str.begin(),
                   [](char c) { return std::tolower(c, std::locale()); });
}

std::string
changeToLower_copy(std::string str)
{
    static std::vector<std::string> vekcharup{"Á", "Č", "Ď", "É", "Ě",
                                              "Í", "Ň", "Ó", "Ř", "Š",
                                              "Ť", "Ú", "Ů", "Ý", "Ž"};
    static std::vector<std::string> vekchardown{"á", "č", "ď", "é", "ě",
                                                "í", "ň", "ó", "ř", "š",
                                                "ť", "ú", "ů", "ý", "ž"};

    for (std::vector<std::string>::size_type i = 0; i < vekcharup.size(); ++i) {
        size_t pos = 0;
        while ((pos = str.find(vekcharup[i], 0)) != std::string::npos) {
            str.replace(pos, vekcharup[i].size(), vekchardown[i], 0,
                        vekchardown[i].size());
        }
    }
    std::transform(str.begin(), str.end(), str.begin(),
                   [](char c) { return std::tolower(c, std::locale()); });
    return str;
}

std::string
makeTimeString(int64_t seconds)
{
    std::string time;
    int s = seconds % 60;
    seconds /= 60;
    int m = seconds % 60;
    seconds /= 60;
    int h = seconds % 24;
    seconds /= 24;
    int d = seconds;
    if (d) {
        time += std::to_string(d) + "d ";
    }
    if (h) {
        time += std::to_string(h) + "h ";
    }
    if (m) {
        time += std::to_string(m) + "m ";
    }
    if (s) {
        time += std::to_string(s) + "s ";
    }
    if (time.size() != 0 && time.back() == ' ') {
        time.pop_back();
    }
    return time;
}

std::vector<std::string>
splitIntoChunks(std::string &&str, int maxlen)
{
    std::vector<std::string> vec;
    for (;;) {
        if (str.size() > maxlen) {
            auto pos = str.find_last_of(' ', maxlen);
            if (pos == std::string::npos) {
                pos = maxlen;
            } else if (pos == 0) {
                break;
            }
            vec.push_back(str.substr(0, pos));
            str.erase(0, pos);
        } else {
            vec.push_back(str.substr(0, std::string::npos));
            break;
        }
    }
    return vec;
}

std::mutex localTimeM;

std::string localTime()
{
    std::lock_guard lock(localTimeM);
    std::time_t t = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%H:%M:%S");
    return ss.str();
}

std::string localDate()
{
    std::lock_guard lock(localTimeM);
    std::time_t t = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%F");
    return ss.str();
}

std::string todayDateName()
{
    std::lock_guard lock(localTimeM);
    std::time_t t = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%e %b");
    return ss.str();
}

std::string todayDateNameLong()
{
    std::lock_guard lock(localTimeM);
    std::time_t t = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%d %B");
    return ss.str();
}

std::string todayDateNameNumber()
{
    std::lock_guard lock(localTimeM);
    std::time_t t = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%d %m");
    return ss.str();
}


std::string utcDateTime()
{
    std::lock_guard lock(localTimeM);
    std::time_t t = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&t), "[%F %H:%M:%S]");
    return ss.str();
}

void replaceTodayDates(std::string& str)
{
    boost::algorithm::replace_all(str, "{today1}", todayDateName());
    boost::algorithm::replace_all(str, "{today2}", todayDateNameLong());
    boost::algorithm::replace_all(str, "{today3}", todayDateNameNumber());
}