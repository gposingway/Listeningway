// ---------------------------------------------
// Logging Module Implementation
// Thread-safe logging to a file for debugging and diagnostics
// ---------------------------------------------
#include "logging.h"
#include <fstream>
#include <mutex>
#include <chrono>
#include <ctime>

static std::ofstream g_log_file;
std::mutex g_log_mutex;

// Writes a timestamped message to the log file (thread-safe)
void LogToFile(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    if (g_log_file.is_open()) {
        // Get current time for timestamp
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
        localtime_s(&now_tm, &now_c);
        char time_str[100];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &now_tm);
        g_log_file << "[" << time_str << "] " << message << std::endl;
        g_log_file.flush(); // Ensure log is written immediately
    }
}

// Opens the log file for writing (call at startup)
void OpenLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    if (g_log_file.is_open()) g_log_file.close();
    g_log_file.open(filename, std::ios::out | std::ios::app);
}

// Closes the log file (call at shutdown)
void CloseLogFile() {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    if (g_log_file.is_open()) g_log_file.close();
}
