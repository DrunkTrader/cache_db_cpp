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
        // ... ECHO message
    }

    //key/value commands (SET, GET, DEL, etc.) would go here
    // list operations
    // hash operations
    
    else {
        response << "-Error: Unknown Command\r\n";
    }

    return response.str();
}