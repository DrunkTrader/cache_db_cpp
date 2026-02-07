#include "../include/RedisDatabase.h"

#include <fstream>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

RedisDatabase& RedisDatabase::getInstance(){
    static RedisDatabase instance;
    return instance;
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