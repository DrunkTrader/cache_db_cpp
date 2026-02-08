#include "../include/RedisDatabase.h"

#include <fstream>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

//singleton instance getter
RedisDatabase& RedisDatabase::getInstance(){
    static RedisDatabase instance;
    return instance;
}

//common commands
bool RedisDatabase::flushAll(){
    std::lock_guard<std::mutex> lock(db_mutex);
    key_value_store.clear();
    list_store.clear();
    hash_store.clear();

    return true;
}

//Key/Value Operations
void RedisDatabase::set(const std::string &key, const std::string &value){
    std::lock_guard<std::mutex> lock(db_mutex);
    key_value_store[key] = value;
}

bool RedisDatabase::get(const std::string &key, std::string &value){
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = key_value_store.find(key);

    if(it != key_value_store.end()){
        value = it-> second;
        return true;
    }
    return false;
}

std::vector<std::string>RedisDatabase::keys(){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<std::string> allKeys;

    // key-value keys
    for(const auto &pair : key_value_store){
        allKeys.push_back(pair.first);
    }

    //  list keys
    for(const auto &pair : list_store){
        allKeys.push_back(pair.first);
    }

    // hash keys
    for(const auto &pair : hash_store){
        allKeys.push_back(pair.first);
    }
    return allKeys;
}


std::string RedisDatabase::type(const std::string &key){
    std::lock_guard<std::mutex> lock(db_mutex);

    if(key_value_store.find(key) != key_value_store.end()){
        return "string";
    }
    else if(list_store.find(key) != list_store.end()){
        return "list";
    }
    else if(hash_store.find(key) != hash_store.end()){
        return "hash";
    }
    else return "none"; //key does not exist
}

bool RedisDatabase::del(const std::string &key){
    std::lock_guard<std::mutex> lock(db_mutex);
    bool erased = false;
  
    //try to erase the key from all data structures - if it exists in any of them, it will be erased and we return true
    //use bitwise OR to combine the results of erase operations - if any of them erased a key, the result will be true
    erased |= (key_value_store.erase(key) > 0); 
    erased |= (list_store.erase(key) > 0);
    erased |= (hash_store.erase(key) > 0);

    return erased;
}

bool RedisDatabase::expire(const std::string &key, const std::string &seconds){
    std::lock_guard<std::mutex> lock(db_mutex);

    //check if key exists in any of the data structures
    bool exists = (key_value_store.find(key) != key_value_store.end()) ||
                 (list_store.find(key) != list_store.end()) ||
                 (hash_store.find(key) != hash_store.end());

    if(!exists){
        return false;   //key does not exist, cannot set expiry
    }

    expiry_map[key] = std::chrono::steady_clock::now() + std::chrono::seconds(std::stoll(seconds));
    return true;
}

bool RedisDatabase::rename(const std::string &oldKey, const std::string &newKey){
    std::lock_guard<std::mutex> lock(db_mutex);
    
    bool found = false;
    auto it_kv = key_value_store.find(oldKey);
    if(it_kv != key_value_store.end()){
        key_value_store[newKey] = it_kv->second;   //copy value to new key
        key_value_store.erase(it_kv);   //erase old key
        found = true;
    }

    auto it_list = list_store.find(oldKey);
    if(it_list != list_store.end()){
        list_store[newKey] = it_list->second;   //copy list to new key
        list_store.erase(it_list);   //erase old key
        found = true;
    }

    auto it_hash = hash_store.find(oldKey);
    if(it_hash != hash_store.end()){
        hash_store[newKey] = it_hash->second;   //copy hash to new key
        hash_store.erase(it_hash);   //erase old key
        found = true;
    }

    auto it_expiry = expiry_map.find(oldKey);
    if(it_expiry != expiry_map.end()){
        expiry_map[newKey] = it_expiry->second;   //copy expiry to new key
        expiry_map.erase(it_expiry);   //erase old key
    }

    return found;
}

//key - value operations
// list operations
// hash operations

/*
dump function :
memory -> file - dump()

load function :
file -> memory - load()

K - Key value
L - List
H - Hash
*/

bool RedisDatabase::dump(const std::string& filename){
    std::lock_guard<std::mutex> lock(db_mutex);   //lock the mutex for
    std::ofstream ofs(filename, std::ios::binary);
    
    if(!ofs.is_open()){
        return false;
    }

    //serialize data structures and write to file
    for(const auto &kv : key_value_store){
        ofs << "K " << kv.first << " " << kv.second << "\n";
    }

    //serialize list store
    for(const auto &kv : list_store){
        ofs << "L " <<kv.first;

        for(const auto &item : kv.second){
            ofs << " " << item;
        }
        ofs << "\n";
    }
    
    //serialize hash store
    for(const auto& kv : hash_store){
        ofs << "H " << kv.first;
        for(const auto &field_value : kv.second){
            ofs << " " << field_value.first << " " << field_value.second;
        }
    }
    
    return true;
}

bool RedisDatabase::load(const std::string& filename){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::ifstream ifs(filename, std::ios::binary);

    //check if file opened successfully
    if(!ifs.is_open())  return false;
    
    //clear existing data
    key_value_store.clear();
    list_store.clear();
    hash_store.clear();

    std::string line;

    while(std::getline(ifs, line)){
        std::istringstream iss(line);
        char type; iss >> type;
        
        if(type == 'K'){
            std::string key, value;
            iss >> key >> value;
            key_value_store[key] = value;
        } 
        else if (type == 'L'){
            std::string key;
            iss >> key;
            std::vector<std::string> list_item;
            std::string item;

            while(iss >> item){
                list_item.push_back(item);
            }
            list_store[key] = list_item;
        }
        else if (type == 'H'){
            std::string key;
            iss >> key;
            std::unordered_map<std::string, std::string> hash;
            std::string pair;

            // now we have pairs of field and value in the format field:value
            while(iss >> pair){
                auto pos = pair.find(':');
                if(pos != std::string::npos){
                    std::string field = pair.substr(0, pos);    //field is the substring before the colon
                    std::string value = pair.substr(pos + 1);   //value is the substring after the colon
                    hash[field] = value;    //store the field-value pair in the hash
                }
            }
            hash_store[key] = hash;   //store the hash in the hash store
        }
    }
    return true;
}
