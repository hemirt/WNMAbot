#include "messenger.hpp"

Messenger::Messenger(boost::asio::io_service &_ioService, std::function<bool(const std::string&)> _f)
    : lk(mtx, std::defer_lock)
    , ioService(_ioService)
    , f(_f)
{
}


bool
Messenger::push_front(const std::string &value)
{
    this->lk.lock();
    this->deque.push_front(value);
    this->lk.unlock();
    this->startSending();
    return true;
}


bool
Messenger::push_front(std::string &&value)
{
    this->lk.lock();
    this->deque.push_front(std::move(value));
    this->lk.unlock();
    this->startSending();
    return true;
}


bool
Messenger::push_back(const std::string &value)
{
    this->lk.lock();
    if (this->deque.size() >= this->maxMsgsInQueue) {
        this->lk.unlock();
        this->startSending();
        return false;
    }
    this->deque.push_back(value);
    this->lk.unlock();
    this->startSending();
    return true;
}


bool
Messenger::push_back(std::string &&value)
{
    this->lk.lock();
    if (this->deque.size() >= this->maxMsgsInQueue) {
        this->lk.unlock();
        this->startSending();
        return false;
    }
    this->deque.push_back(std::move(value));
    this->lk.unlock();
    this->startSending();
    return true;
}


size_t
Messenger::size() const
{
    this->lk.lock();
    auto size = this->deque.size();
    this->lk.unlock();
    return size;
}


bool
Messenger::empty() const
{
    this->lk.lock();
    auto empty = this->deque.empty();
    this->lk.unlock();
    return empty;
}

std::string
Messenger::extract_front()
{
    std::string value = std::move(this->deque.front());
    this->deque.pop_front();
    return value;
}


void
Messenger::startSending()
{
    this->lk.lock();
    if(this->ready) {
        this->ready = false;
        std::string value = this->extract_front();
        this->f(value);
        auto t = new boost::asio::steady_timer(
                                ioService, std::chrono::milliseconds(1550));
        t->async_wait([this](const boost::system::error_code &er) {
                if (er) {
                    return;
                }
                this->lk.lock();
                if (this->deque.empty()) {
                    this->ready = true;
                    this->lk.unlock();
                } else {
                    this->ready = true;
                    this->lk.unlock();
                    this->startSending();
                }
            });
    }
    this->lk.unlock();
}