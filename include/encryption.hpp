#ifndef ENCRYPTION_HPP
#define ENCRYPTION_HPP

#include <map>
#include <string>
#include <vector>

class Encryption
{
public: 
    std::string encrypt(const std::string &what);
    std::string decrypt(const std::string &what);

private:
    const std::map<std::string, unsigned int> emotesToInt = {{"gachiBASS", 1}, {"Kappa", 2}, {"Keepo", 3}, {"KKona", 4}, {"KKonaW", 5}, {"monkaS", 6}, {"monkaMEGA", 7}, {"LUL", 8}, {"PETALUL", 9}, {"OMEGALUL", 10}, {"4Head", 11}, {"4HEad", 12}, {"4Header", 13}, {"4Headest", 14}, {"gachiGASM", 15}, {"FeelsOkayMan", 16}, {"FeelsBadMan", 17}, {"FeelsAmazingMan", 18}, {"TriHard", 19}, {"TriEasy", 20}, {"forsenPls", 21}, {"WutFace", 22}, {"SeemsGood", 23}, {"PagChomp", 24}, {"ResidentSleeper", 25}, {"DansGame", 26}, {"pajaDank", 27}, {"haHAA", 28}, {"HotPokket", 29}, {"BrokeBack", 30}, {"ANELE", 31}, {"DatSheffy", 32}, {"FailFish", 33}, {"KappaPride", 34}, {"MrDestructoid", 35}, {"VoteNay", 36}, {"VoteYea", 37}, {"forsenSWA", 38}, {"CiGrip", 39}, {"NaM", 40}, {"OMGScoots", 41}, {"KKomrade", 42}, {"PepeLaugh", 43}, {"pajaBASS", 44}, {"eShrug", 45}, {"gachiOnFIRE", 46}, {"gachiGold", 47}, {"ZULUL", 48}, {"weSmart", 49}, {"SMOrc", 50}, {"AngelThump", 51}, {"TaxiBro", 52}, {"tooDank", 53}};
    const std::map<unsigned int, std::string> intToEmotes = {{1, "gachiBASS"}, {2, "Kappa"}, {3, "Keepo"}, {4, "KKona"}, {5, "KKonaW"}, {6, "monkaS"}, {7, "monkaMEGA"}, {8, "LUL"}, {9, "PETALUL"}, {10, "OMEGALUL"}, {11, "4Head"}, {12, "4HEad"}, {13, "4Header"}, {14, "4Headest"}, {15, "gachiGASM"}, {16, "FeelsOkayMan"}, {17, "FeelsBadMan"}, {18, "FeelsAmazingMan"}, {19, "TriHard"}, {20, "TriEasy"}, {21, "forsenPls"}, {22, "WutFace"}, {23, "SeemsGood"}, {24, "PagChomp"}, {25, "ResidentSleeper"}, {26, "DansGame"}, {27, "pajaDank"}, {28, "haHAA"}, {29, "HotPokket"}, {30, "BrokeBack"}, {31, "ANELE"}, {32, "DatSheffy"}, {33, "FailFish"}, {34, "KappaPride"}, {35, "MrDestructoid"}, {36, "VoteNay"}, {37, "VoteYea"}, {38, "forsenSWA"}, {39, "CiGrip"}, {40, "NaM"}, {41, "OMGScoots"}, {42, "KKomrade"}, {43, "PepeLaugh"}, {44, "pajaBASS"}, {45, "eShrug"}, {46, "gachiOnFIRE"}, {47, "gachiGold"}, {48, "ZULUL"}, {49, "weSmart"}, {50, "SMOrc"}, {51, "AngelThump"}, {52, "TaxiBro"}, {53, "tooDank"}};
    const std::vector<char> chars = {'\0', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' '};
};

#endif