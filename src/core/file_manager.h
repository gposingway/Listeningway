#pragma once
#include <string>

class FileManager {
public:
    // Returns the directory path where config/log files should be stored
    static std::string GetPath();
};
