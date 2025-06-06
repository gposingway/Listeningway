// ---------------------------------------------
// Logging Module Implementation
// Thread-safe logging to a file for debugging and diagnostics
// ---------------------------------------------
#include "logging.h"
#include "settings.h"
#include "../core/thread_safety_manager.h"
#include <fstream>
#include <chrono>
#include <ctime>

static std::ofstream g_log_file;
extern bool g_listeningway_debug_enabled;

static std::string GetLogFilePath() {
    std::string ini = GetSettingsPath();
    size_t pos = ini.find_last_of("\\/");
    std::string dir = (pos != std::string::npos) ? ini.substr(0, pos + 1) : "";
    return dir + "listeningway.log";
}

// Writes a timestamped message to the log file (thread-safe)
void LogToFile(const std::string& message, LogLevel level) {
    // Always log errors, only log debug if enabled
    if (level == LogLevel::Debug && !g_listeningway_debug_enabled) return;
    LOCK_LOGGING();
    if (g_log_file.is_open()) {
        // Get current time for timestamp
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
        localtime_s(&now_tm, &now_c);
        char time_str[100];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &now_tm);
        const char* level_str = (level == LogLevel::Error) ? "ERROR" : "DEBUG";
        g_log_file << "[" << time_str << "] [" << level_str << "] " << message << std::endl;
        g_log_file.flush(); // Ensure log is written immediately
    }
}

// Opens the log file for writing (call at startup)
void OpenLogFile(const char* /*filename*/) {
    LOCK_LOGGING();
    if (g_log_file.is_open()) return;
    g_log_file.open(GetLogFilePath(), std::ios::out | std::ios::app);
}

void OpenLogFile(const std::string& filename) {
    OpenLogFile(filename.c_str());
}

// Closes the log file (call at shutdown)
void CloseLogFile() {
    LOCK_LOGGING();
    if (g_log_file.is_open()) g_log_file.close();
}
