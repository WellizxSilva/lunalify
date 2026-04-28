#include "../errors.hpp"

namespace Lunalify::App::Identity {
    Lunalify::Errors::Code RegisterAppIdentity(const std::wstring& wAppId, const std::wstring& wAppName);
    Lunalify::Errors::Code UnregisterAppIdentity(const std::wstring& wAppId, const std::wstring& shortcutPath);
    Lunalify::Errors::Code CreateAppShortcut(const std::wstring& appId,const std::wstring& exePath, const std::wstring& shortcutPath, const std::wstring& iconPath);
}
