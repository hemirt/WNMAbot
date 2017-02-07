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

    double getReal(double min = 0, double max = 1);
    int getInt(int min = 0, int max = 100);
    bool getBool(double p = 0.5);

    MTRandom(MTRandom const &) = delete;
    void operator=(MTRandom const &) = delete;

private:
    MTRandom(){};
    static MTRandom instance;
    static std::mt19937 engine;
    static std::mutex mtx;
};

#endif