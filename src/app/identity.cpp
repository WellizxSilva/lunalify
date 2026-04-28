#include "lunalify/app/identity.hpp"
#include <windows.h>
#include <shobjidl.h>
#include <propvarutil.h>
#include <propkey.h>
#include <filesystem>

using namespace Lunalify::App;

Lunalify::Errors::Code Identity::CreateAppShortcut(const std::wstring& wAppName,const std::wstring& exePath, const std::wstring& shortcutPath, const std::wstring& wIconPath) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return Errors::Code::WINRT_INIT_FAILED;

    IShellLinkW* pShellLink = nullptr;
    hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&pShellLink);

    if (SUCCEEDED(hr)) {
        pShellLink->SetPath(exePath.c_str());
        if (!wIconPath.empty()) pShellLink->SetIconLocation(wIconPath.c_str(), 0);
        pShellLink->SetDescription(L"Lunalify Native Framework");

        IPropertyStore* pPropStore = nullptr;
        hr = pShellLink->QueryInterface(IID_IPropertyStore, (void**)&pPropStore);
        if (SUCCEEDED(hr)) {
            PROPVARIANT var;
            hr = InitPropVariantFromString(wAppName.c_str(), &var);
            if (SUCCEEDED(hr)) {
                hr = pPropStore->SetValue(PKEY_AppUserModel_ID, var);
                if (SUCCEEDED(hr)) hr = pPropStore->Commit();
                PropVariantClear(&var);
            }
            pPropStore->Release();
        }

        IPersistFile* pPersistFile = nullptr;
        hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
        if (SUCCEEDED(hr)) {
            hr = pPersistFile->Save(shortcutPath.c_str(), TRUE);
            pPersistFile->Release();
        }
        pShellLink->Release();
    }

    CoUninitialize();
    return SUCCEEDED(hr) ? Errors::Code::SUCCESS : Errors::Code::SYS_SHORTCUT_FAILED;
}


Lunalify::Errors::Code Identity::RegisterAppIdentity(const std::wstring& wAppId, const std::wstring& wAppName) {
    auto createKey = [&](const std::wstring& path, const std::wstring& valueName, const std::wstring& data) {
        HKEY hKey;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ, (const BYTE*)data.c_str(), (DWORD)(data.size() + 1) * sizeof(wchar_t));
            RegCloseKey(hKey);
            return true;
        }
        return false;
    };

    bool ok = true;
    ok &= createKey(L"Software\\Classes\\AppUserModelId\\" + wAppId + L"\\Capabilities", L"ApplicationName", wAppName);
    ok &= createKey(L"Software\\Classes\\AppUserModelId\\" + wAppId, L"DisplayName", wAppName);
    ok &= createKey(L"Software\\Microsoft\\Windows\\CurrentVersion\\AppUserModelId\\" + wAppId, L"DisplayName", wAppName);

    HKEY hKey;
    std::wstring notifyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\" + wAppId;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, notifyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD enabled = 1;
        RegSetValueExW(hKey, L"ShowInActionCenter", 0, REG_DWORD, (const BYTE*)&enabled, sizeof(DWORD));
        RegCloseKey(hKey);
    }

    return ok ? Errors::Code::SUCCESS : Errors::Code::SYS_ACCESS_DENIED;
}

Lunalify::Errors::Code Identity::UnregisterAppIdentity(const std::wstring& wAppId, const std::wstring& shortcutPath) {
    if (std::filesystem::exists(shortcutPath)) {
        std::filesystem::remove(shortcutPath);
    }

    std::wstring keys[] = {
        L"Software\\Classes\\AppUserModelId\\" + wAppId,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\AppUserModelId\\" + wAppId,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\" + wAppId
    };

    for (const auto& key : keys) {
        RegDeleteTreeW(HKEY_CURRENT_USER, key.c_str());
    }

    return Errors::Code::SUCCESS;
}
