#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <stdint.h>
#include <string>
#include <vector>

void changeToLower(std::string &str);
std::string changeToLower_copy(std::string str);
std::string makeTimeString(int64_t seconds);
std::vector<std::string> splitIntoChunks(std::string &&str, int maxlen = 350);
std::string localTime();
std::string localDate();
std::string utcDateTime();
std::string todayDateName();
std::string todayDateNameLong();
std::string todayDateNameNumber();
void replaceTodayDates(std::string& str);

#endif
