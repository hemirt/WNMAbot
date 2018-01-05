#ifndef DATABASEHANDLE_HPP
#define DATABASEHANDLE_HPP

#include <memory>

#include "hemirt/mariadb.hpp"

class DatabaseHandle
{
public:
    static void init(hemirt::DB::Credentials &&creds) {
        DatabaseHandle::pdb = std::make_unique<hemirt::DB::MariaDB>(std::move(creds));
    }
    
    static hemirt::DB::MariaDB& get() noexcept {
        return *DatabaseHandle::pdb.get();
    }

private:
    inline static std::unique_ptr<hemirt::DB::MariaDB> pdb = nullptr;
};

#endif