#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H

#include <string>

class RedisDatabase {
public:
    //get the singleton instance of the database
    static RedisDatabase& getInstance();

    //persistance: dump the database to disk
    bool dump(const std::string& filename);
    bool load(const std::string& filename);

private:
    RedisDatabase() = default;  //private constructor for singleton pattern
    ~RedisDatabase() = default; //default destructor
    RedisDatabase(const RedisDatabase&) = delete;   //delete copy constructor
    RedisDatabase& operator=(const RedisDatabase&) = delete;    //delete copy assignment operator

};

#endif