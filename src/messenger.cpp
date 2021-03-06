#include "messenger.hpp"
#include "utilities.hpp"

Messenger::Messenger(boost::asio::io_service &_ioService,
                     std::function<bool(const std::string &)> _sendFunc)
    : sendFunc(_sendFunc)
    , ioService(_ioService)
{
}

void
Messenger::push_front(const std::string &value)
{
    std::unique_lock<std::mutex> lk(this->mtx);
    auto val = value;
    auto chunks = splitIntoChunks(std::move(val), this->maxlen);
    for (auto && chunk : chunks) {
        this->deque.push_front(std::move(chunk));
    }
    lk.unlock();
    this->startSending();
}

void
Messenger::push_front(std::string &&value)
{
    std::unique_lock<std::mutex> lk(this->mtx);
    auto chunks = splitIntoChunks(std::move(value), this->maxlen);
    for (auto && chunk : chunks) {
        this->deque.push_front(std::move(chunk));
    }
    lk.unlock();
    this->startSending();
}

void
Messenger::push_front(const std::vector<std::string>& vec)
{
    std::unique_lock<std::mutex> lk(this->mtx);
    for (auto it = vec.crbegin(); it != vec.crend(); ++it) {
        this->deque.push_front(*it);
    }
    lk.unlock();
    this->startSending();
}

void
Messenger::push_front(std::vector<std::string>&& vec)
{
    std::unique_lock<std::mutex> lk(this->mtx);
    for (auto it = vec.rbegin(); it != vec.rend(); ++it) {
        this->deque.push_front(std::move(*it));
    }
    vec.clear();
    lk.unlock();
    this->startSending();
}

void
Messenger::push_back(const std::string &value)
{
    std::unique_lock<std::mutex> lk(this->mtx);
    auto val = value;
    auto chunks = splitIntoChunks(std::move(val), this->maxlen);
    for (auto && chunk : chunks) {
        this->deque.push_back(std::move(chunk));
    }
    lk.unlock();
    this->startSending();
}

void
Messenger::push_back(std::string &&value)
{
    std::unique_lock<std::mutex> lk(this->mtx);
    auto chunks = splitIntoChunks(std::move(value), this->maxlen);
    for (auto && chunk : chunks) {
        this->deque.push_back(std::move(chunk));
    }
    lk.unlock();
    this->startSending();
}

void
Messenger::push_back(const std::vector<std::string>& vec)
{
    std::unique_lock<std::mutex> lk(this->mtx);
    for (auto it = vec.crbegin(); it != vec.crend(); ++it) {
        this->deque.push_back(*it);
    }
    lk.unlock();
    this->startSending();
}

void
Messenger::push_back(std::vector<std::string>&& vec)
{
    std::unique_lock<std::mutex> lk(this->mtx);
    for (auto it = vec.rbegin(); it != vec.rend(); ++it) {
        this->deque.push_back(std::move(*it));
    }
    vec.clear();
    lk.unlock();
    this->startSending();
}

size_t
Messenger::size() const
{
    std::unique_lock<std::mutex> lk(this->mtx);
    auto size = this->deque.size();
    lk.unlock();
    return size;
}

bool
Messenger::empty() const
{
    std::unique_lock<std::mutex> lk(this->mtx);
    auto empty = this->deque.empty();
    lk.unlock();
    return empty;
}

bool
Messenger::able() const
{
    std::unique_lock<std::mutex> lk(this->mtx);
    bool b = this->deque.size() < this->maxMsgsInQueue;
    lk.unlock();
    return b;
}

std::string
Messenger::extract_front()
{
    std::string value = std::move(this->deque.front());
    this->deque.pop_front();
    return value;
}

void
Messenger::clearQueue()
{
    std::unique_lock<std::mutex> lk(this->mtx);
    this->deque.clear();
}

void
Messenger::startSending()
{
    std::unique_lock<std::mutex> lk(this->mtx);
    if (this->ready) {
        this->ready = false;
        std::string value = this->extract_front();
        this->sendFunc(value);
        auto t = std::make_shared<boost::asio::steady_timer>(ioService,
                                               std::chrono::milliseconds(1550));
        t->async_wait([this, t](const boost::system::error_code &er) {
            if (er) {
                return;
            }
            std::unique_lock<std::mutex> lk(this->mtx);
            if (this->deque.empty()) {
                this->ready = true;
            } else {
                this->ready = true;
                lk.unlock();
                this->startSending();
            }
        });
    }
}