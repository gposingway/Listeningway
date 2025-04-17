#pragma once
#include <string>
#include <mutex>

void LogToFile(const std::string& message);
void OpenLogFile(const std::string& filename);
void CloseLogFile();
extern std::mutex g_log_mutex;
