# FTP Client + ClamAV Virus Scanning

This project comprises two standalone programs:

1. **ClamAV Agent** (`clamav_agent.exe`):  
   - Listens on TCP port **21**  
   - Receives files from the FTP client  
   - Runs ClamAV (`clamscan.exe`) to scan for viruses  
   - Returns `OK` or `INFECTED`  
   - Logs activity to `agent.log`

2. **FTP Client** (`ftp_client.exe`):  
   - Implements core FTP commands (`open`, `close`, `ls`, `cd`, `mkdir`, `rmdir`, `delete`, `rename`, `put`, `get`, …)  
   - On `put`, first sends file to ClamAV Agent for scanning  
   - If scan result is `OK`, uploads to FTP Server; if `INFECTED`, aborts

---

## 1. Instructions to Run

### Build

```
# In MSYS2/MinGW or CMD with g++
g++ -std=c++17 -o clamav_agent clamav_agent.cpp -lws2_32
g++ -std=c++17 -o ftp_client ftp_client.cpp -lws2_32

```

### Run
1. Start ClamAV Agent
```
./clamav_agent.exe
```
2. Start FTP Client
```
./ftp_client.exe
```

## 2. FTP Server Software Used & Setup

### FTP Server Software

- **FileZilla Server** (free, open-source)
- Download: https://filezilla-project.org/download.php?type=server
- Tested version: FileZilla Server 1.7.x

---

### Setup Steps

1. **Install FileZilla Server**
   - Run installer and complete setup (keep default port 14147 for admin interface)
   - Allow through Windows Firewall if prompted

2. **Create FTP User:**
   - Open FileZilla Server Interface
   - Go to `Edit → Users`
   - Click `Add` → create username (e.g., `test`)
   - Set password (e.g., `test`)

3. **Set Shared Folder:**
   - Still in `Users → Shared folders`
   - Click `Add` under folders → choose a directory (e.g., `C:\FTP\Root`)
   - Check `Read`, `Write`, `Delete`, `Append` permissions

4. **Enable Passive Mode (for data transfers):**
   - Go to `Edit → Settings → Passive Mode Settings`
   - Choose: **Use custom port range** → `49152–65534`
   - Select: **Use the following IP** → Enter local IP (e.g. `192.168.1.10`)
     - Find IP using: `ipconfig`

5. **Firewall Rules:**
   - Ensure ports **21**, **49152–65534** are allowed in Windows Defender Firewall:
     - Inbound rules → add rule for TCP, allow ports 21 and 49152–65534
   - Also allow `FileZilla Server.exe` to communicate on private networks

6. **Start Server**
   - Click `Start Server` if not running already
   - Server will now listen on **port 21**

---

### Quick Test with Client

From your `ftp_client.exe`:
```text
ftp> open 127.0.0.1
ftp> put clean.txt clean.txt
```

## 3. ClamAV Installation and Configuration

### Software

- ClamAV for Windows
- Download from: https://www.clamav.net/downloads
- Recommended install path: `C:\ClamAV`

---

### Setup Steps

1. **Install ClamAV**
   - Run the installer
   - Use the default install path: `C:\ClamAV`
   - Allow any firewall or security prompts

2. **Update the virus definition database**
   - Open Command Prompt (as Administrator)
   - Run the following command:
     ```
     "C:\ClamAV\freshclam.exe"
     ```

3. **Verify installation**
   - Check if ClamAV is working:
     ```
     "C:\ClamAV\clamscan.exe" --version
     ```

4. **Test scanning manually (optional)**
   - Use a test file (e.g., `eicar.txt`) and scan it:
     ```
     "C:\ClamAV\clamscan.exe" --no-summary "eicar.txt"
     ```

---

## 4. FTP Client Usage
1. Connect to FTP server
```
ftp> open 127.0.0.1
[Client] Connected to 127.0.0.1
[Server] 220-FileZilla Server 1.10.3
220 Please visit https://filezilla-project.org/
ftp> user test1
[Server] 331 Please, specify the password.
ftp> pass 123456
[Server] 230 Login successful.
```

2. Upload file to FTP server
```
ftp> put safe.txt safe.txt
[Server] 227 Entering Passive Mode (127,0,0,1,217,70)
[Server] 150 About to start data transfer.
[Server] 226 Operation successful
[DEBUG] Done handle_put()
```

FileZilla log:
```
STOR safe.txt
150 About to start data transfer.
226 Operation successful
```

3. Run ClamAV scan before upload file. (Expected output after upload)
```
C:\Users\Admin\source\repos\SocketProject\clamav_agent.exe
C:\Users\Admin\source\repos\SocketProject\temp\safe.txt: OK
```

4. Upload multiple files
```
Transfer file "safe1.txt"? (y/n): y
[Server] 227 Entering Passive Mode (127,0,0,1,213,206)
[Server] 150 About to start data transfer.
[Server] 226 Operation successful
[DEBUG] Done handle_put()

Transfer file "safe2.txt"? (y/n): y
[Server] 227 Entering Passive Mode (127,0,0,1,247,6)
[Server] 150 About to start data transfer.
[Server] 226 Operation successful
[DEBUG] Done handle_put()
```

ClamAV.exe:
```
C:\Users\Admin\source\repos\SocketProject\clamav_agent.exe
C:\Users\Admin\source\repos\SocketProject\temp\safe1.txt: OK
C:\Users\Admin\source\repos\SocketProject\temp\safe2.txt: OK
```
5. List files on server
```
ftp> ls
[Server] 227 Entering Passive Mode (127,0,0,1,201,123)
[Server] 150 Starting data transfer.
-rw-rw-rw- 1 ftp ftp 5 Jul 05 10:10 safe.txt
-rw-rw-rw- 1 ftp ftp 5 Jul 05 13:43 safe1.txt
-rw-rw-rw- 1 ftp ftp 5 Jul 05 13:44 safe2.txt
[Server] 226 Operation successful
```

6. Create folder on server
```
ftp> mkdir folder1
[Server] 257 "/folder1" created successfully.
```

7. Change to new directory
```
ftp> cd folder1
[Server] 250 CWD command successful
```

8. Check current directory
```
ftp> pwd
[Server] 257 "/folder1" is current directory.
```
---
### Notes

- You do not need to run ClamAV as a background service.
- The agent program (`clamav_agent.exe`) uses `system()` to call `clamscan.exe` directly.
- Ensure paths (like filenames) passed to ClamAV are wrapped in quotes if they contain spaces.

---

### File and Folder Usage

- Files are saved temporarily in the `temp/` folder before scanning.
- If a file is infected, it is moved to the `quarantine/` folder.
- Logs (including scan results) are written to the `agent.log` file.

---



