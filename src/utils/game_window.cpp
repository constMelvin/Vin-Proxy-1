#include "game_window.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace utils::game_window {

#ifdef _WIN32
namespace {

// File name of the exe that owns the window, lower case ("growtopia.exe")
std::string process_name(DWORD pid) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return {};
    char path[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    std::string name;
    if (QueryFullProcessImageNameA(process, 0, path, &size)) {
        name = path;
        const size_t slash = name.find_last_of("\\/");
        if (slash != std::string::npos) name = name.substr(slash + 1);
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }
    CloseHandle(process);
    return name;
}

// Visible top-level windows of Growtopia.exe
std::vector<HWND> find_windows() {
    std::vector<HWND> windows;
    EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
        if (!IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER) != nullptr) return TRUE;
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (process_name(pid) == "growtopia.exe")
            reinterpret_cast<std::vector<HWND>*>(lp)->push_back(hwnd);
        return TRUE;
    }, reinterpret_cast<LPARAM>(&windows));
    return windows;
}

} // namespace
#endif

bool press_back() {
#ifdef _WIN32
    const auto windows = find_windows();
    if (windows.empty()) {
        spdlog::warn("[GameWindow] Growtopia window not found, can't press Back");
        return false;
    }
    const UINT scan = MapVirtualKeyA(VK_ESCAPE, MAPVK_VK_TO_VSC);
    const LPARAM down = 1 | (static_cast<LPARAM>(scan) << 16);
    const LPARAM up = down | (1LL << 30) | (1LL << 31);
    for (HWND hwnd : windows) {
        PostMessageA(hwnd, WM_KEYDOWN, VK_ESCAPE, down);
        PostMessageA(hwnd, WM_KEYUP, VK_ESCAPE, up);
    }
    spdlog::info("[GameWindow] Pressed Back (Esc) in the Growtopia window");
    return true;
#else
    return false;
#endif
}

}
