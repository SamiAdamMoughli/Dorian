#include "SectionManager.hpp"
#include <iostream>

HANDLE SectionManager::CreateImageSection(HANDLE fileHandle)
{
    if (fileHandle == INVALID_HANDLE_VALUE || fileHandle == NULL)
    {
        std::cerr << "[-] Invalid file handle provided to SectionManager." << std::endl;
        return NULL;
    }

    HANDLE hSection = NULL;
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, NULL, 0, NULL, NULL);

    NTSTATUS status = DynamicNT::Instance().NtCreateSection(
        &hSection,
        SECTION_ALL_ACCESS,
        &objAttr,
        NULL,
        PAGE_READONLY,
        SEC_IMAGE,
        fileHandle);

    if (!NT_SUCCESS(status))
    {
        CommonUtils::LogStatus("NtCreateSection", status);
        return NULL;
    }

    std::cout << "[+] SEC_IMAGE section created from payload file." << std::endl;
    return hSection;
}
