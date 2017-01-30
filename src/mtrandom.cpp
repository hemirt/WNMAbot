#include "mtrandom.hpp"

std::mt19937 MTRandom::engine{static_cast<unsigned int>(
    std::chrono::system_clock::now().time_since_epoch().count())};
std::mutex MTRandom::mtx;
MTRandom MTRandom::instance;

double
MTRandom::getReal(const double &min, const double &max)
{
    if (min > max) {
        return 0;
    }
    std::uniform_real_distribution<double> dis(min, max);
    std::lock_guard<std::mutex> lock(this->mtx);
    return dis(this->engine);
}

int
MTRandom::getInt(const int &min, const int &max)
{
    if (min > max) {
        return 0;
    }
    std::uniform_int_distribution<int> dis(min, max);
    std::lock_guard<std::mutex> lock(this->mtx);
    return dis(this->engine);
}