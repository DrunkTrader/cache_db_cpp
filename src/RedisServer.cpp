#include "include/RedisServer.h"

#include <iostream>
#include <format>
#include <cerrno>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// Global pointer to allow signal handlers to access the server instance
static RedisServer* globalServer = nullptr;

// Signal handler for graceful shutdown
RedisServer::RedisServer(int port) : port(port), server_socket(-1), running(true){
    globalServer = this;
}

// server shutdown implementation
void RedisServer::shutdown(){
    running = false;

    if(server_socket != -1){
        close(server_socket);
    }
    std::cout << "Server Shutdown Complete!\n";
}

// server run implementation
void RedisServer::run(){
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0){
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
    if (bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
        std::cerr << "Error Binding Server Socket.\n";
        return;
    }
    
    // Listen for incoming connections
    if (listen(server_socket, 10) < 0){
        std::cerr << "Error Listening On Server Socket.\n";
        return;
    }

    std::cout << std::format("Redis Server listening on Port {}.\n", port);
}