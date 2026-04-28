#pragma once
#include <string>
#include <windows.h>

namespace Lunalify::Utils {
    std::wstring to_wstring(const char* str);
    std::wstring to_wstring(const std::wstring& str);
    std::wstring to_wstring(const std::string& str);
    std::wstring get_current_exe_path();
}
