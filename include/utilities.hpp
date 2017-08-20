#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <stdint.h>
#include <string>
#include <vector>

void changeToLower(std::string &str);
std::string changeToLower_copy(std::string str);
std::string makeTimeString(int64_t seconds);
std::vector<std::string> splitIntoChunks(std::string &&str);
std::string localTime();
std::string localDate();
std::string utcDateTime();

#endif
