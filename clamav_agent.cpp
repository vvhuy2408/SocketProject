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
#include <vector> // Thêm để dùng cho output của process

#pragma comment(lib, "Ws2_32.lib")

// Cấu hình
constexpr int AGENT_PORT = 9000;
constexpr int BACKLOG = 5;
constexpr int BUF_SIZE = 4096;
const std::string TEMP_DIR = "temp";
const std::string QUARANTINE_DIR = "quarantine";
const std::string LOG_FILE = "agent.log";
// Cấu hình đường dẫn ClamAV
const std::string CLAMAV_PATH = "\"C:\\ClamAV\\clamscan.exe\""; // Sử dụng biến riêng
constexpr int RECV_TIMEOUT_MS = 10000; // 10 giây timeout cho recv

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
    if (log) {
        log << "[" << timestamp() << "] " << msg << std::endl;
    }
    else {
        std::cerr << "[" << timestamp() << "] ERROR: Could not open log file! Message: " << msg << std::endl;
    }
}

// Nhận đủ n bytes
bool recvAll(SOCKET s, char* buf, int64_t len) {
    int64_t total = 0;
    while (total < len) {
        int ret = recv(s, buf + total, (int)(len - total), 0);
        if (ret == 0) { // Client disconnected
            writeLog("recvAll: Client disconnected unexpectedly.");
            return false;
        }
        if (ret < 0) { // Error
            writeLog("recvAll: recv error: " + std::to_string(WSAGetLastError()));
            return false;
        }
        total += ret;
    }
    return true;
}

// Hàm để escape tên file cho command line (đơn giản, cần cải thiện cho production)
std::string escapeFilename(const std::string& filename) {
    std::string escaped = filename;
    // Replace all " with "" (double quote)
    size_t pos = escaped.find('\"');
    while (pos != std::string::npos) {
        escaped.replace(pos, 1, "\"\"");
        pos = escaped.find('\"', pos + 2);
    }
    return "\"" + escaped + "\""; // Wrap the whole thing in quotes
}

// Xử lý 1 client
void handleClient(SOCKET client) {
    std::string client_info = "Unknown";
    sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    if (getpeername(client, (sockaddr*)&client_addr, &client_addr_len) == 0) {
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
        client_info = std::string(ip_str) + ":" + std::to_string(ntohs(client_addr.sin_port));
    }
    writeLog("Handling client from " + client_info);

    std::string tempPath;
    try {
        // Thiết lập timeout cho socket
        DWORD timeout = RECV_TIMEOUT_MS;
        if (setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
            writeLog("setsockopt failed: " + std::to_string(WSAGetLastError()));
            closesocket(client);
            return;
        }

        // Đọc độ dài tên file
        uint32_t netNameLen;
        if (!recvAll(client, (char*)&netNameLen, sizeof(netNameLen))) {
            closesocket(client);
            writeLog("Failed to receive filename length from " + client_info);
            return;
        }
        uint32_t nameLen = ntohl(netNameLen);
        if (nameLen == 0 || nameLen > 255) { // Giới hạn độ dài tên file
            writeLog("Invalid filename length received from " + client_info + ": " + std::to_string(nameLen));
            closesocket(client);
            return;
        }

        // Đọc tên file
        std::string filename(nameLen, '\0');
        if (!recvAll(client, filename.data(), nameLen)) {
            closesocket(client);
            writeLog("Failed to receive filename from " + client_info);
            return;
        }
        // Basic filename validation: Remove path separators if present
        size_t last_slash = filename.find_last_of("\\/");
        if (last_slash != std::string::npos) {
            filename = filename.substr(last_slash + 1);
            writeLog("Stripped path from filename: " + filename);
        }
        if (filename.empty()) {
            writeLog("Received empty filename from " + client_info);
            closesocket(client);
            return;
        }


        // Đọc kích thước
        uint32_t high, low;
        if (!recvAll(client, (char*)&high, sizeof(high)) || !recvAll(client, (char*)&low, sizeof(low))) {
            closesocket(client);
            writeLog("Failed to receive file size from " + client_info);
            return;
        }
        high = ntohl(high);
        low = ntohl(low);
        uint64_t fileSize = (uint64_t(high) << 32) | low;

        // Chuẩn bị thư mục
        std::filesystem::create_directories(TEMP_DIR);
        std::filesystem::create_directories(QUARANTINE_DIR);
        tempPath = TEMP_DIR + "/" + filename;

        // Nhận file
        std::ofstream ofs(tempPath, std::ios::binary);
        if (!ofs) {
            writeLog("ERROR: Could not open file for writing: " + tempPath);
            closesocket(client);
            return;
        }

        char buffer[BUF_SIZE];
        uint64_t remain = fileSize;
        while (remain > 0) {
            int chunk_to_recv = (int)std::min<uint64_t>(BUF_SIZE, remain);
            if (!recvAll(client, buffer, chunk_to_recv)) {
                writeLog("Failed to receive all file data for " + filename + " from " + client_info);
                ofs.close();
                std::filesystem::remove(tempPath); // Clean up incomplete file
                closesocket(client);
                return;
            }
            ofs.write(buffer, chunk_to_recv);
            if (!ofs) { // Check for write errors
                writeLog("ERROR: Failed to write to file " + tempPath + " while receiving.");
                ofs.close();
                std::filesystem::remove(tempPath); // Clean up incomplete file
                closesocket(client);
                return;
            }
            remain -= chunk_to_recv;
        }
        ofs.close();
        writeLog("Received " + filename + " (" + std::to_string(fileSize) + " bytes) from " + client_info);

        // Quét virus bằng clamscan
        auto absPath = std::filesystem::absolute(tempPath).string();
        // Escape (thêm dấu quote, xử lý " bên trong)
        std::string escapedTempPath = escapeFilename(absPath);
        std::string cmd = CLAMAV_PATH + " --no-summary " + escapedTempPath;

        // Sử dụng CreateProcess thay vì system() để kiểm soát tốt hơn
        PROCESS_INFORMATION pi;
        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESTDHANDLES; // Để redirect output
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE); // Redirect to console for now, or to a pipe
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

        // For better output capturing, you'd create pipes and redirect
        // For simplicity, sticking to existing handles for now
        // A more robust solution would be to use pipes and read the output.

        BOOL success = CreateProcessA(
            NULL,
            (LPSTR)cmd.c_str(),
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            // <-- Đặt thư mục làm việc của clamscan.exe là TEMP_DIR
            std::filesystem::path(TEMP_DIR).string().c_str(),
            &si,
            &pi
        );

        int clamscan_exit_code = -1; // Default to error
        if (success) {
            WaitForSingleObject(pi.hProcess, INFINITE); // Wait indefinitely for clamscan to finish
            DWORD exitCode;
            if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
                clamscan_exit_code = (int)exitCode;
            }
            else {
                writeLog("ERROR: GetExitCodeProcess failed for clamscan: " + std::to_string(GetLastError()));
            }
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else {
            writeLog("ERROR: Failed to start clamscan process: " + std::to_string(GetLastError()));
        }

        bool clean = (clamscan_exit_code == 0);
        std::string result = clean ? "OK" : "INFECTED";
        writeLog("Scan " + filename + ": " + result + " (ClamAV Exit Code: " + std::to_string(clamscan_exit_code) + ")");

        // Nếu nhiễm, cách ly
        if (!clean) {
            std::string quarPath = QUARANTINE_DIR + "/" + filename;
            try {
                std::filesystem::rename(tempPath, quarPath);
                writeLog("Quarantined " + filename + " to " + quarPath);
            }
            catch (const std::filesystem::filesystem_error& e) {
                writeLog("ERROR: Failed to quarantine " + filename + ": " + e.what());
                // If quarantine fails, delete the temp file to prevent it from remaining unquarantined
                std::filesystem::remove(tempPath);
                writeLog("Deleted unquarantinable file: " + tempPath);
            }
        }
        else {
            // Nếu sạch, xóa file tạm
            std::filesystem::remove(tempPath);
            writeLog("Clean file " + filename + " deleted from temp.");
        }

        // Gửi kết quả
        send(client, result.c_str(), (int)result.size(), 0);

    }
    catch (const std::exception& e) {
        writeLog("Unhandled exception in handleClient for " + client_info + ": " + e.what());
        // Attempt to clean up temp file if an exception occurred after it was created
        if (!tempPath.empty() && std::filesystem::exists(tempPath)) {
            std::filesystem::remove(tempPath);
            writeLog("Cleaned up temp file " + tempPath + " due to exception.");
        }
    }
    closesocket(client);
    writeLog("Client " + client_info + " disconnected.");
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        writeLog("WSAStartup failed: " + std::to_string(WSAGetLastError()));
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        std::cerr << "socket creation failed: " << WSAGetLastError() << std::endl;
        writeLog("socket creation failed: " + std::to_string(WSAGetLastError()));
        WSACleanup();
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(AGENT_PORT);

    if (bind(listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        writeLog("bind failed: " + std::to_string(WSAGetLastError()));
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    if (listen(listenSock, BACKLOG) == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        writeLog("listen failed: " + std::to_string(WSAGetLastError()));
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    writeLog("ClamAV Agent started on port " + std::to_string(AGENT_PORT));

    // Vòng lặp chấp nhận
    while (true) {
        SOCKET client = accept(listenSock, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            writeLog("accept failed: " + std::to_string(WSAGetLastError()));
            // Depending on the error, might want to exit or log and continue
            if (WSAGetLastError() == WSAEINTR) { // Interrupted call, can be ignored
                continue;
            }
            // For other critical errors, you might want to break the loop or exit
            break;
        }
        std::thread(handleClient, client).detach();
    }

    closesocket(listenSock);
    WSACleanup();
    writeLog("ClamAV Agent shutting down.");
    return 0;
}