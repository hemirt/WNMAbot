#include "tablesinitialize.hpp"
#include "databasehandle.hpp"

namespace TablesInitialize {

namespace {
    void initialize(const char* tableName, std::string&& query) {
        auto& db = DatabaseHandle::get();
        hemirt::DB::Query<hemirt::DB::MariaDB::Values> q(std::move(query));
        q.type = hemirt::DB::QueryType::RAWSQL;
        auto res = db.executeQuery(std::move(q));
        if (auto eval = res.error(); eval) {
            std::string err = "Creating table `" + tableName + "` error: ";
            err += eval->error();
            std::cerr << err << std::endl;
            throw std::runtime_error(err);
            return;
        }
    }
    
    void initUserids()
    {
        initialize("users", "CREATE TABLE IF NOT EXISTS `users` (`id` INT(8) UNIQUE NOT NULL UNSIGNED AUTO_INCREMENT, `userid` VARCHAR(64) UNIQUE NOT NULL, `username` VARCHAR(64) NOT NULL, `displayname` VARCHAR(128) NOT NULL, `level` INT(4) DEFAULT 0, PRIMARY KEY(`id`))");
    }
    
    void initConnectionhandler()
    {
        initialize("wnmabot_settings", "CREATE TABLE IF NOT EXISTS `wnmabot_settings` (`setting` VARCHAR(32) UNIQUE NOT NULL, `value` VARCHAR(128) NOT NULL, PRIMARY KEY(`setting`))");
        
        initialize("channels", "CREATE TABLE IF NOT EXISTS `channels` (`m_channel_id` INT(8) UNSIGNED AUTO_INCREMENT, `userid` VARCHAR(64) UNIQUE NOT NULL, `username` VARCHAR(64) NOT NULL, PRIMARY KEY(`m_channel_id`))");
    }
    
    void initIgnore()
    {
        initialize("ignoredUsers", "CREATE TABLE IF NOT EXISTS `ignoredUsers` (`username` VARCHAR(64) UNIQUE NOT NULL)");
    }
    
    /* username -> set ? in mysql
       multiple entries username -> username
    void initPingme()
    {
        initialize("pingers", "CREATE TABLE IF NOT EXISTS `pingers` (`username` VARCHAR(64) UNIQUE NOT NULL)");
    }
    */
    
    
} // namespace

void TablesInitialize::initTables()
{
    initUserids();
    initConnectionhandler();
    initIgnore();
}

} // namespace TablesInitialize