#include "encryption.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

std::string
Encryption::encrypt(const std::string &what)
{
    std::string ret;
    for (const auto &c : what) {
        int i;
        for (i = 1; i < this->chars.size(); i++) {
            if (this->chars[i] == c) {
                break;
            }
        }
        if (i == this->chars.size()) {
            ret += c;
            ret += ' ';
        } else {
            ret += this->intToEmotes.find(i)->second + " ";
        }
    }
    return ret;
}

std::string
Encryption::decrypt(const std::string &what)
{
    std::string ret;
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, what,
                            boost::algorithm::is_space(),
                            boost::token_compress_on);
    for(const auto &tok : tokens) {
        auto it = this->emotesToInt.find(tok);
        if (it != this->emotesToInt.end()) {
            ret += this->chars[it->second];
        } else {
            ret += tok;
        }
    }
    return ret;
}