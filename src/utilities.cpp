#include "utilities.hpp"

#include <algorithm>
#include <iomanip>
#include <locale>

void
changeToLower(std::string &str)
{
    static std::vector<std::string> vekcharup{"Á", "Č", "Ď", "É", "Ě",
                                              "Í", "Ň", "Ó", "Ř", "Š",
                                              "Ť", "Ú", "Ů", "Ý", "Ž"};
    static std::vector<std::string> vekchardown{"á", "č", "ď", "é", "ě",
                                                "í", "ň", "ó", "ř", "š",
                                                "ť", "ú", "ů", "ý", "ž"};

    for (int i = 0; i < vekcharup.size(); ++i) {
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

    for (int i = 0; i < vekcharup.size(); ++i) {
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