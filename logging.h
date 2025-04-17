// ---------------------------------------------
// Logging Module
// Thread-safe logging to a file for debugging and diagnostics
// ---------------------------------------------
#pragma once
#include <string>
#include <mutex>

// Writes a message to the log file (thread-safe, timestamped)
void LogToFile(const std::string& message);
// Opens the log file for writing (call at startup)
void OpenLogFile(const std::string& filename);
// Closes the log file (call at shutdown)
void CloseLogFile();
// Global mutex for log file access (for advanced use)
extern std::mutex g_log_mutex;
