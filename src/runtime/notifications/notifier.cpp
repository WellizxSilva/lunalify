#include "lunalify/lunalify.hpp"
#include "lunalify/api/types.hpp"
#include "lunalify/runtime/notifications/notifier.hpp"
#include <winrt/base.h>
#include <winrt/Windows.UI.Notifications.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Data.Xml.Dom.h>

using namespace winrt;
using namespace Windows::Data::Xml::Dom;
using namespace Windows::UI::Notifications;
using namespace Lunalify::API::Toast;
using namespace Lunalify::Utils;

namespace Lunalify::Runtime::Notifications::Notifier {

    std::wstring BuildToastXml(const ToastConfig& config) {
        std::wstring xml = L"<toast scenario=\"";
        xml += to_wstring(config.scenario.c_str());
        xml += L"\" duration=\"";
        xml += (config.duration == "long" ? L"long" : L"short");
        xml += L"\"";

        if (!config.tag.empty()) {
            xml += L" tag=\"" + to_wstring(config.tag.c_str()) + L"\"";
        }
        if (!config.group.empty()) {
            xml += L" group=\"" + to_wstring(config.group.c_str()) + L"\"";
        }
        xml += L" useButtonStyle=\"true\">";
        xml += L"<visual><binding template=\"ToastGeneric\">";
        xml += L"<text>" + to_wstring(config.title.c_str()) + L"</text>";
        xml += L"<text>" + to_wstring(config.message.c_str()) + L"</text>";


        if (config.progress.active) {
            xml += L"<progress ";
            // Data binding
            xml += L"title=\"{progressTitle}\" ";
            xml += L"value=\"{progressValue}\" ";
            xml += L"valueStringOverride=\"{progressValueString}\" ";
            xml += L"status=\"{progressStatus}\" ";
            xml += L"/>";
        }

        if (!config.iconPath.empty()) {
            xml += L"<image placement=\"appLogoOverride\" src=\"file:///" +
                   to_wstring(config.iconPath.c_str()) + L"\" hint-crop=\"circle\"/>";
        }
        if (!config.heroPath.empty()) {
            xml += L"<image placement=\"hero\" src=\"file:///" +
                   to_wstring(config.heroPath.c_str()) + L"\"/>";
        }

        xml += L"</binding></visual>";

        if (config.silent) {
            xml += L"<audio silent=\"true\"/>";
        } else {
            xml += L"<audio src=\"ms-winsoundevent:Notification." +
                   to_wstring(config.sound.c_str()) + L"\"/>";
        }

        if (!config.actions.empty()) {
            xml += L"<actions>";
            for (const auto& input : config.inputs) {
                xml += L"<input id=\"" + to_wstring(input.id.c_str()) + L"\" type=\"" + to_wstring(input.type.c_str()) + L"\" ";
                if (!input.placeholder.empty()) xml += L"placeHolderContent=\"" + to_wstring(input.placeholder.c_str()) + L"\" ";
                if (!input.title.empty()) xml += L"title=\"" + to_wstring(input.title.c_str()) + L"\" ";
                if (!input.defaultInput.empty()) xml += L"defaultInput=\"" + to_wstring(input.defaultInput.c_str()) + L"\" ";
                xml += L">";
                for (const auto& sel : input.selections) {
                    xml += L"<selection id=\"" + to_wstring(sel.id.c_str()) + L"\" content=\"" + to_wstring(sel.content.c_str()) + L"\"/>";
                }
                xml += L"</input>";
            }

            for (const auto& action : config.actions) {
                xml += L"<action content=\"" + to_wstring(action.label.c_str()) + L"\" ";
                xml += L"arguments=\"" + to_wstring(action.target.c_str()) + L"\" ";
                xml += L"activationType=\"" + to_wstring(action.type.c_str()) + L"\" ";
                if (!action.image.empty()) {
                    xml += L"imageUri=\"file:///" + to_wstring(action.image.c_str()) + L"\" ";
                }
                if (!action.style.empty()) {
                    std::wstring s = to_wstring(action.style.c_str());
                    if (s == L"success" || s == L"Success") s = L"Success";
                    else if (s == L"critical" || s == L"Critical") s = L"Critical";
                    xml += L" hint-buttonStyle=\"" + s + L"\"";
                }
                xml += L"/>";
            }
            xml += L"</actions>";
        }

        xml += L"</toast>";
        return xml;
    }

    Lunalify::Errors::Code Update(const std::string& appId, const std::string& tag, const std::string& title, const std::string& value, const std::string& vString, const std::string& status) {
        try {
            NotificationData data;
            LNF_DEBUG("Lunalify:Notifications:Notifier:Update", "Binding Keys: progressValue={}", value);

            data.Values().Insert(L"progressTitle", winrt::to_hstring(title));
            data.Values().Insert(L"progressValue", winrt::to_hstring(value));
            data.Values().Insert(L"progressValueString", winrt::to_hstring(vString));
            data.Values().Insert(L"progressStatus", winrt::to_hstring(status));
            data.SequenceNumber(0); // Allow Windows accepts overrides

            auto notifier = ToastNotificationManager::CreateToastNotifier(to_wstring(appId.c_str()));
            auto result = notifier.Update(data, winrt::to_hstring(tag));

            if (result == NotificationUpdateResult::Succeeded) return Lunalify::Errors::Code::SUCCESS;
            return Lunalify::Errors::Code::WINRT_REGISTRATION_MISSING;
        } catch (winrt::hresult_error const& ex) {
            LNF_ERROR("Lunalify:Notifications:Notifier:Update", "Exception: {}", winrt::to_string(ex.message()));
            return Lunalify::Errors::Code::UNKNOWN;
        }
    }

    Lunalify::Errors::Code Dispatch(const std::string& appId, const std::wstring& xmlContent, const std::string& uniqueId, const std::string& initialTitle, const std::string& initialVal, const std::string& initialDisplay, const std::string& initialStatus, InteractionCallback onEvent) {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        XmlDocument doc;
        try {
            doc.LoadXml(xmlContent);
        } catch (...) {
            LNF_ERROR("Lunalify:Notifications:Notifier:Dispatch", "XML Load failed for Id: {}", uniqueId);
            return Lunalify::Errors::Code::WINRT_INVALID_XML;
        }
        ToastNotification toast(doc);
        toast.Tag(winrt::to_hstring(uniqueId));
        NotificationData data;
        data.Values().Insert(L"progressTitle", winrt::to_hstring(initialTitle));
        data.Values().Insert(L"progressValue", winrt::to_hstring(initialVal));
        data.Values().Insert(L"progressValueString", winrt::to_hstring(initialDisplay));
        data.Values().Insert(L"progressStatus", winrt::to_hstring(initialStatus));
        data.SequenceNumber(0);
        toast.Data(data);

        toast.Activated([uniqueId, onEvent](auto const&, winrt::Windows::Foundation::IInspectable const& args) {
            auto activatedArgs = args.try_as<ToastActivatedEventArgs>();
            std::string actionArgs = "";
            if (activatedArgs) {
                actionArgs = winrt::to_string(activatedArgs.Arguments());
                for (auto const& pair : activatedArgs.UserInput()) {
                    actionArgs += "|" + winrt::to_string(pair.Key()) + "=" + winrt::to_string(winrt::unbox_value<winrt::hstring>(pair.Value()));
                }
            }
            LNF_INFO("Lunalify:Notifications:Notifier:Dispatch", "Interaction detected Id:{} Args: {}", uniqueId, actionArgs);
            onEvent(uniqueId, "activated", actionArgs);
        });

        toast.Dismissed([uniqueId, onEvent](auto const&, ToastDismissedEventArgs const& args) {
            std::string reason = "Unknown";
            switch (args.Reason()) {
                case ToastDismissalReason::UserCanceled: reason = "UserCanceled"; break;
                case ToastDismissalReason::TimedOut:     reason = "TimedOut"; break;
                case ToastDismissalReason::ApplicationHidden: reason = "AppHidden"; break;
            }
            LNF_INFO("Lunalify:Notifications:Notifier:Dispatch", "Toast dismissed, reason: {}", reason);
            onEvent(uniqueId, "dismissed", reason);
        });
        std::wstring wAppId = to_wstring(appId.c_str());
        ToastNotifier notifier = ToastNotificationManager::CreateToastNotifier(wAppId);
        Lunalify::Errors::Code errorCode = Lunalify::Errors::Code::SUCCESS;

        switch (notifier.Setting()) {
            case NotificationSetting::Enabled:
                LNF_DEBUG("Lunalify:Notifications:Notifier:Dispatch", "Showing toast for uniqueId: {}", uniqueId);
                notifier.Show(toast);
                return Lunalify::Errors::Code::SUCCESS;

            case NotificationSetting::DisabledForUser:
                LNF_WARN("Lunalify:Notifications:Notifier:Dispatch", "Disabled for user, toast blocked Id: {}", uniqueId);
                errorCode = Lunalify::Errors::Code::WINRT_DISBLED_BY_USER;
                break;

            case NotificationSetting::DisabledByGroupPolicy:
                LNF_WARN("Lunalify:Notifications:Notifier:Dispatch", "Disabled by group policy, toast blocked Id: {}", uniqueId);
                errorCode = Lunalify::Errors::Code::WINRT_DISABLED_BY_POLICY;
                break;

            case NotificationSetting::DisabledForApplication:
                LNF_WARN("Lunalify:Notifications:Notifier:Dispatch", "Disabled for application, toast blocked Id: {}", uniqueId);
                errorCode = Lunalify::Errors::Code::WINRT_REGISTRATION_MISSING;
                break;

            default:
                // Not certain what went wrong, return generic error
                LNF_ERROR("Lunalify:Notifications:Notifier:Dispatch", "Unknown Error, toast blocked Id: {}", uniqueId);
                errorCode = Lunalify::Errors::Code::UNKNOWN;
                break;
        }
        LNF_ERROR("Lunalify:Notifications:Notifier:Dispatch", "Toast dispatch failed for {}. Error Code: {}", uniqueId, static_cast<int>(errorCode));
        onEvent(uniqueId, "failed", std::to_string(static_cast<int>(errorCode)));
        return Lunalify::Errors::Code::UNKNOWN;
    }
}
