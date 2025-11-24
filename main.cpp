#include <iostream>
#include <cstring>
#include "server.h"

void showHelp() {
    std::cout << "Usage: server [OPTIONS]\n"
              << "Options:\n"
              << "  -h              Show this help\n"
              << "  -p PORT         Port number (default: 33333)\n"
              << "  -c CONFIG_FILE  User database file (default: ./vcalc.conf)\n"
              << "  -l LOG_FILE     Log file (default: ./log/vcalc.log)\n";
}

int main(int argc, char* argv[]) {
    int port = 33333;
    std::string configFile = "./vcalc.conf";
    std::string logFile = "./log/vcalc.log";
    
    if (argc == 1) {
        showHelp();
        return 0;
    }
    
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0) {
            showHelp();
            return 0;
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            configFile = argv[++i];
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            logFile = argv[++i];
        }
    }
    
    Server server(port, configFile, logFile);
    std::cout << "Starting server on port " << port << std::endl;
    std::cout << "User database: " << configFile << std::endl;
    std::cout << "Log file: " << logFile << std::endl;
    
    if (!server.start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    return 0;
}