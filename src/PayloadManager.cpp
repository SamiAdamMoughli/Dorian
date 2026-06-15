#include "PayloadManager.hpp"
#include <iostream>
#include <fstream>
#include <windows.h>

std::vector<uint8_t> PayloadManager::LoadPayload(const std::string &filePath)
{
    std::vector<uint8_t> buffer;
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);

    if (!file.is_open())
    {
        std::cerr << "[-] Failed to open file: " << filePath << std::endl;
        return buffer;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    buffer.resize(static_cast<size_t>(size));

    if (!file.read(reinterpret_cast<char *>(buffer.data()), size))
    {
        std::cerr << "[-] Failed to read file: " << filePath << std::endl;
        buffer.clear();
    }

    return buffer;
}

bool PayloadManager::ValidatePE(const std::vector<uint8_t> &buffer)
{
    if (buffer.size() < sizeof(IMAGE_DOS_HEADER))
        return false;

    const auto *dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER *>(buffer.data());
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        std::cerr << "[-] Invalid DOS signature." << std::endl;
        return false;
    }

    uint32_t peOffset = dosHeader->e_lfanew;
    if (peOffset + sizeof(IMAGE_NT_HEADERS32) > buffer.size())
    {
        std::cerr << "[-] PE header out of bounds." << std::endl;
        return false;
    }

    const auto *ntHeaders32 = reinterpret_cast<const IMAGE_NT_HEADERS32 *>(buffer.data() + peOffset);
    if (ntHeaders32->Signature != IMAGE_NT_SIGNATURE)
    {
        std::cerr << "[-] Invalid PE signature." << std::endl;
        return false;
    }

    WORD magic = ntHeaders32->OptionalHeader.Magic;
    if (magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC && magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        std::cerr << "[-] Unknown optional header magic: 0x" << std::hex << magic << std::dec << std::endl;
        return false;
    }

    return true;
}

uint32_t PayloadManager::GetEntryPoint(const std::vector<uint8_t> &buffer)
{
    if (buffer.empty())
        return 0;

    const auto *dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER *>(buffer.data());
    uint32_t peOffset = dosHeader->e_lfanew;

    if (peOffset + sizeof(IMAGE_NT_HEADERS32) > buffer.size())
        return 0;

    const auto *ntHeaders32 = reinterpret_cast<const IMAGE_NT_HEADERS32 *>(buffer.data() + peOffset);

    if (ntHeaders32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        if (peOffset + sizeof(IMAGE_NT_HEADERS64) > buffer.size())
            return 0;
        const auto *ntHeaders64 = reinterpret_cast<const IMAGE_NT_HEADERS64 *>(buffer.data() + peOffset);
        return ntHeaders64->OptionalHeader.AddressOfEntryPoint;
    }

    return ntHeaders32->OptionalHeader.AddressOfEntryPoint;
}
