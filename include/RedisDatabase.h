#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H

#include <string>
#include <unordered_map>
#include <vector>

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

    std::mutex db_mutex; //mutex for thread safety

    //data structures to hold the database
    std::unordered_map<std::string, std::string> key_value_store; //key-value store
    std::unordered_map<std::string, std::vector<std::string>> list_store; //list store
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store; // unordered_map storing string keys and values for hash data type    
};

#endif