#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <boost/asio.hpp>

/// Shared network file with definitions that every network file will need

// This is useful if we ever wanted to use udp instead of tcp
using BoostConnection = boost::asio::ip::tcp;

#endif  // NETWORK_HPP
