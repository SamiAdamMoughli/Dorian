#pragma once
#include <windows.h>
#include <vector>
#include <string>

class FileManager
{
public:
    // Writes payload to filePath, creates a SEC_IMAGE section, then overwrites
    // the file with decoy content. The section retains a reference to the
    // original PE mapping even after the file no longer matches it.
    static HANDLE WriteAndObscure(const std::wstring &filePath,
                                   const std::vector<uint8_t> &payload,
                                   const std::vector<uint8_t> &decoy);
};
