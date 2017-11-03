#ifndef MESSENGER_HPP
#define MESSENGER_HPP

#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

class Messenger
{
public:
    Messenger(boost::asio::io_service &_ioService,
              std::function<bool(const std::string &)> _sendFunc);
    void push_front(const std::string &value);
    void push_front(std::string &&value);
    void push_back(const std::string &value);
    void push_back(std::string &&value);
    size_t size() const;
    bool empty() const;
    void startSending();
    bool able() const;
    void clearQueue();

private:
    std::function<bool(const std::string &)> sendFunc;
    bool ready = true;
    std::deque<std::string> deque;
    mutable std::mutex mtx;
    boost::asio::io_service &ioService;
    std::string extract_front();
    constexpr static int maxMsgsInQueue = 2;
};

#endif