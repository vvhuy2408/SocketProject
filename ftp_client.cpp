#include "ftp_client.h"
#pragma comment(lib, "ws2_32.lib")

// Cổng điều khiển FTP tiêu chuẩn
#define FTP_PORT 9000

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

// 2. Upload and download

void handle_put(FtpSession& session, const std::string& localFile, const std::string& remoteFile) {
    if (!session.connected) {
        std::cout << "[Client] Not connected.\n";
        return;
    }

    if (!scan_file_with_clamav(localFile)) {
        std::cout << "[Client] File \"" << localFile << "\" is INFECTED. Upload aborted.\n";
        return;
    }

    // Gửi PASV
    send(session.controlSocket, "PASV\r\n", 6, 0);
    char response[512];
    int len = recv(session.controlSocket, response, sizeof(response) - 1, 0);
    if (len <= 0) {
        std::cerr << "[Client] Failed to receive PASV response.\n";
        return;
    }
    response[len] = '\0';
    std::cout << "[Server] " << response;

    int h1, h2, h3, h4, p1, p2;
    sscanf_s(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
    int port = p1 * 256 + p2;
    std::string ip = std::to_string(h1) + "." + std::to_string(h2) + "." + std::to_string(h3) + "." + std::to_string(h4);

    SOCKET dataSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in dataAddr{};
    dataAddr.sin_family = AF_INET;
    dataAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &dataAddr.sin_addr);
    if (connect(dataSocket, (sockaddr*)&dataAddr, sizeof(dataAddr)) != 0) {
        std::cerr << "[Client] Cannot connect to data socket.\n";
        closesocket(dataSocket);
        return;
    }

    std::string cmd = "STOR " + remoteFile + "\r\n";
    send(session.controlSocket, cmd.c_str(), cmd.length(), 0);
    len = recv(session.controlSocket, response, sizeof(response) - 1, 0);
    if (len <= 0) {
        std::cerr << "[Client] No response after STOR command.\n";
        closesocket(dataSocket);
        return;
    }
    response[len] = '\0';
    std::cout << "[Server] " << response;

    // Gửi file
    std::ifstream file(localFile, std::ios::binary);
    char buffer[4096];
    while (file.read(buffer, sizeof(buffer))) {
        sendAll(dataSocket, buffer, file.gcount());
    }
    if (file.gcount() > 0) {
        sendAll(dataSocket, buffer, file.gcount());
    }
    file.close();
    closesocket(dataSocket);

    //Nhận thông báo kết thúc từ server
    len = recv(session.controlSocket, response, sizeof(response) - 1, 0);
    if (len <= 0) {
        std::cerr << "[Client] Lost connection or no response after file upload.\n";
    }
    else {
        response[len] = '\0';
        std::cout << "[Server] " << response;
    }

    std::cout << "[DEBUG] Done handle_put()\n";  // Kiểm tra client có về vòng lặp không
}

void handle_mput(FtpSession& session, const std::vector<std::string>& files) {
    for (const auto& file : files) {
        if (confirm_prompt(file)) {
            handle_put(session, file, file);
        }
    }
}

void handle_get(FtpSession& session, const std::string& remoteFile, const std::string& localFile) {
    if (!session.connected) {
        std::cout << "[Client] Not connected.\n";
        return;
    }

    send(session.controlSocket, "PASV\r\n", 6, 0);
    char response[512];
    int len = recv(session.controlSocket, response, sizeof(response) - 1, 0);
    response[len] = '\0';
    std::cout << "[Server] " << response;

    int h1, h2, h3, h4, p1, p2;
    sscanf_s(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
    int port = p1 * 256 + p2;
    std::string ip = std::to_string(h1) + "." + std::to_string(h2) + "." + std::to_string(h3) + "." + std::to_string(h4);

    SOCKET dataSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in dataAddr{};
    dataAddr.sin_family = AF_INET;
    dataAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &dataAddr.sin_addr);
    connect(dataSocket, (sockaddr*)&dataAddr, sizeof(dataAddr));

    std::string cmd = "RETR " + remoteFile + "\r\n";
    send(session.controlSocket, cmd.c_str(), cmd.length(), 0);
    len = recv(session.controlSocket, response, sizeof(response) - 1, 0);
    response[len] = '\0';
    std::cout << "[Server] " << response;

    std::ofstream file(localFile, std::ios::binary);
    char buffer[4096];
    while ((len = recv(dataSocket, buffer, sizeof(buffer), 0)) > 0) {
        file.write(buffer, len);
    }
    file.close();
    closesocket(dataSocket);

    len = recv(session.controlSocket, response, sizeof(response) - 1, 0);
    response[len] = '\0';
    std::cout << "[Server] " << response;
}

void handle_mget(FtpSession& session, const std::vector<std::string>& files) {
    for (const auto& file : files) {
        if (confirm_prompt(file)) {
            handle_get(session, file, file);
        }
    }
}

// Biến toàn cục để bật/tắt chế độ xác nhận khi truyền nhiều file
bool prompt_enabled = true;

// Hàm xác nhận từ người dùng trước khi truyền từng file
bool confirm_prompt(const std::string& filename) {
    if (!prompt_enabled) return true; // Nếu prompt bị tắt, luôn cho phép
    std::string answer;
    std::cout << "Transfer file \"" << filename << "\"? (y/n): ";
    std::getline(std::cin, answer);
    return (answer == "y" || answer == "Y"); // Trả về true nếu người dùng đồng ý
}

// Hàm xử lý lệnh prompt từ người dùng: bật/tắt xác nhận
void handle_prompt(const std::string& arg) {
    if (arg == "on") {
        prompt_enabled = true;
        std::cout << "[Client] Prompt enabled.\n";
    }
    else if (arg == "off") {
        prompt_enabled = false;
        std::cout << "[Client] Prompt disabled.\n";
    }
    else {
        std::cout << "[Client] Usage: prompt on|off\n"; // Hướng dẫn sử dụng
    }
}

// Hàm gửi đủ 'len' byte dữ liệu qua socket 's'
bool sendAll(SOCKET s, const char* buf, int64_t len) {
    int64_t total = 0;
    while (total < len) {
        int ret = send(s, buf + total, (int)(len - total), 0);
        if (ret <= 0) return false; // Lỗi khi gửi
        total += ret;
    }
    return true; // Gửi thành công toàn bộ dữ liệu
}

bool scan_file_with_clamav(const std::string& filepath) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[ClamAV] Failed to create socket.\n";
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        std::cerr << "[ClamAV] Cannot connect to ClamAV Agent.\n";
        closesocket(sock);
        return false;
    }

    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "[ClamAV] Cannot open file for scanning.\n";
        closesocket(sock);
        return false;
    }

    uint64_t size = file.tellg();
    file.seekg(0);
    std::string filename = std::filesystem::path(filepath).filename().string();

    // Gửi tên file
    uint32_t nameLen = htonl((uint32_t)filename.size());
    send(sock, (char*)&nameLen, sizeof(nameLen), 0);
    send(sock, filename.c_str(), (int)filename.size(), 0);

    // Gửi kích thước file
    uint32_t high = htonl((uint32_t)(size >> 32));
    uint32_t low = htonl((uint32_t)(size & 0xFFFFFFFF));
    send(sock, (char*)&high, sizeof(high), 0);
    send(sock, (char*)&low, sizeof(low), 0);

    // Gửi nội dung file
    char buffer[4096];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        send(sock, buffer, (int)file.gcount(), 0);
    }

    // Nhận kết quả
    char result[16] = {};
    recv(sock, result, sizeof(result) - 1, 0);
    closesocket(sock);

    return std::string(result) == "OK";
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

        //// Nhận dòng chào từ server
        //char buffer[512] = { 0 };
        //int len = recv(session.controlSocket, buffer, sizeof(buffer) - 1, 0);
        //if (len > 0) {
        //    buffer[len] = '\0';
        //    std::cout << "[Server]: " << buffer;
        //}
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
    std::cout << "\nAvailable commands:\n";

    std::cout << "\n--- Session management ---\n";
    std::cout << "  open <ip>             Connect to FTP server\n";
    std::cout << "  close                 Disconnect from server\n";
    std::cout << "  status                Show connection status\n";
    std::cout << "  quit / bye            Exit the FTP client\n";
    std::cout << "  help / ?              Show this help message\n";

    std::cout << "\n--- File and directory operations ---\n";
    std::cout << "  ls                    List files/folders on the server\n";
    std::cout << "  cd <dir>              Change current directory\n";
    std::cout << "  pwd                   Show current directory on server\n";
    std::cout << "  mkdir <dir>           Create directory on the server\n";
    std::cout << "  rmdir <dir>           Remove directory from the server\n";
    std::cout << "  delete <file>         Delete a file on the server\n";
    std::cout << "  rename <old> <new>    Rename a file on the server\n";

    std::cout << "\n--- File transfers ---\n";
    std::cout << "  put <local> <remote>  Upload a file (with virus scan)\n";
    std::cout << "  mput <file1> [...]    Upload multiple files\n";
    std::cout << "  get <remote> <local>  Download a file\n";
    std::cout << "  mget <file1> [...]    Download multiple files\n";

    std::cout << "\n--- Prompt & scan options ---\n";
    std::cout << "  prompt [on|off]       Enable/disable confirmation before mput/mget\n";

    std::cout << std::endl;
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
