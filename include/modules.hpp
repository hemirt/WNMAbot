#ifndef MODULES_HPP
#define MODULES_HPP

#include "userids.hpp"

#include <hiredis/hiredis.h>

#include <string>
#include <unordered_map>
#include <mutex>

class Module
{
public:  
    Module(const std::string &_name);
    ~Module() = default;
    
    void setName(const std::string &newName);
    bool setType(const std::string &newType);
    bool setSubtype(const std::string &newSubtype);
    bool setStatus(bool newStatus);
    void setFormat(const std::string &newFormat);
    
    const std::string &getName() const {
        return this->name;
    }
    const std::string &getType() const {
        return this->type;
    }
    const std::string &getSubtype() const {
        return this->subtype;
    }
    bool getStatus() const {
        return this->status;
    }
    const std::string &getFormat() const {
        return this->format;
    }
    

    
private:
    std::string name;
    std::string type;
    std::string subtype;
    std::string format;
    bool status = false;

};

class ModulesManager
{
public:
    ModulesManager();
    ~ModulesManager();
    
    std::unordered_map<std::string, Module> modules;
    UserIDs &userIDs;
    
    bool createModule(const std::string &name);
    bool setType(const std::string &moduleName, const std::string &type);
    bool setSubtype(const std::string &moduleName, const std::string &subtype);
    bool setName(const std::string &moduleName, const std::string &newName);
    bool setFormat(const std::string &moduleName, const std::string &format);
    bool setStatus(const std::string &moduleName, const std::string &status);
    
    std::string getInfo(const std::string &moduleName);
    
    std::string getData(const std::string &user, const std::string &moduleName);

private:
    redisContext *context;
    mutable std::mutex access;

};

#endif