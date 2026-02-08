#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <mutex>

class RedisDatabase {
public:
    //get the singleton instance of the database
    static RedisDatabase& getInstance();

    //common commands/operations
    bool flushAll(); //clear all data in the database

    //key - value operations
    void set(const std::string &key, const std::string &value);
    bool get(const std::string &key, std::string &value);
    std::vector<std::string> keys(); //get all keys in the database
    std::string type(const std::string &key); //get the type of a key (string, list, hash, etc.)
    bool del(const std::string &key); //delete a key from the databasem

    //expire
    bool expire(const std::string &key, const std::string &seconds); //set a key to expire after a certain number of seconds

    //rename
    bool rename(const std::string &oldKey, const std::string &newKey); //rename a key

    //persistance: dump the database to disk
    bool dump(const std::string &filename);
    bool load(const std::string &filename);

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

    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiry_map;
};

#endif