#pragma once
#include <windows.h>
#include <string>
#include "Common.hpp"

class ProcessExecutor
{
public:
    static HANDLE CreateProcessFromSection(HANDLE sectionHandle);
    static bool SetupProcessParameters(HANDLE processHandle, const std::wstring &imagePath);
    static HANDLE CreateProcessThread(HANDLE processHandle, uint32_t entryPointRVA);
    static bool ExecutePayload(HANDLE threadHandle);
    static bool Run(HANDLE sectionHandle, uint32_t entryPointRVA, const std::wstring &imagePath);
};
