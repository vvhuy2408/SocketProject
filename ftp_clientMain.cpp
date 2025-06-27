#include "ftp_client.h"
int main() {
    // Khởi động Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[Client] Failed to initialize Winsock.\n";
        return 1;
    }

    FtpSession session;

    // Vòng lặp nhận lệnh từ người dùng
    while (true) {
        std::cout << "ftp> ";
        std::string line;
        std::getline(std::cin, line);
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "open") {
            std::string ip;
            iss >> ip;
            handle_open(session, ip);
        }
        else if (cmd == "close") {
            handle_close(session);
        }
        else if (cmd == "status") {
            handle_status(session);
        }
        else if (cmd == "quit" || cmd == "bye") {
            handle_quit(session);
        }
        else if (cmd == "help" || cmd == "?") {
            handle_help();
        }

        // ==== Lệnh điều hướng & quản lý file ====
        else if (cmd == "ls") {
            handle_ls(session);
        }
        else if (cmd == "cd") {
            std::string folder;
            iss >> folder;
            handle_cd(session, folder);
        }
        else if (cmd == "pwd") {
            handle_pwd(session);
        }
        else if (cmd == "mkdir") {
            std::string folder;
            iss >> folder;
            handle_mkdir(session, folder);
        }
        else if (cmd == "rmdir") {
            std::string folder;
            iss >> folder;
            handle_rmdir(session, folder);
        }
        else if (cmd == "delete") {
            std::string filename;
            iss >> filename;
            handle_delete(session, filename);
        }
        else if (cmd == "rename") {
            std::string oldname, newname;
            iss >> oldname >> newname;
            handle_rename(session, oldname, newname);
        }

        // ==== Lệnh không hợp lệ ====
        else {
            std::cout << "[Client] Invalid command. Type 'help' to display the command list.\n";
        }
    }

    return 0;
}
