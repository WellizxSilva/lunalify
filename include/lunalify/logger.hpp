#pragma once
#include <format>
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <windows.h>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace Lunalify {

enum class LogLevel {
    LEVEL_DEBUG = 0,
    LEVEL_INFO = 1,
    LEVEL_WARN = 2,
    LEVEL_ERROR = 3
};

class Logger {
private:
    static inline bool _enabled = false;
    static inline bool _consoleOutput = true;
    static inline LogLevel _minLevel = LogLevel::LEVEL_INFO;
    static inline std::string _logPath = "lunalify.log";
    static inline std::mutex _logMutex;

    static std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
           << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    static std::string LevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::LEVEL_DEBUG: return "DEBUG";
            case LogLevel::LEVEL_INFO:  return "INFO";
            case LogLevel::LEVEL_WARN:  return "WARN";
            case LogLevel::LEVEL_ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

public:
    static void Initialize(bool enabled, LogLevel level, const std::string& path, bool consoleOutput = true)    {
        _enabled = enabled;
        _minLevel = level;
        _logPath = path;
        _consoleOutput = consoleOutput;

        if (_enabled && !_logPath.empty()) {
            try {
                std::filesystem::path p(_logPath);
                if (p.has_parent_path()) {
                    std::filesystem::create_directories(p.parent_path());
                }
            } catch (...) {
            }
        }
    }

    static void Log(LogLevel level, const std::string& component, const std::string& message) {
        if (!_enabled || level < _minLevel) return;

        std::lock_guard<std::mutex> lock(_logMutex);
        std::ofstream file(_logPath, std::ios::app);
        std::string formatted = std::format("[{}] [{}] [{}] [{}] {}\n",
                                                        GetTimestamp(),
                                                        LevelToString(level),
                                                        GetCurrentProcessId(),
                                                        component,
                                                        message);

        if (_consoleOutput) {
            std::cout << formatted << std::flush;
        }

        if (!_logPath.empty()) {
            std::ofstream file(_logPath, std::ios::app);
            if (file.is_open()) {
                file << formatted;
                file.close();
            }
        }
    }
};

}

// Macros
#define LNF_DEBUG(comp, fmt, ...) Lunalify::Logger::Log(Lunalify::LogLevel::LEVEL_DEBUG, comp, std::format(fmt, ##__VA_ARGS__))
#define LNF_INFO(comp, fmt, ...)  Lunalify::Logger::Log(Lunalify::LogLevel::LEVEL_INFO,  comp, std::format(fmt, ##__VA_ARGS__))
#define LNF_WARN(comp, fmt, ...)  Lunalify::Logger::Log(Lunalify::LogLevel::LEVEL_WARN,  comp, std::format(fmt, ##__VA_ARGS__))
#define LNF_ERROR(comp, fmt, ...) Lunalify::Logger::Log(Lunalify::LogLevel::LEVEL_ERROR, comp, std::format(fmt, ##__VA_ARGS__))
