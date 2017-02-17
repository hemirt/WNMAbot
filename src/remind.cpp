#include "remind.hpp"
#include <sstream>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

RemindsHandler::RemindsHandler()
{
    // get reminders from redis
}

void
RemindsHandler::saveToRedis(const Reminder &reminder)
{
    pt::ptree tree;
    tree.put("from", reminder.from);
    tree.put("to", reminder.to);
    tree.put("what", reminder.what);
    tree.put("id", reminder.id);
    // some kind of to string
    std::string whenStr = reminder.when;
    tree.put("when", whenStr);
    
    std::stringstream ss;
    pt::write_json(ss, tree, false);
    std::string jsonString;
    
    // check hiredis if its threadsafe or not
    std::unique_lock<std::mutex> lock(this->redisMtx);
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->redisContext, "SADD %b", jsonString.c_str(), jsonString.size()));
}

size_t
RemindsHandler::count(const std::string& from)
{
    std::shared_lock<std::shared_mutex> lock(this->vecMtx);
    // find reminders that are from user "from"
    return std::count_if(this->reminders.begin(), this->reminders.end(), [](const Reminder &r) {
            if(r.from == from) {
                return true;
            }
            return false;
        });
}

std::vector<Reminder>
RemindsHandler::getRemindersFrom(const std::string &from)
{
    std::vector<Reminder> results;
    std::shared_lock<std::shared_mutex> lock(this->vecMtx);
    for(const &auto r : this->reminders)
    {
        if(r.from == from) {
            results.push_back(r);
        }
    }
    return results;
}

std::vector<Reminder>
RemindsHandler::getRemindersTo(const std::string &to)
{   
    std::vector<Reminder> results;
    std::shared_lock<std::shared_mutex> lock(this->vecMtx);
    for(const &auto r : this->reminders)
    {
        if(r.to == to) {
            results.push_back(r);
        }
    }
    return results;
}

bool
RemindsHandler::addReminder(const std::string &from, const std::string &to, const std::string &what, int64_t seconds)
{
    auto now = std::chrono::system_clock::now();
    auto when = now + std::chrono::seconds(seconds);
    
    // find out if vector.push_back invalidates iterators etc
    // if it does, then need be unique lock
    std::shared_lock<std::shared_mutex> lock(this->vecMtx);
    
    // self reminds || other reminds, can shared_lock be recursive?
    if (from == to || this->count(from) < 2 ) {
        size_t id = 0;
        // get reminders based upon from and to and check their id
        // use unoccupied id
        
        // or some kind of emplace?
        this->reminders.push_back({from, to, what, id, when});
        return true;
    } 
    // better to else or to leave it out?
    return false;
}

void
RemindsHandler::removeReminder(const std::string& from, const std::string &to, size_t id)
{
    std::unique_lock<std::shared_mutex> lock(this->vecMtx);
    std::remove_if(this->reminders.begin(), this->reminders.end(), [](const Reminder @r) {
            if(r.from == from && r.to = to && r.id = id) {
                return true;
            }
            return false;
        });
    // remove from redis
}

void
RemindsHandler::removeAllRemindersFrom(const std::string &from)
{
    std::unique_lock<std::shared_mutex> lock(this->vecMtx);
    std::remove_if(this->reminders.begin(), this->reminders.end(), [](const Reminder &r) {
            if(r.from == from) {
                return true;
            }
            return false;
        });
    // remove from redis
}

void
RemindsHandler::removeAllRemindersTo(const std::string &to)
{
    std::unique_lock<std::shared_mutex> lock(this->vecMtx);
    std::remove_if(this->reminders.begin(), this->reminders.end(), [](const Reminder &r) {
            if(r.to == to) {
                return true;
            }
            return false;
        });
    // remove from redis
}