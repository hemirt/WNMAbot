#include "remind.hpp"
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include <sstream>

namespace pt = boost::property_tree;

RemindsHandler::RemindsHandler(boost::asio::io_service &_ioService)
    : ioService(_ioService)
{
    this->loadFromRedis();
    // fire reminders
    for (auto &i : this->reminders) {
        this->fireReminder(i);
    }
}

void
RemindsHandler::fireReminder(Reminder &reminder)
{
    // what if already expired?
    auto duration = reminder.when - std::chrono::system_clock::now();
    // does delete t cancel the timer?
    reminder.timer =
        std::make_unique<boost::asio::steady_timer>(ioService, duration);
    auto remindFunction = [&reminder,
                           this](const boost::system::error_code &er) {
        if (er) {
            std::cerr << "Reminder error: " << er;
            return;
        }

        if (owner->channels.count(owner->nick) == 0) {
            owner->joinChannel(owner->nick);
        }
        std::string msg =
            "Reminder from: " + reminder.from ": " + reminder.what;

        owner->channels.at(owner->nick).whisper(msg, reminder.user);
        this->removeFromRedis(reminder);
        this->removeReminder(reminder.from, reminder.to, reminder.id);
    };
    reminder.timer->async_wait(remindFunction);
}

void
RemindsHandler::saveToRedis(const Reminder &reminder)
{
    std::cout << __FILE__ << " " << __LINE__ << std::endl;
    pt::ptree tree;
    tree.put("from", reminder.from);
    tree.put("to", reminder.to);
    tree.put("what", reminder.what);
    tree.put("id", reminder.id);
    // some kind of to string
    int64_t seconds = std::duration_cast<std::chrono::seconds>(
                          reminder.when.time_since_epoch())
                          .count();
    tree.put("when", seconds);

    std::stringstream ss;
    pt::write_json(ss, tree, false);
    std::string jsonString;

    // check hiredis if its threadsafe or not
    std::lock_guard<std::mutex> lock(this->redisMtx);
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->redisContext, "SADD WNMA:reminds %b",
                     jsonString.c_str(), jsonString.size()));
    freeReplyObject(reply);
}

void
RemindsHandler::removeFromRedis(const Reminder &reminder)
{
    std::cout << __FILE__ << " " << __LINE__ << std::endl;
    pt::ptree tree;
    tree.put("from", reminder.from);
    tree.put("to", reminder.to);
    tree.put("what", reminder.what);
    tree.put("id", reminder.id);
    // some kind of to string
    int64_t seconds = std::duration_cast<std::chrono::seconds>(
                          reminder.when.time_since_epoch())
                          .count();
    tree.put("when", seconds);

    std::stringstream ss;
    pt::write_json(ss, tree, false);
    std::string jsonString;

    // check hiredis if its threadsafe or not
    std::lock_guard<std::mutex> lock(this->redisMtx);
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->redisContext, "SREM WNMA:reminds %b",
                     jsonString.c_str(), jsonString.size()));
    freeReplyObject(reply);
}

void
RemindsHandler::loadFromRedis()
{
    // check hiredis if its threadsafe or not
    std::lock_guard<std::mutex> lock(this->redisMtx);
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->redisContext, "SMEMBERS WNMA:reminds"));

    if (reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return;
    }
    std::lock_guard<std::mutex> lock(this->vecMtx);
    for (int i = 0; i < reply->elements; ++i) {
        std::cout << __FILE__ << " " << __LINE__ << std::endl;
        pt::ptree tree;
        std::string json(reply->element[i]->str, reply->element[i]->len);
        std::stringstream ss(json);
        pt::read_json(ss, tree);

        std::string from = tree.get("from", "");
        if (from.empty()) {
            continue;
        }
        std::string to = tree.get("to", "");
        if (to.empty()) {
            continue;
        }
        std::string what = tree.get("what", "");
        if (what.empty()) {
            continue;
        }
        int id = tree.get("id", -1);
        if (id == -1) {
            continue;
        }
        int64_t seconds = tree.get("when", -1);
        if (seconds == -1) {
            continue;
        }
        auto when = std::chrono::time_point<std::chrono::system_clock>(
            std::chrono::seconds(seconds));

        this->reminders.push_back({from, to, what, id, when, nullptr});
    }
    freeReplyObject(reply);
}

size_t
RemindsHandler::count(const std::string &from)
{
    std::shared_lock<std::shared_mutex> lock(this->vecMtx);
    // find reminders that are from user "from"
    return std::count_if(this->reminders.begin(), this->reminders.end(),
                         [](const Reminder &r) {
                             if (r.from == from) {
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
    for (const &auto r : this->reminders) {
        if (r.from == from) {
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
    for (const &auto r : this->reminders) {
        if (r.to == to) {
            results.push_back(r);
        }
    }
    return results;
}

bool
RemindsHandler::addReminder(const std::string &from, const std::string &to,
                            const std::string &what, int64_t seconds)
{
    auto now = std::chrono::system_clock::now();
    auto when = now + std::chrono::seconds(seconds);

    // find out if vector.push_back invalidates iterators etc
    // if it does, then need be unique lock
    std::shared_lock<std::shared_mutex> lock(this->vecMtx);

    // self reminds || other reminds, can shared_lock be recursive?
    if (from == to || this->count(from) < 2) {
        size_t id = 0;
        // get reminders based upon from and to and check their id
        // use unoccupied id

        // or some kind of emplace?
        Reminder reminder = {from, to, what, id, when, nullptr};
        this->fireReminder(reminder);
        this->reminders.push_back(reminder);

        return true;
    }
    // better to else or to leave it out?
    return false;
}

void
RemindsHandler::removeReminder(const std::string &from, const std::string &to,
                               size_t id)
{
    std::unique_lock<std::shared_mutex> lock(this->vecMtx);

    // ?? look down
    auto reminder = std::find(this->reminders.begin(), this->reminders.end(),
                              [](const Reminder @r) {
                                  if (r.from == from &&r.to = to &&r.id = id) {
                                      return true;
                                  }
                                  return false;
                              });
    this->removeFromRedis(reminder);
    this->reminders.erase(reminder);
}

void
RemindsHandler::removeAllRemindersFrom(const std::string &from)
{
    std::unique_lock<std::shared_mutex> lock(this->vecMtx);
    std::remove_if(this->reminders.begin(), this->reminders.end(),
                   [](const Reminder &r) {
                       if (r.from == from) {
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
    std::remove_if(this->reminders.begin(), this->reminders.end(),
                   [](const Reminder &r) {
                       if (r.to == to) {
                           return true;
                       }
                       return false;
                   });
    // remove from redis
}