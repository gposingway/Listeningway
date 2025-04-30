// ---------------------------------------------
// Logging Module
// Thread-safe logging to a file for debugging and diagnostics
// ---------------------------------------------
#pragma once
#include <string>
#include <mutex>

/**
 * @brief Log levels for Listeningway logging.
 */
enum class LogLevel {
    Debug,
    Error
};

/**
 * @brief Writes a message to the log file (thread-safe, timestamped, with log level).
 * @param message The message to log.
 * @param level The log level (debug or error).
 */
void LogToFile(const std::string& message, LogLevel level = LogLevel::Debug);

/**
 * @brief Macro for logging debug messages.
 */
#define LOG_DEBUG(msg) LogToFile(msg, LogLevel::Debug)
/**
 * @brief Macro for logging error messages.
 */
#define LOG_ERROR(msg) LogToFile(msg, LogLevel::Error)

/**
 * @brief Opens the log file for writing (call at startup).
 * @param filename The log file name.
 */
void OpenLogFile(const std::string& filename);
/**
 * @brief Closes the log file (call at shutdown).
 */
void CloseLogFile();
/**
 * @brief Global mutex for log file access (for advanced use).
 */
extern std::mutex g_log_mutex;
