#ifndef MTRANDOM_HPP
#define MTRANDOM_HPP

#include <chrono>
#include <mutex>
#include <random>

class MTRandom
{
public:
    static MTRandom &
    getInstance()
    {
        return instance;
    }

    double getReal(const double &min = 0, const double &max = 1);
    int getInt(const int &min = 0, const int &max = 100);

    MTRandom(MTRandom const &) = delete;
    void operator=(MTRandom const &) = delete;

private:
    MTRandom(){};
    static MTRandom instance;
    static std::mt19937 engine;
    static std::mutex mtx;
};

#endif