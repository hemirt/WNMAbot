#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <boost/asio.hpp>

/// Shared network file with definitions that every network file will need

// This is useful if we ever wanted to use udp instead of tcp
using BoostConnection = boost::asio::ip::tcp;

namespace Network
{
    constexpr inline const char *IRC_HOST = "irc.chat.twitch.tv";
    constexpr inline const char *IRC_PORT = "6667";
}


#endif  // NETWORK_HPP
