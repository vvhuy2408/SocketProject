#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>



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
void handle_put(FtpSession& session, const std::string& localFile, const std::string& remoteFile); // Upload 1 file (with virus scan)
void handle_mput(FtpSession& session, const std::vector<std::string>& files); // Upload multiple files (with virus scan)
void handle_get(FtpSession& session, const std::string& remoteFile, const std::string& localFile); // Download 1 file
void handle_mget(FtpSession& session, const std::vector<std::string>& files); // Download multiple files
//  Prompt & Utility
extern bool prompt_enabled; // extern -> do co the define o nhieu .cpp khac nhau
bool confirm_prompt(const std::string& filename);
void handle_prompt(const std::string& arg);// Enable/disable confirmation prompt
bool sendAll(SOCKET s, const char* buf, int64_t len);
//  Check file with ClamAV
bool scan_file_with_clamav(const std::string& filepath);

// 3. Session management
//ascii,binary
void handle_status(const FtpSession& session); //Showư current session status
//passive

void handle_open(FtpSession& session, const std::string& ip); //Connect to the FTP server
void handle_close(FtpSession& session); //Disconnect to the FTP server

void handle_quit(FtpSession& session); //Exit the FTP client
void hande_bye(FtpSession& session); //Exit the FTP client

void handle_help(); //Show help text for commands
