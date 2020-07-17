#include "lotto.hpp"
#include "utilities.hpp"

#include <boost/function_output_iterator.hpp>

#include <iostream>
#include <iterator>

LottoTicket::LottoTicket(const std::string& _user, std::vector<int> &&numbers)
    : user(_user) 
{
    if (numbers.size() != 6) {
        return;
    }
    
    std::copy_n(numbers.begin(), 5, std::inserter(white, white.end()));
    if (white.size() != 5) {
        return;
    }

    if (*white.begin() < 1 || *white.rbegin() > 69) {
        // test whether the values are in range [1, 69]
        return;
    }
    
    red = numbers.back();
    if (red > 26 || red < 1) {
        return;
    }
    
    valid = true;
}

size_t
LottoTicket::whiteMatches(const std::set<int> &rhs) const
{
    size_t i = 0;
    auto counter = [&i](auto) {++i;};
    
    std::set_intersection(
        this->white.begin(), this->white.end(),
        rhs.begin(), rhs.end(),
        boost::make_function_output_iterator(counter)
    );
    return i;
}

bool
LottoTicket::redMatch(int red) const
{
    return this->red == red;
}

Lotto::Lotto(boost::asio::io_service &_ioService, Messenger &_messenger)
    : ioService(_ioService)
    , messenger(_messenger)
{
    
}

Lotto::~Lotto()
{
    
}

std::string
Lotto::addPlay(LottoTicket &&ticket)
{
    if (!ticket.valid) {
        return ticket.user + ", invalid ticket, you need 6 numbers, of which the first 5 have to be within 1-69. The 6th number has to be between 1-26";
    }
    std::string ret = ticket.user + ", added your ticket, good luck! FeelsGoodMan";
    std::lock_guard<std::mutex> lock(this->vecMtx);
    if (this->cooldown) {
        return ticket.user + ", the lotto is on cooldown, please wait " + makeTimeString(std::chrono::duration_cast<std::chrono::seconds>((this->lottoLastTime + std::chrono::minutes(1)) - std::chrono::steady_clock::now()).count());
    }
    if (this->autoWinners == nullptr) {
        this->autoWinners = std::make_shared<boost::asio::steady_timer>(ioService, std::chrono::minutes(5));
        this->autoWinners->async_wait([this](const boost::system::error_code &er) {
           if (er) {
               std::cerr << "lotto autoWinners timer error: " << er << std::endl;
               std::lock_guard<std::mutex> lock(this->vecMtx);
               this->autoWinners = nullptr;
               return;
           }
           this->messenger.push_back(this->getWinners());
        });
        this->announcerTimer = std::make_shared<boost::asio::steady_timer>(ioService, std::chrono::seconds(270));
        this->announcerTimer->async_wait([this](const boost::system::error_code &er) {
            if (er) {
               std::cerr << "lotto announcerTimer timer error: " << er << std::endl;
               std::lock_guard<std::mutex> lock(this->vecMtx);
               this->announcerTimer = nullptr;
               return;
           }
           this->messenger.push_back("Announcing Lotto winners in 30 seconds PagChomp");
        });
        
    }
    this->tickets.push_back(std::move(ticket));
    return ret;
}

std::string
Lotto::getWinners()
{
    std::string winners;
    auto numbers = LottoImpl::getNumbers();
    std::set<int> white(numbers.begin(), numbers.end() - 1);
    int red = numbers.back();
    std::string winningNumbers = "The winning numbers are: ";
    for (const auto &i : white) {
        winningNumbers += std::to_string(i) + ", ";
    }
    winningNumbers += "and the jackpot number is " + std::to_string(red);
    
    std::unique_lock<std::mutex> lock(this->vecMtx);
    for (const auto &ticket : tickets) {
        auto i = ticket.whiteMatches(white);
        auto redMatch = ticket.redMatch(red);
        if (i) {
            
            if (i == 5 && redMatch) {
                winners += ticket.user + "(5:1 JACKPOT PagChomp ), ";
            } else {
                winners += ticket.user + "(" + std::to_string(i) + ":" + std::to_string(redMatch) + "), ";
            }
            
        } else if (redMatch) {
            winners += ticket.user + "(0:1), ";
        }
    }
    
    this->tickets.clear();
    this->cooldown = true;
    this->autoWinners = nullptr;
    this->announcerTimer = nullptr;
    this->cooldownTimer = std::make_shared<boost::asio::steady_timer>(ioService, std::chrono::minutes(1));
    this->cooldownTimer->async_wait([this](const boost::system::error_code &er){
        if (er) {
            std::cerr << "lotto cooldownTimer timer error: " << er << std::endl;
            return;
        }
        std::lock_guard<std::mutex> lock(this->vecMtx);
        this->cooldown = false;
    });
    this->lottoLastTime = std::chrono::steady_clock::now();
    lock.unlock();
    
    if (!winners.empty()) {
        winners.pop_back();
        winners.pop_back();
        return winningNumbers + ", the winners are: " + winners;
    } else {
        return winningNumbers + ", there are no winners! FeelsBadMan";
    }
}