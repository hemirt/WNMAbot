#ifndef MODULES_HPP
#define MODULES_HPP

#include "userids.hpp"

#include <hiredis/hiredis.h>

#include <string>
#include <map>
#include <mutex>
#include <utility>

class Module
{
public:  
    Module(const std::string &_name);
    Module(const std::string &name, const std::string &type, const std::string &subtype
        , const std::string &format, bool status) : name(name), type(type), subtype(subtype), format(format), status(status) {};
    ~Module() = default;
    
    void setName(const std::string &newName);
    bool setType(const std::string &newType);
    bool setSubtype(const std::string &newSubtype);
    void setFormat(const std::string &newFormat);
    bool setStatus(bool newStatus);
    
    const std::string &getName() const {
        return this->name;
    }
    const std::string &getType() const {
        return this->type;
    }
    const std::string &getSubtype() const {
        return this->subtype;
    }
    const std::string &getFormat() const {
        return this->format;
    }
    bool getStatus() const {
        return this->status;
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
    
    std::map<std::string, Module> modules;
    UserIDs &userIDs;
    
    bool createModule(const std::string &name);
    bool setType(const std::string &moduleName, const std::string &type);
    bool setSubtype(const std::string &moduleName, const std::string &subtype);
    bool setName(const std::string &moduleName, const std::string &newName);
    bool setFormat(const std::string &moduleName, const std::string &format);
    bool setStatus(const std::string &moduleName, const std::string &status);
    bool saveModule(const std::string &moduleName);
    void deleteModule(const std::string &moduleName);
    
    std::string getInfo(const std::string &moduleName);
    std::string getAllModules();
    
    std::pair<bool, std::string> getData(const std::string &user, const std::string &moduleName);
    std::string setData(const std::string &user, const std::string &moduleName, const std::string &data);
    std::string deleteData(const std::string &user, const std::string &moduleName);
    std::string deleteDataFull(const std::string &user);
    
    std::string getAllData(const std::string &user);

private:
    void loadAllModules();
    redisContext *context;
    mutable std::mutex access;

};

#endif