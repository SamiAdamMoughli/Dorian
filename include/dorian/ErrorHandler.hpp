#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "Common.hpp"

class ErrorHandler
{
public:
    static void LogWin32Error(const std::string &operation);
    static void LogNTStatus(const std::string &operation, NTSTATUS status);
    static void SafeCloseHandle(HANDLE &handle);
    static void CleanupHandles(std::vector<HANDLE *> handles);
};
