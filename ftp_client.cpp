#include "ftp_client.h"
#pragma comment(lib, "ws2_32.lib")

// Cổng điều khiển FTP tiêu chuẩn
#define FTP_PORT 21

// 1. File and Directory Operations
void handle_ls(FtpSession& session) { 
    if (!session.connected) {
        std::cout << "[Client] Not connected.\n";
        return;
    }

    // Bước 1: gửi lệnh chuyển sang passive mode
    send(session.controlSocket, "PASV\r\n", 6, 0);

    char response[512];
    int len = recv(session.controlSocket, response, sizeof(response) - 1, 0);
    response[len] = '\0';
    std::cout << "[Server] " << response;

    // Bước 2: trích IP:PORT từ phản hồi PASV (ví dụ: 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2))
    int h1, h2, h3, h4, p1, p2;
    sscanf_s(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
        &h1, &h2, &h3, &h4, &p1, &p2);
    int port = p1 * 256 + p2;
    std::string ip = std::to_string(h1) + "." + std::to_string(h2) + "." + std::to_string(h3) + "." + std::to_string(h4);

    // Bước 3: tạo socket data kết nối tới ip:port
    SOCKET dataSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in dataAddr{};
    dataAddr.sin_family = AF_INET;
    dataAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &dataAddr.sin_addr);
    connect(dataSocket, (sockaddr*)&dataAddr, sizeof(dataAddr));

    // Bước 4: gửi lệnh LIST
    send(session.controlSocket, "LIST\r\n", 6, 0);
    len = recv(session.controlSocket, response, sizeof(response) - 1, 0);
    response[len] = '\0';
    std::cout << "[Server] " << response;

    // Bước 5: đọc dữ liệu từ data socket
    char buf[1024];
    while ((len = recv(dataSocket, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[len] = '\0';
        std::cout << buf;
    }
    closesocket(dataSocket);

    // Bước 6: nhận kết thúc từ control socket
    len = recv(session.controlSocket, response, sizeof(response) - 1, 0);
    response[len] = '\0';
    std::cout << "[Server] " << response;
}
void handle_cd(FtpSession& session, const std::string& folder) { 
    if (!session.connected) {
        std::cout << "[Client] Not connected.\n";
        return;
    }

    std::string cmd = "CWD " + folder + "\r\n";
    send(session.controlSocket, cmd.c_str(), cmd.length(), 0);

    char buffer[512];
    int len = recv(session.controlSocket, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';
    std::cout << "[Server] " << buffer;
}
void handle_pwd(FtpSession& session) { 
    if (!session.connected) {
        std::cout << "[Client] Not connected.\n";
        return;
    }

    send(session.controlSocket, "PWD\r\n", 5, 0);

    char buffer[512];
    int len = recv(session.controlSocket, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';
    std::cout << "[Server] " << buffer;
}
void handle_mkdir(FtpSession& session, const std::string& folder) {
    if (!session.connected) {
        std::cout << "[Client] Not connected.\n";
        return;
    }

    std::string cmd = "MKD " + folder + "\r\n";
    send(session.controlSocket, cmd.c_str(), cmd.length(), 0);

    char buffer[512];
    int len = recv(session.controlSocket, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';
    std::cout << "[Server] " << buffer;
}
void handle_rmdir(FtpSession& session, const std::string& folder) {
    if (!session.connected) {
        std::cout << "[Client] Not connected.\n";
        return;
    }

    std::string cmd = "RMD " + folder + "\r\n";
    send(session.controlSocket, cmd.c_str(), cmd.length(), 0);

    char buffer[512];
    int len = recv(session.controlSocket, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';
    std::cout << "[Server] " << buffer;
}

void handle_delete(FtpSession& session, const std::string& filename) {
    if (!session.connected) {
        std::cout << "[Client] Not connected.\n";
        return;
    }

    std::string cmd = "DELE " + filename + "\r\n";
    send(session.controlSocket, cmd.c_str(), cmd.length(), 0);

    char buffer[512];
    int len = recv(session.controlSocket, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';
    std::cout << "[Server] " << buffer;
}
void handle_rename(FtpSession& session, const std::string& oldname, const std::string& newname) {
    if (!session.connected) {
        std::cout << "[Client] Not connected.\n";
        return;
    }

    std::string cmd1 = "RNFR " + oldname + "\r\n";
    send(session.controlSocket, cmd1.c_str(), cmd1.length(), 0);

    char buffer[512];
    int len = recv(session.controlSocket, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';
    std::cout << "[Server] " << buffer;

    std::string cmd2 = "RNTO " + newname + "\r\n";
    send(session.controlSocket, cmd2.c_str(), cmd2.length(), 0);
    len = recv(session.controlSocket, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';
    std::cout << "[Server] " << buffer;
}

// 3. Session management

// Hàm kết nối đến FTP server
void handle_open(FtpSession& session, const std::string& ip) {
    if (session.connected) {
        std::cout << "[Client] Connected.\n";
        return;
    }

    // Tạo socket TCP
    session.controlSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (session.controlSocket == INVALID_SOCKET) {
        std::cerr << "[Client] Failed to initialize socket.\n";
        return;
    }

    // Thiết lập địa chỉ server
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(FTP_PORT);
    inet_pton(AF_INET, ip.c_str(), &server.sin_addr);

    // Kết nối đến server
    if (connect(session.controlSocket, (sockaddr*)&server, sizeof(server)) == 0) {
        session.connected = true;
        std::cout << "[Client] Connected to " << ip << "\n";

        // Nhận dòng chào từ server
        char buffer[512] = { 0 };
        int len = recv(session.controlSocket, buffer, sizeof(buffer) - 1, 0);
        if (len > 0) {
            buffer[len] = '\0';
            std::cout << "[Server]: " << buffer;
        }
    }
    else {
        std::cerr << "[Client] Connection failed.\n";
        closesocket(session.controlSocket);
    }
}

// Hàm đóng kết nối
void handle_close(FtpSession& session) {
    if (session.connected) {
        send(session.controlSocket, "QUIT\r\n", 6, 0);
        closesocket(session.controlSocket);
        session.connected = false;
        std::cout << "[Client] Connection closed.\n";
    }
    else {
        std::cout << "[Client] Not connected.\n";
    }
}

// Hàm in trạng thái phiên hiện tại
void handle_status(const FtpSession& session) {
    if (session.connected)
        std::cout << "[Client] Connecting to FTP Server.\n";
    else
        std::cout << "[Client] Not connected.\n";
}

// Hàm hiển thị danh sách lệnh hỗ trợ
void handle_help() {
    std::cout << "Command:\n";
    std::cout << "open <ip>     - Connect to FTP server\n";
    std::cout << "close         - Disconnect to FTP server\n";
    std::cout << "status        - Show current session status\n";
    std::cout << "help / ?      - Show help text for commands\n";
    std::cout << "quit / bye    - Exit the FTP client\n";
}

// Hàm thoát chương trình
void handle_quit(FtpSession& session) {
    handle_close(session);  // Đảm bảo đóng kết nối nếu còn
    std::cout << "[Client] Good bye!\n";
    WSACleanup();
    exit(0);
}
void hande_bye(FtpSession& session) {
    handle_quit(session);
}