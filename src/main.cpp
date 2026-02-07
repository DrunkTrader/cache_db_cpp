#include "RedisServer.h"
#include "RedisDatabase.h"

#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]){
    int port = 6379;    //default port
    if(argc >= 2) port = std::stoi(argv[1]);

    RedisServer server(port);

    //Background persistance : dump the database every 300 seconds (5 minutes, save the database)
    std::thread persistanceThread([](){
        while(true){
            std::this_thread::sleep_for(std::chrono::seconds(300));
            
            //dump the database 
            if(!RedisDatabase::getInstance().dump("dump.my_rdb")){
                std::cerr << "Error Dumping Database\n";
            }
            else{
                std::cout << "Database Dumped Successfully\n";
            }
        }
    });

    persistanceThread.detach();

    server.run();
    return 0;
}