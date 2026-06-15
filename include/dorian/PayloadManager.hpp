#pragma once
#include <windows.h>
#include <vector>
#include <string>

class PayloadManager
{
public:
    static std::vector<uint8_t> LoadPayload(const std::string &filePath);
    static bool ValidatePE(const std::vector<uint8_t> &buffer);
    static uint32_t GetEntryPoint(const std::vector<uint8_t> &buffer);
};
