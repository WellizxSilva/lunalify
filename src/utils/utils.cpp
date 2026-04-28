#include "lunalify/utils/utils.hpp"
#include <string>
#include <windows.h>


std::wstring Lunalify::Utils::to_wstring(const char* str) {
    if (!str) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str, -1, &wstr[0], len);
    // Removes the null terminator extra
    if (!wstr.empty() && wstr.back() == L'\0') {
        wstr.pop_back();
    }
    return wstr;
}

std::wstring Lunalify::Utils::to_wstring(const std::wstring& str) {
    return str.empty() ? L"" : std::wstring(str.begin(), str.end());
}

std::wstring Lunalify::Utils::to_wstring(const std::string& str) {
    return to_wstring(str.c_str());
};

std::wstring  Lunalify::Utils::get_current_exe_path() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return std::wstring(buffer);
}
