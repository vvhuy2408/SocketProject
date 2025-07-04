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
g++ -std=c++17 -o clamav_agent.exe clamav_agent.cpp -lws2_32
g++ -std=c++17 -o ftp_client.exe ftp_client_main.cpp ftp_client.cpp -lws2_32
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
   - Choose: **Use custom port range** → `50000–51000`
   - Select: **Use the following IP** → Enter local IP (e.g. `192.168.1.10`)
     - Find IP using: `ipconfig`

5. **Firewall Rules:**
   - Ensure ports **21**, **50000–51000** are allowed in Windows Defender Firewall:
     - Inbound rules → add rule for TCP, allow ports 21 and 50000–51000
   - Also allow `FileZilla Server.exe` to communicate on private networks

6. **Start Server**
   - Click `Start Server` if not running already
   - Server will now listen on **port 21**

---

### Quick Test with Client

From your `ftp_client.exe`:
```text
ftp> open 127.0.0.1
ftp> put clean.txt
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



