// clamav_agent.cpp
// ClamAV Agent: nhận file qua socket, quét với ClamAV, trả kết quả "OK" hoặc "INFECTED"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "Ws2_32.lib")

// Cấu hình
constexpr int AGENT_PORT = 9000;
constexpr int BACKLOG = 5;
constexpr int BUF_SIZE = 4096;
const std::string TEMP_DIR = "temp";
const std::string QUARANTINE_DIR = "quarantine";
const std::string LOG_FILE = "agent.log";

// Lấy timestamp dạng [YYYY-MM-DD HH:MM:SS]
std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// Ghi log đơn giản
void writeLog(const std::string& msg) {
    std::ofstream log(LOG_FILE, std::ios::app);
    if (log) log << "[" << timestamp() << "] " << msg << std::endl;
}

// Nhận đủ n bytes
bool recvAll(SOCKET s, char* buf, int64_t len) {
    int64_t total = 0;
    while (total < len) {
        int ret = recv(s, buf + total, (int)(len - total), 0);
        if (ret <= 0) return false;
        total += ret;
    }
    return true;
}

// Xử lý 1 client
void handleClient(SOCKET client) {
    // Đọc độ dài tên file
    uint32_t netNameLen;
    if (!recvAll(client, (char*)&netNameLen, sizeof(netNameLen))) {
        closesocket(client);
        return;
    }
    uint32_t nameLen = ntohl(netNameLen);
    // Đọc tên file
    std::string filename(nameLen, '\0');
    if (!recvAll(client, filename.data(), nameLen)) {
        closesocket(client);
        return;
    }
    // Đọc kích thước
    uint32_t high, low;
    recvAll(client, (char*)&high, sizeof(high));
    recvAll(client, (char*)&low, sizeof(low));
    high = ntohl(high);
    low = ntohl(low);
    uint64_t fileSize = (uint64_t(high) << 32) | low;
    // Chuẩn bị thư mục
    std::filesystem::create_directories(TEMP_DIR);
    std::filesystem::create_directories(QUARANTINE_DIR);
    std::string tempPath = TEMP_DIR + "/" + filename;
    // Nhận file
    std::ofstream ofs(tempPath, std::ios::binary);
    char buffer[BUF_SIZE];
    uint64_t remain = fileSize;
    while (remain > 0) {
        int chunk = (int)std::min<uint64_t>(BUF_SIZE, remain);
        if (!recvAll(client, buffer, chunk)) break;
        ofs.write(buffer, chunk);
        remain -= chunk;
    }
    ofs.close();
    writeLog("Received " + filename + " (" + std::to_string(fileSize) + " bytes)");
    // Quét virus bằng clamscan
    std::string cmd = "\"C:\\Program Files\\ClamAV\\clamscan.exe\" --no-summary \"" + tempPath + "\"";
    writeLog("Running: " + cmd);
    int ret = system(cmd.c_str());
    bool clean = (ret == 0);
    std::string result = clean ? "OK" : "INFECTED";
    writeLog("Scan " + filename + ": " + result);
    // Nếu nhiễm, cách ly
    if (!clean) {
        std::string quarPath = QUARANTINE_DIR + "/" + filename;
        std::filesystem::rename(tempPath, quarPath);
        writeLog("Quarantined " + filename);
    }
    // Gửi kết quả
    send(client, result.c_str(), (int)result.size(), 0);
    closesocket(client);
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(AGENT_PORT);
    bind(listenSock, (sockaddr*)&addr, sizeof(addr));
    listen(listenSock, BACKLOG);
    writeLog("ClamAV Agent started on port " + std::to_string(AGENT_PORT));
    // Vòng lặp chấp nhận
    while (true) {
        SOCKET client = accept(listenSock, nullptr, nullptr);
        if (client != INVALID_SOCKET) {
            std::thread(handleClient, client).detach();
        }
    }
    WSACleanup();
    return 0;
}
