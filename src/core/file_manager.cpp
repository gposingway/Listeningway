#include "file_manager.h"
#include <windows.h>
#include <string>

std::string FileManager::GetPath() {
    char modulePath[MAX_PATH] = {};
    HMODULE hModule = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&GetPath, &hModule);
    GetModuleFileNameA(hModule, modulePath, MAX_PATH);
    std::string path(modulePath);
    size_t pos = path.find_last_of("\\/");
    std::string dir = (pos != std::string::npos) ? path.substr(0, pos + 1) : "";
    return dir;
}
