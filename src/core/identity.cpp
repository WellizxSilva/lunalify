#include <windows.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <propvarutil.h>
#include <propkey.h>
#include <string>
#include <cstdio>
#include <winrt/Windows.Foundation.h>
#include "../../include/lunalify/lunalify.hpp"

Lunalify::Errors::Code create_app_shortcut(const std::wstring& appId, const std::wstring& exePath, const std::wstring& shortcutPath, const std::wstring& iconPath) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return Lunalify::Errors::Code::WINRT_INIT_FAILED;

    IShellLinkW* pShellLink = nullptr;
    hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&pShellLink);

    if (SUCCEEDED(hr)) {
        pShellLink->SetPath(exePath.c_str());
        if (!iconPath.empty()) pShellLink->SetIconLocation(iconPath.c_str(), 0);
        pShellLink->SetDescription(L"Lunalify.Native.Framework");

        // Defines the AppUserModelID
        IPropertyStore* pPropStore = nullptr;
        hr = pShellLink->QueryInterface(IID_IPropertyStore, (void**)&pPropStore);
        if (SUCCEEDED(hr)) {
            PROPVARIANT var;
            hr = InitPropVariantFromString(appId.c_str(), &var);
            if (SUCCEEDED(hr)) {
                hr = pPropStore->SetValue(PKEY_AppUserModel_ID, var);
                if (SUCCEEDED(hr)) {
                    hr = pPropStore->Commit();
                }
                PropVariantClear(&var);
            }
            pPropStore->Release();
        }

        // Saves the shortcut to disk with the properties already injected
        IPersistFile* pPersistFile = nullptr;
        hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
        if (SUCCEEDED(hr)) {
            hr = pPersistFile->Save(shortcutPath.c_str(), TRUE);
            pPersistFile->Release();
        }

        pShellLink->Release();
    }

    CoUninitialize();
    return SUCCEEDED(hr) ? Lunalify::Errors::Code::SUCCESS : Lunalify::Errors::Code::SYS_SHORTCUT_FAILED;
}

Lunalify::Errors::Code register_app_identity(const std::wstring& wAppId, const std::wstring& wAppName) {
    HKEY hKey;
    LSTATUS status;
    std::wstring subkey = L"Software\\Classes\\AppUserModelId\\" + wAppId;
    std::wstring capKeyPath = subkey + L"\\Capabilities";

    if (RegCreateKeyExW(HKEY_CURRENT_USER, capKeyPath.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"ApplicationName", 0, REG_SZ, (const BYTE*)wAppName.c_str(), (DWORD)(wAppName.size() + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }

    status = RegCreateKeyExW(HKEY_CURRENT_USER, subkey.c_str(), 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

    if (status == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"DisplayName", 0, REG_SZ, (const BYTE*)wAppName.c_str(), (DWORD)(wAppName.size() + 1) * sizeof(wchar_t));
        // RegSetValueExW(hKey, "IconUri", 0, REG_SZ, (const BYTE*)iconPath, ...);
        RegCloseKey(hKey);
    } else {
        return Lunalify::Errors::Code::SYS_ACCESS_DENIED;
    }

    // Registers in CurrentVersion\AppUserModelId (Essential for WinRT to index quickly)
    std::wstring appModelPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\AppUserModelId\\" + wAppId;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, appModelPath.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"DisplayName", 0, REG_SZ, (const BYTE*)wAppName.c_str(), (DWORD)(wAppName.size() + 1) * sizeof(wchar_t));
        RegCloseKey(hKey);
    }

    std::wstring notifyKeyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\" + wAppId;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, notifyKeyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD enabled = 1;
        RegSetValueExW(hKey, L"ShowInActionCenter", 0, REG_DWORD, (const BYTE*)&enabled, sizeof(DWORD));
        RegCloseKey(hKey);
    }

    return Lunalify::Errors::Code::SUCCESS;
}


Lunalify::Errors::Code unregister_app_identity(const std::wstring& wAppId, const std::wstring& shortcutPath) {
    if (std::filesystem::exists(shortcutPath)) {
        if (!std::filesystem::remove(shortcutPath)) {
            LNF_ERROR("App:Unregister", "Failed to remove shortcut file");
        }
    }

    // RegDeleteTreeW removes the key and all subkeys (like \Capabilities)
    std::wstring keys[] = {
        L"Software\\Classes\\AppUserModelId\\" + wAppId,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\AppUserModelId\\" + wAppId,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\" + wAppId
    };

    for (const auto& keyPath : keys) {
        LSTATUS status = RegDeleteTreeW(HKEY_CURRENT_USER, keyPath.c_str());
        if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND) {
            LNF_WARN("App:Unregister", "Could not delete registry key: {}", winrt::to_string(keyPath));
        }
    }

    return Lunalify::Errors::Code::SUCCESS;
}
