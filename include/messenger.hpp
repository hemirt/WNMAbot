#ifndef MESSENGER_HPP
#define MESSENGER_HPP

#include <deque>
#include <mutex>
#include <condition_variable>

template <typename T>
class Messenger
{
public:
    Messenger();
    void push_front(const T &value);
    void push_front(T &&value);
    void push_back(const T &value);
    void push_back(T &&value);
    size_t size();
    bool empty();
    T extract_front();

private:
    std::deque<T> deque;
    std::mutex mtx;
    std::unique_lock<std::mutex> lk;
};