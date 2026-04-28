#pragma once
#include "lunalify/lunalify.hpp"
#include "lunalify/api/types.hpp"
#include <string>
#include <functional>

using namespace Lunalify::API::Toast;

namespace Lunalify::Runtime::Notifications::Notifier {
    using InteractionCallback = std::function<void(std::string id, std::string event, std::string args)>;
    std::wstring BuildToastXml(const ToastConfig& config);
    Errors::Code Update(const std::string& appId, const std::string& tag, const std::string& title, const std::string& value, const std::string& vString, const std::string& status);
    Errors::Code Dispatch(const std::string& appId, const std::wstring& xmlContent, const std::string& uniqueId, const std::string& initialTitle, const std::string& initialVal, const std::string& initialDisplay, const std::string& initialStatus, InteractionCallback onEvent);
}
