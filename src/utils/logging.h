#pragma once
#include <string>
#include <mutex>

enum class LogLevel {
    Debug,
    Error
};

// Thread-safe logging to file with timestamps
void LogToFile(const std::string& message, LogLevel level = LogLevel::Debug);

// Logging macros
#define LOG_DEBUG(msg) LogToFile(msg, LogLevel::Debug)
#define LOG_ERROR(msg) LogToFile(msg, LogLevel::Error)
#define LOG_WARNING(msg) LogToFile(msg, LogLevel::Debug)
#define LOG_INFO(msg) LogToFile(msg, LogLevel::Debug)

// Log file management
void OpenLogFile(const std::string& filename);
void CloseLogFile();
/**
 * @brief Global mutex for log file access (for advanced use).
 */
extern std::mutex g_log_mutex;
