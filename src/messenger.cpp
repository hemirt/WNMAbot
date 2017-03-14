#include "messenger.hpp"

template <typename T>
Messenger<T>::Messenger() 
    : lk(mtx, std::defer_lock)
{
}



template <typename T>
void
Messenger<T>::push_front(const T &value)
{
    this->lk.lock();
    this->deque.push_front(value);
    this->lk.unlock();
}

template <typename T>
void
Messenger<T>::push_front(T &&value)
{
    this->lk.lock();
    this->deque.push_front(std::move(value));
    this->lk.unlock();
}

template <typename T>
void
Messenger<T>::push_back(const T &value)
{
    this->lk.lock();
    this->deque.push_back(value);
    this->lk.unlock();
}

template <typename T>
void
Messenger<T>::push_back(T &&value)
{
    this->lk.lock();
    this->deque.push_back(std::move(value));
    this->lk.unlock();
}

template <typename T>
size_t
Messenger<T>::size()
{
    this->lk.lock();
    auto size = this->deque.size();
    this->lk.unlock();
    return size;
}

template <typename T>
bool
Messenger<T>::empty()
{
    this->lk.lock();
    auto empty = this->deque.empty();
    this->lk.unlock();
    return empty;
}

template <typename T>
T
Messenger<T>::extract_front()
{
    this->lk.lock();
    T value = std::move(this->deque.front());
    this->deque.pop_front();
    this->lk.unlock();
    return value;
}

