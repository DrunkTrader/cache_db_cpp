#include <../include/RedisCommandHandler.h>
#include <../include/RedisDatabase.h>

#include <iostream>
#include <vector>
#include <sstream>
#include <algorithm>
/*
# RESP PARSER:

*2\r\n$4\r\n\PING\r\n$4\r\nTEST\r\n

*2 -> array has 2 elements
$4 -> next string has 4 characters
c
//PING
//TEST
*/

// Function to parse RESP-formatted command
std::vector<std::string> parseRespCommand(const std::string &input){
    
    std::vector<std::string> tokens;    //to store parsed tokens
    size_t pos = 0;

    //if it doesn't start with '*'
    if(input[pos] != '*') {
        std::istringstream iss(input);
        std::string token;
        while(iss >> token){
            tokens.push_back(token);
        }
        return tokens;  //no RESP format, return space-separated tokens
    }

    //expect '*' followed by number of elements
    if(input[pos] != '*') return tokens;
    pos++;  //skip '*'

    //crlf = carriage return (\r), line feed (\n)
    size_t crlf = input.find("\r\n", pos);
    if(crlf == std::string::npos) return tokens;

    int numElements = std::stoi(input.substr(pos, crlf - pos));
    pos = crlf + 2;

    for(int i = 0; i < numElements; i++){
        if(pos >= input.size() || input[pos] != '$') break; //check format error
        pos++; //skip '$'
        
        //find length of the string
        crlf = input.find("\r\n", pos);
        if(crlf == std::string::npos) break;
        int len = std::stoi(input.substr(pos, crlf - pos));

        //move pos to the start of the string
        pos = crlf + 2; //skip \r\n
        if (pos + len > input.size()) break;

        //extract the string
        std::string token = input.substr(pos, len);
        tokens.push_back(token);
        pos += len + 2; //skip token (the \r\n after the token)
    }
    return tokens;
}

// Default constructor
RedisCommandHandler::RedisCommandHandler() {}  

std::string RedisCommandHandler::processCommand(const std::string& commandLine){
    
    //use RESP parser
    auto tokens = parseRespCommand(commandLine);
    if(tokens.empty()) return "-Error: Empty Command\r\n";

    // For debugging: print the command and tokens
    //std::cout << commandLine << "\n";
    // for(auto &t : tokens) std::cout << t << "\n";

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    std::ostringstream response;

    // connect to database
    RedisDatabase &db = RedisDatabase::getInstance(); //get the singleton database instance

    //check command and execute
    if(cmd == "PING"){
        response << "+PONG\r\n";
    }
    else if(cmd == "ECHO"){
        if(tokens.size() < 2){
            response << "-Error : ECHO requires an argument\r\n";
        }
        else {
            response << "+" << tokens[1] << "\r\n";
        }
    }
    else if(cmd == "FLUSHALL"){
        //clear all data in the database
        {
            db.flushAll();
            response << "+OK\r\n";
        }
    }

    //key/value commands (SET, GET, DEL, etc.) would go here
    else if (cmd == "SET"){
        if(tokens.size() < 3){ // set key value
            response << "-Error: SET requires key and value\r\n";
        }
        else {
            db.set(tokens[1], tokens[2]);
            response << "+OK\r\n";
        }
    }
    else if (cmd == "GET"){
        if(tokens.size() < 2){
            response << "-Error: GET requires a key\r\n";
        }
        else {
            std::string value;

            if(db.get(tokens[1], value)){
                response << "$" << value.size() << "\r\n" << value << "\r\n";
            }
            else {
                response << "$-1\r\n";  //null bulk string for non-existent key
            }
        }
    }
    else if(cmd == "KEYS"){
        std::vector<std::string> allKeys = db.keys();
        response << "*" << allKeys.size() << "\r\n";
        for(const auto &key : allKeys){
            response << "$" << key.size() << "\r\n" << key << "\r\n";
        }
    }
    else if(cmd == "TYPE"){
        if(tokens.size() < 2){
            response << "-Error: TYPE requires key\r\n";
        }
        else {
            response << "+" << db.type(tokens[1]) << "\r\n";
        }
    }
    else if (cmd == "DEL" || cmd == "UNLINK"){
        if(tokens.size() < 2){
            response << "Error: " << cmd << " requires key\r\n";
        }
        else {
            bool deleted = db.del(tokens[1]);
            response << ":" << (deleted ? "1" : "0") << "\r\n"; //integer reply: 1 if key was deleted, 0 if key did not exist
        }
    } else if(cmd == "EXPIRE"){
        if(tokens.size() < 3){  // EXPIRE key seconds
            response << "-Error: EXPIRE requires key and seconds\r\n";
        }
        else {
            int seconds = std::stoi(tokens[2]);
            bool success = db.expire(tokens[1], seconds);
            response << ":" << (success ? "1" : "0") << "\r\n"; //integer reply: 1 if timeout was set, 0 if key does not exist

        }
    }
    // list operations
    // hash operations
    
    else {
        response << "-Error: Unknown Command\r\n";
    }

    return response.str();
}