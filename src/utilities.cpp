#include "utilities.hpp"

#include <algorithm>
#include <iomanip>
#include <locale>

void
changeToLower(std::string &str)
{
    static std::vector<std::string> vekcharup{"Á", "C", "D", "É", "E",
                                              "Í", "N", "Ó", "R", "Š",
                                              "T", "Ú", "U", "Ý", "Ž"};
    static std::vector<std::string> vekchardown{"á", "c", "d", "é", "e",
                                                "í", "n", "ó", "r", "š",
                                                "t", "ú", "u", "ý", "ž"};

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