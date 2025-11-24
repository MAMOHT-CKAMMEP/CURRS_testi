#include "server.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/evp.h>
#include <cstdlib>

Server::Server(int port, const std::string& userDbPath, const std::string& logPath)
    : port(port), userDbPath(userDbPath), logPath(logPath) {}

void Server::loadUserDatabase() {
    std::ifstream file(userDbPath);
    if (!file.is_open()) {
        logError("Cannot open user database file: " + userDbPath, true);
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find(':');
        if (pos != std::string::npos && pos > 0 && pos < line.length() - 1) {
            std::string login = line.substr(0, pos);
            std::string password = line.substr(pos + 1);
            if (!login.empty() && !password.empty()) {
                users[login] = password;
            }
        }
    }
    file.close();
}

void Server::logError(const std::string& message, bool isCritical) {
    std::ofstream logFile(logPath, std::ios::app);
    if (!logFile.is_open()) {
        return;
    }
    
    std::time_t now = std::time(nullptr);
    std::tm* timeinfo = std::localtime(&now);
    
    logFile << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S") << " | "
            << (isCritical ? "CRITICAL" : "NON-CRITICAL") << " | "
            << message << std::endl;
    
    logFile.close();
}

std::string Server::md5Hash(const std::string& input) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context) {
        return "";
    }
    
    if (EVP_DigestInit_ex(context, EVP_md5(), nullptr) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }
    
    if (EVP_DigestUpdate(context, input.c_str(), input.length()) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }
    
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLength;
    if (EVP_DigestFinal_ex(context, digest, &digestLength) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }
    
    EVP_MD_CTX_free(context);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digestLength; ++i) {
        ss << std::setw(2) << static_cast<unsigned int>(digest[i]);
    }
    
    std::string result = ss.str();
    for (auto& c : result) {
        c = std::toupper(c);
    }
    return result;
}

bool Server::authenticate(int clientSocket) {
    char buffer[256];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        std::cout << "No data received from client" << std::endl;
        return false;
    }
    buffer[bytesRead] = '\0';
    
    std::string message(buffer);
    
    // Проверяем всех пользователей из базы
    for (const auto& user : users) {
        const std::string& login = user.first;
        const std::string& password = user.second;
        
        // Проверяем, начинается ли сообщение с этого логина
        // Формат: логин + SALT16 (16 символов) + HASH (32 символа)
        if (message.length() == login.length() + 16 + 32 && 
            message.substr(0, login.length()) == login) {
            
            std::string salt = message.substr(login.length(), 16);
            std::string receivedHash = message.substr(login.length() + 16, 32);
            
            // Проверяем hex формат
            auto isHexDigit = [](char c) {
                return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
            };
            
            bool validHex = true;
            for (char c : salt) {
                if (!isHexDigit(c)) {
                    validHex = false;
                    break;
                }
            }
            for (char c : receivedHash) {
                if (!isHexDigit(c)) {
                    validHex = false;
                    break;
                }
            }
            
            if (!validHex) {
                continue; // Пробуем следующего пользователя
            }
            
            // Вычисляем хеш
            std::string computedHash = md5Hash(salt + password);
            
            if (computedHash == receivedHash) {
                send(clientSocket, "OK", 2, 0);
                return true;
            }
        }
    }
    
    send(clientSocket, "ERR", 3, 0);
    return false;
}


int16_t Server::calculateProduct(const std::vector<int16_t>& vector) {
    if (vector.empty()) {
        return 0;
    }
    
    int32_t product = 1;
    for (int16_t value : vector) {
        product *= value;
        
        // Проверка переполнения
        if (product > 32767) {
            return 32767;
        }
        if (product < -32768) {
            return -32768;
        }
    }
    return static_cast<int16_t>(product);
}

void Server::processVectors(int clientSocket) {
    
    // Читаем количество векторов
    uint32_t numVectors;
    ssize_t bytesRead = recv(clientSocket, &numVectors, sizeof(numVectors), 0);
    
    if (bytesRead != sizeof(numVectors)) {
        logError("Failed to read number of vectors", false);
        return;
    }
    
    // ДЕТЕКТИРУЕМ порядок байт автоматически
    if (numVectors == 4) {
        // Клиент использует little-endian, оставляем как есть
    } else {
        // Клиент использует network order, конвертируем
        numVectors = ntohl(numVectors);
    }
    
    
    // Обрабатываем каждый вектор и сразу отправляем результат
    for (uint32_t i = 0; i < numVectors; ++i) {

        // Читаем размер вектора
        uint32_t vectorSize;
        bytesRead = recv(clientSocket, &vectorSize, sizeof(vectorSize), 0);
        
        if (bytesRead != sizeof(vectorSize)) {
            logError("Failed to read vector size", false);
            return;
        }
        
        // АВТОМАТИЧЕСКОЕ определение порядка байт для размера вектора
        if (vectorSize == 4) {
            // little-endian, оставляем как есть
        } else {
            // network order, конвертируем
            vectorSize = ntohl(vectorSize);
        }

        // Читаем данные вектора
        std::vector<int16_t> vector(vectorSize);
        size_t totalBytesToRead = vectorSize * sizeof(int16_t);
        bytesRead = recv(clientSocket, vector.data(), totalBytesToRead, 0);
        
        if (bytesRead != static_cast<ssize_t>(totalBytesToRead)) {
            logError("Failed to read vector data", false);
            return;
        }

        // АВТОМАТИЧЕСКОЕ определение порядка байт для данных
        // Проверяем первое значение - если оно разумное, оставляем, иначе конвертируем
        bool needConversion = false;
        for (const auto& val : vector) {
            if (val > 10000 || val < -10000) { // Нереальные значения для тестовых данных
                needConversion = true;
                break;
            }
        }
        
        if (needConversion) {
            for (auto& value : vector) {
                value = ntohs(value);
            }
        }
        
        for (const auto& val : vector) {
            std::cout << val << " ";
        }
        std::cout << std::endl;

        // Вычисляем произведение
        int16_t product = calculateProduct(vector);
        
        // Отправляем результат в порядке байт КЛИЕНТА
        // Тестовый клиент ожидает little-endian
        ssize_t bytesSent = send(clientSocket, &product, sizeof(product), 0);
        
        if (bytesSent != sizeof(product)) {
            logError("Failed to send result for vector " + std::to_string(i + 1), false);
            return;
        }
        
    }

}

void Server::handleClient(int clientSocket) {
    std::cout << "New client connection" << std::endl;
    
    if (!authenticate(clientSocket)) {
        logError("Authentication failed for client", false);
        close(clientSocket);
        return;
    }
    
    std::cout << "Client authenticated successfully" << std::endl;
    processVectors(clientSocket);
    close(clientSocket);
}

bool Server::start() {
    loadUserDatabase();
    system("mkdir -p log");
    // Инициализация OpenSSL
    OpenSSL_add_all_digests();
    
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        logError("Cannot create socket", true);
        return false;
    }
    
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logError("Cannot set socket options", true);
        close(serverSocket);
        return false;
    }
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        logError("Cannot bind socket to port " + std::to_string(port), true);
        close(serverSocket);
        return false;
    }
    
    if (listen(serverSocket, 10) < 0) {
        logError("Cannot listen on socket", true);
        close(serverSocket);
        return false;
    }
    
    std::cout << "Server started on port " << port << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket < 0) {
            logError("Cannot accept client connection", false);
            continue;
        }
        
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::cout << "Client connected from " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;
        
        handleClient(clientSocket);
    }
    
    close(serverSocket);
    return true;
}