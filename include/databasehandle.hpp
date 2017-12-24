#ifndef DATABASEHANDLE_HPP
#define DATABASEHANDLE_HPP

#include <memory>

#include "hemirt/mariadb.hpp"

class DatabaseHandle
{
public:
    static init(hemirt::DB::Credentials &&creds) {
        this->pdb = std::make_unique<hemirt::DB::MariaDB>(std::move(creds));
    }
    
    static hemirt::DB::MariaDB& get() const noexcept {
        return *this->pdb.get();
    }

private:
    static std::unique_ptr<hemirt::DB::MariaDB> pdb;
};

#endif