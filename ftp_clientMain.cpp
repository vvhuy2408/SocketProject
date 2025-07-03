#include "ftp_client.h"

int main() {
    // Khởi tạo Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[Client] Failed to initialize Winsock.\n";
        return 1;
    }

    FtpSession session;

    std::cout << "Secure FTP Client with ClamAV Scanner\n";
    std::cout << "Type 'help' or '?' to see available commands.\n";

    while (true) {
        std::cout << "ftp> ";
        std::string line;
        std::getline(std::cin, line);

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        // Session management
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

        // File and directory operations
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

        // Upload and download
        else if (cmd == "put") {
            std::string localFile, remoteFile;
            iss >> localFile >> remoteFile;
            handle_put(session, localFile, remoteFile);
        }
        else if (cmd == "mput") {
            std::vector<std::string> files;
            std::string file;
            while (iss >> file) files.push_back(file);
            handle_mput(session, files);
        }
        else if (cmd == "get") {
            std::string remoteFile, localFile;
            iss >> remoteFile >> localFile;
            handle_get(session, remoteFile, localFile);
        }
        else if (cmd == "mget") {
            std::vector<std::string> files;
            std::string file;
            while (iss >> file) files.push_back(file);
            handle_mget(session, files);
        }

        // Prompt confirmation toggle
        else if (cmd == "prompt") {
            std::string arg;
            iss >> arg;
            handle_prompt(arg);
        }

        // Unknown command
        else {
            std::cout << "[Client] Invalid command. Type 'help' to display the command list.\n";
        }
    }

    return 0;
}
