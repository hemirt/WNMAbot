#ifndef MESSENGER_HPP
#define MESSENGER_HPP

#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <boost/asio/steady_timer.hpp>
#include <functional>


class Messenger
{
public:
    Messenger(boost::asio::io_service &_ioService, std::function<bool(const std::string&)> _f);
    bool push_front(const std::string &value);
    bool push_front(std::string &&value);
    bool push_back(const std::string &value);
    bool push_back(std::string &&value);
    size_t size() const;
    bool empty() const;
    void startSending();

private:
    std::function<bool(const std::string&)> f;
    bool ready = true;
    std::deque<std::string> deque;
    mutable std::mutex mtx;
    mutable std::unique_lock<std::mutex> lk;
    boost::asio::io_service &ioService;
    std::string extract_front();
    constexpr static int maxMsgsInQueue = 3;

};

#endif