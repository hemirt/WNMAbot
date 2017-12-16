#include "mtrandom.hpp"
#include <iostream>

std::mt19937 MTRandom::engine{static_cast<unsigned int>(
    std::chrono::system_clock::now().time_since_epoch().count())};
std::mutex MTRandom::mtx;
MTRandom MTRandom::instance;

double
MTRandom::getReal(double min, double max)
{
    if (min > max) {
        std::swap(min, max);
    }
    std::uniform_real_distribution<double> dis(min, max);
    std::lock_guard<std::mutex> lock(this->mtx);
    return dis(this->engine);
}

std::int64_t
MTRandom::getInt(std::int64_t min, std::int64_t max)
{
    if (min > max) {
        std::swap(min, max);
    }
    std::uniform_int_distribution<int> dis(min, max);
    std::lock_guard<std::mutex> lock(this->mtx);
    return dis(this->engine);
}

bool
MTRandom::getBool(double p)
{
    if (p > 1 || p < 0) {
        std::cout << "getBool error" << p << std::endl;
        return false;
    }
    std::bernoulli_distribution dis(p);
    std::lock_guard<std::mutex> lock(this->mtx);
    return dis(this->engine);
}