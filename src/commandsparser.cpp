#include "commandsparser.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

CommandsParser(const IRCMessage &message)
{
    boost::algorithm::split(this->words, message.params,
                        boost::algorithm::is_space(),
                        boost::token_compress_on);
                        
    //this->trigger = this->words[0]
}