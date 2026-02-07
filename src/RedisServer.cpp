#include <../include/RedisServer.h>
#include <../include/RedisCommandHandler.h>
#include <../include/RedisDatabase.h>

#include <iostream>
#include <format>
#include <cerrno>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <cstring>
#include <signal.h>

// Global pointer to allow signal handlers to access the server instance
static RedisServer *globalServer = nullptr;

void signalHandler(int signum){
    if(globalServer){
        std::cout << "\nSignal " << signum << " received. Shutting down server...\n";
        globalServer->shutdown();
    }
    exit(signum);
}

void RedisServer::setupSignalHandler(){
    signal(SIGINT, signalHandler);  // Handle Ctrl+C
}


// Signal handler for graceful shutdown
RedisServer::RedisServer(int port) : port(port), server_socket(-1), running(true)
{
    globalServer = this;
}

// server shutdown implementation
void RedisServer::shutdown(){
    running = false;

    if (server_socket != -1){
        close(server_socket);
    }
    std::cout << "Server Shutdown Complete!\n";
}

// server run implementation
void RedisServer::run(){
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){
        std::cerr << "Error Creating Server Socket\n";
        return;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Error Binding Server Socket.\n";
        return;
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) < 0)
    {
        std::cerr << "Error Listening On Server Socket.\n";
        return;
    }

    std::cout << std::format("Redis Server listening on Port {}.\n", port);

    // Vector to hold client handler threads
    std::vector<std::thread> threads;
    RedisCommandHandler cmdHandler;

    // Main server loop to accept and handle client connections
    while (running)
    {
        int clientSocket = accept(server_socket, nullptr, nullptr);
        if (clientSocket < 0)
        {
            if (running)
            {
                std::cerr << "Error Accepting Client Connection.\n";
                break;
            }
        }

        // Handle client in a separate thread
        threads.emplace_back([clientSocket, &cmdHandler]()
                             {
        char buffer[1024];

        //loop to receive and process commands from the client
        while(true){
            memset(buffer, 0, sizeof(buffer));
            int bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if(bytes <= 0) break; //connection closed or error
            std::string request(buffer, bytes);
            std::string response = cmdHandler.processCommand(request);
            send(clientSocket, response.c_str(), response.size(), 0);
        }
        close(clientSocket); });
    }
    // Join all client handler threads before exiting
    for (auto &t : threads) {
        if (t.joinable()) t.join();
    }

    // before exiting, persist/dump the database to disk
    if (RedisDatabase::getInstance().dump("dump.my_rdb")){
        std::cout << "Database Dumped to dump.my_rdb on Shutdown.\n";
    }
    else{
        std::cerr << "Error Dumping Database on Shutdown.\n";
    } 
}