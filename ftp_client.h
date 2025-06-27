#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>
#include <string>

// Cấu trúc lưu trạng thái phiên FTP
struct FtpSession {
    SOCKET controlSocket;  // Socket kết nối với server
    bool connected;        // Trạng thái kết nối

    FtpSession() : controlSocket(INVALID_SOCKET), connected(false) {}
};

// 1. File and Directory Operations
void handle_ls(FtpSession& session); //List files and folders on the FTP server
void handle_cd(FtpSession& session, const std::string& path); //Change directory (on server or local)
void handle_pwd(FtpSession& session); //Show the current directory on the server
void handle_mkdir(FtpSession& session, const std::string& folder); //Create folders on the FTP server
void handle_rmdir(FtpSession& session, const std::string& folder); //Delete folders on the FTP server
void handle_delete(FtpSession& session, const std::string& filename); //Delete a file on the FTP server
void handle_rename(FtpSession& session, const std::string& oldname, const std::string& newname); //Rename a file on the FTP server

// 2. Upload and download

// 3. Session management
//ascii,binary
void handle_status(const FtpSession& session); //Show current session status
//passive

void handle_open(FtpSession& session, const std::string& ip); //Connect to the FTP server
void handle_close(FtpSession& session); //Disconnect to the FTP server

void handle_quit(FtpSession& session); //Exit the FTP client
void hande_bye(FtpSession& session); //Exit the FTP client

void handle_help(); //Show help text for commands
