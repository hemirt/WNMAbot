#include "utilities.hpp"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>

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
timenow()
{
    std::time_t result = std::time(nullptr);
    std::stringstream ss;
    ss.imbue(std::locale());
    ss << std::put_time(std::localtime(&result), "%T %Z (UTC%z)");
    return ss.str();
}

std::vector<std::string>
splitMsg(std::string &msg, const std::string &delimiter)
{
    std::vector<std::string> vec;
    size_t pos = 0;
    while ((pos = msg.find(delimiter)) != std::string::npos) {
        vec.push_back(msg.substr(0, pos));
        msg.erase(0, pos + delimiter.length());
    }
    vec.push_back(msg);
    return vec;
}
