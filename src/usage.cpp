#include "usage.hpp"
#include <iostream>

Usage::Usage()
{
    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "Usage error: " << this->context->errstr
                      << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "Usage can't allocate redis context"
                      << std::endl;
        }
    }
}

Usage::~Usage()
{
    if (!this->context)
        return;
    redisFree(this->context);
}

void
Usage::reconnect()
{
    if (this->context) {
        redisFree(this->context);
    }

    this->context = redisConnect("127.0.0.1", 6379);
    if (this->context == NULL || this->context->err) {
        if (this->context) {
            std::cerr << "Usage error: " << this->context->errstr
                      << std::endl;
            redisFree(this->context);
        } else {
            std::cerr << "Usage can't allocate redis context"
                      << std::endl;
        }
    }
}

void
Usage::doIncrUser(const std::string& user, const std::string& channel)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HINCRBY WNMA:USAGE:users:%b %b 1", user.c_str(), user.size(), channel.c_str(), channel.size()));
    if (reply == NULL && this->context->err) {
        std::cerr << "USAGE error: " << this->context->errstr
                  << std::endl;
        freeReplyObject(reply);
        this->reconnect();
        ++this->attempt;
        if (++this->attempt > 2) {
            this->attempt = 0;
            std::cerr << "USAGE cannot doIncrUser for " << user << " " << channel << std::endl;
            return;
        }
        doIncrUser(user, channel);
        return;
    }
    this->attempt = 0;
    freeReplyObject(reply);
}

void
Usage::doIncrUserCommand(const std::string& user, const std::string& channel, const std::string& command)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HINCRBY WNMA:USAGE:users:%b:%b %b 1", user.c_str(), user.size(), channel.c_str(), channel.size(), command.c_str(), command.size()));
    if (reply == NULL && this->context->err) {
        std::cerr << "USAGE error: " << this->context->errstr
                  << std::endl;
        freeReplyObject(reply);
        this->reconnect();
        ++this->attempt;
        if (++this->attempt > 2) {
            this->attempt = 0;
            std::cerr << "USAGE cannot doIncrUserCommand for " << user << " " << channel << " " << command << std::endl;
            return;
        }
        doIncrUserCommand(user, channel, command);
        return;
    }
    this->attempt = 0;
    freeReplyObject(reply);
}

void
Usage::doIncrChannelCommandUser(const std::string& channel, const std::string& command, const std::string& user)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HINCRBY WNMA:USAGE:commands:channels:%b:%b %b 1", command.c_str(), command.size(), channel.c_str(), channel.size(), user.c_str(), user.size()));
    if (reply == NULL && this->context->err) {
        std::cerr << "USAGE error: " << this->context->errstr
                  << std::endl;
        freeReplyObject(reply);
        this->reconnect();
        ++this->attempt;
        if (++this->attempt > 2) {
            this->attempt = 0;
            std::cerr << "USAGE cannot doIncrChannelCommandUser for " << channel << " " << command << " " << user << std::endl;
            return;
        }
        doIncrChannelCommandUser(channel, command, user);
        return;
    }
    this->attempt = 0;
    freeReplyObject(reply);
}

void
Usage::doIncrChannelCommand(const std::string& channel, const std::string& command)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HINCRBY WNMA:USAGE:channels:commands:%b %b 1", channel.c_str(), channel.size(), command.c_str(), command.size()));
    if (reply == NULL && this->context->err) {
        std::cerr << "USAGE error: " << this->context->errstr
                  << std::endl;
        freeReplyObject(reply);
        this->reconnect();
        ++this->attempt;
        if (++this->attempt > 2) {
            this->attempt = 0;
            std::cerr << "USAGE cannot doIncrChannelCommand for " << channel << " " << command << std::endl;
            return;
        }
        doIncrChannelCommand(channel, command);
        return;
    }
    this->attempt = 0;
    freeReplyObject(reply);
}

void 
Usage::doIncrChannelUser(const std::string& channel, const std::string& user)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HINCRBY WNMA:USAGE:channels:users:%b %b 1", channel.c_str(), channel.size(), user.c_str(), user.size()));
    if (reply == NULL && this->context->err) {
        std::cerr << "USAGE error: " << this->context->errstr
                  << std::endl;
        freeReplyObject(reply);
        this->reconnect();
        ++this->attempt;
        if (++this->attempt > 2) {
            this->attempt = 0;
            std::cerr << "USAGE cannot doIncrChannelUser for " << channel << " " << user << std::endl;
            return;
        }
        doIncrChannelUser(channel, user);
        return;
    }
    this->attempt = 0;
    freeReplyObject(reply);
}
void
Usage::doIncrChannel(const std::string& channel)
{
    redisReply *reply = static_cast<redisReply *>(
        redisCommand(this->context, "HINCRBY WNMA:USAGE:channels %b 1", channel.c_str(), channel.size()));
    if (reply == NULL && this->context->err) {
        std::cerr << "USAGE error: " << this->context->errstr
                  << std::endl;
        freeReplyObject(reply);
        this->reconnect();
        ++this->attempt;
        if (++this->attempt > 2) {
            this->attempt = 0;
            std::cerr << "USAGE cannot doIncrChannel for " << channel << std::endl;
            return;
        }
        doIncrChannel(channel);
        return;
    }
    this->attempt = 0;
    freeReplyObject(reply);
}

void
Usage::IncrUser(const std::string& user, const std::string& channel)
{
    std::lock_guard lk(this->mtx);
    this->doIncrUser(user, channel);
    this->doIncrUser(user, "_self");
}

void
Usage::IncrUserCommand(const std::string& user, const std::string& channel, const std::string& command)
{
    std::lock_guard lk(this->mtx);
    this->doIncrUserCommand(user, channel, command);
    this->doIncrUserCommand(user, "_self", command);
    this->doIncrChannelCommandUser(channel, command, user);
    this->doIncrChannelCommandUser("_self", command, user);
}

void
Usage::IncrChannelCommand(const std::string& channel, const std::string& command)
{
    std::lock_guard lk(this->mtx);
    this->doIncrChannelCommand(channel, command);
    this->doIncrChannelCommand("_self", command);
}

void
Usage::IncrChannel(const std::string& channel)
{
    std::lock_guard lk(this->mtx);
    this->doIncrChannel(channel);
    this->doIncrChannel("_self");
}


void
Usage::IncrChannelUser(const std::string& channel, const std::string& user)
{
    std::lock_guard lk(this->mtx);
    this->doIncrChannelUser(channel, user);
    this->doIncrChannelUser("_self", user);
}

void
Usage::IncrAll(const std::string& user, const std::string& channel, const std::string& command)
{
    this->IncrUserCommand(user, channel, command);
    this->IncrUser(user, channel);
    this->IncrChannelCommand(channel, command);
    this->IncrChannel(channel);
    this->IncrChannelUser(channel, user);
}