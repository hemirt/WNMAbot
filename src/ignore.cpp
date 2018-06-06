#include "ignore.hpp"
#include "utilities.hpp"
#include "databasehandle.hpp"
#include <iostream>

Ignore::Ignore()
{
    /* moved into TablesInitialize::initTables() in ConnectionHandler::start()
    hemirt::DB::Query q(
        "CREATE TABLE IF NOT EXISTS `ignoredUsers` (`username` VARCHAR(64) UNIQUE NOT NULL)");
    q.type = hemirt::DB::QueryType::RAWSQL;
    auto& db = DatabaseHandle::get();
    auto res = db.executeQuery(std::move(q));
    if (auto eval = res.error(); eval) {
        std::string err = "Creating table `ignoredUsers` error: ";
        err += eval->error();
        std::cerr << err << std::endl;
        throw std::runtime_error(err);
        return;
    }
    */
}

void
Ignore::addUser(std::string user)
{
    changeToLower(user);
    if (user == "hemirt") {
        // NaM
        return;
    }
    auto& db = DatabaseHandle::get();
    {
        std::vector<std::string> v{std::move(user)};
        {
            auto res = db.escapeString(std::move(v));
            if (auto pval = res.returned(); pval) {
                user = pval->data[0][0].second;
            } else {
                std::cerr << "Couldnt escape in Ignore::addUser" << std::endl;
                return;
            }
        }
    }
    
    hemirt::DB::Query q("INSERT IGNORE INTO `ignoredUsers` VALUES(\'" + user + "\')");
    q.type = hemirt::DB::QueryType::RAWSQL;
    auto res = db.executeQuery(std::move(q));
    if (auto eval = res.error(); eval) {
        std::cerr << "Error adding user to ignore: " << eval->error() << std::endl;
    } else {
        std::cout << "added user " << user << " to ignore" << std::endl;
    }
    return;
}

void
Ignore::removeUser(std::string user)
{
    changeToLower(user);
    auto& db = DatabaseHandle::get();
    {
        std::vector<std::string> v{std::move(user)};
        {
            auto res = db.escapeString(std::move(v));
            if (auto pval = res.returned(); pval) {
                user = pval->data[0][0].second;
            } else {
                std::cerr << "Couldnt escape in Ignore::removeUser" << std::endl;
                return;
            }
        }
    }
    
    hemirt::DB::Query q("DELETE FROM `ignoredUsers` WHERE `username` = \'" + user + "\'");
    q.type = hemirt::DB::QueryType::RAWSQL;
    auto res = db.executeQuery(std::move(q));
    if (auto eval = res.error(); eval) {
        std::cerr << "Error removing user from ignore: " << eval->error() << std::endl;
    } else {
        std::cout << "removed user " << user << " from ignore" << std::endl;
    }
    return;
}

bool
Ignore::isIgnored(std::string user)
{
    changeToLower(user);
    auto& db = DatabaseHandle::get();
    {
        std::vector<std::string> v{std::move(user)};
        {
            auto res = db.escapeString(std::move(v));
            if (auto pval = res.returned(); pval) {
                user = pval->data[0][0].second;
            } else {
                std::cerr << "Couldnt escape in Ignore::isIgnored" << std::endl;
                return true;
            }
        }
    }
    hemirt::DB::Query q("SELECT EXISTS(SELECT 1 FROM `ignoredUsers` WHERE `username` LIKE \'" + user + "\')");
    q.type = hemirt::DB::QueryType::RAWSQL;
    auto res = db.executeQuery(std::move(q));
    if (auto rval = res.returned(); rval) {
        if (rval->data.size() == 1 && rval->data[0].size() == 1) {
            if (rval->data[0][0].first) {
                if (rval->data[0][0].second == "0") {
                    return false;
                } else {
                    return true;
                }
            }
        }
    } else if (auto eval = res.error(); eval) {
        std::cerr << "Error Ignore::isIgnored user: " << user << ": " << eval->error() << std::endl;
        return true;
    }
    return true;
}