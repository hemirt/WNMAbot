#ifndef LOTTO_HPP
#define LOTTO_HPP

#include "lotto/lottoimpl.hpp" // https://www.random.org/
#include "messenger.hpp"

#include <algorithm>
#include <set>
#include <chrono>
#include <string>
#include <vector>
#include <mutex>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

class LottoTicket
{
public:
    LottoTicket(const std::string& user, std::vector<int> &&numbers);
    size_t whiteMatches(const std::set<int> &rhs) const;
    bool redMatch(int red) const;

private:
    bool valid = false;
    std::set<int> white;
    int red;
    std::string user;
    
    friend class Lotto;
};

class Lotto
{
public: 
    Lotto(boost::asio::io_service &_ioService, Messenger &_messenger); // read api key from redis using impl
    ~Lotto(); // cancel lotto if in progress
    std::string addPlay(LottoTicket &&ticket);
    std::string getWinners();
    bool onCooldown();
    
private:
    std::chrono::steady_clock::time_point lottoLastTime =
        std::chrono::steady_clock::now();
    std::vector<LottoTicket> tickets;
    bool cooldown = false;
    std::mutex vecMtx;
    boost::asio::io_service &ioService;
    std::shared_ptr<boost::asio::steady_timer> autoWinners = nullptr;
    std::shared_ptr<boost::asio::steady_timer> cooldownTimer = nullptr;
    std::shared_ptr<boost::asio::steady_timer> announcerTimer = nullptr;
    
    Messenger &messenger;
};

#endif // LOTTO_HPP
