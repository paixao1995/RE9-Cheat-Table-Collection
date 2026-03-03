#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <shellapi.h>

namespace fs = std::filesystem;

// Console colors
enum Color {
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    YELLOW = 6,
    WHITE = 7,
    GRAY = 8,
    LIGHT_BLUE = 9,
    LIGHT_GREEN = 10,
    LIGHT_CYAN = 11,
    LIGHT_RED = 12,
    LIGHT_MAGENTA = 13,
    LIGHT_YELLOW = 14,
    BRIGHT_WHITE = 15
};

void SetColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void PrintBanner() {
    SetColor(LIGHT_CYAN);
    std::cout << R"(
    ╔══════════════════════════════════════════════════════════╗
    ║  ███████╗███████╗██████╗     ██████╗██╗  ██╗███████╗ █████╗ ████████╗ ║
    ║  ██╔════╝██╔════╝╚════██╗   ██╔════╝██║  ██║██╔════╝██╔══██╗╚══██╔══╝ ║
    ║  █████╗  █████╗   █████╔╝   ██║     ███████║█████╗  ███████║   ██║    ║
    ║  ██╔══╝  ██╔══╝   ╚═══██╗   ██║     ██╔══██║██╔══╝  ██╔══██║   ██║    ║
    ║  ███████╗███████╗██████╔╝██╗╚██████╗██║  ██║███████╗██║  ██║   ██║    ║
    ║  ╚══════╝╚══════╝╚═════╝ ╚═╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝   ╚═╝    ║
    ╚══════════════════════════════════════════════════════════╝
    )" << std::endl;
    SetColor(WHITE);
    std::cout << "            Resident Evil Requiem Cheat Engine Auto-Attacher v2.0\n";
    std::cout << "            ===================================================\n\n";
}

// Find process ID by name
DWORD GetProcessIdByName(const std::wstring& processName) {
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(entry);
        if (Process32FirstW(snapshot, &entry)) {
            do {
                if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0) {
                    processId = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
    return processId;
}

// Check if Cheat Engine is installed
std::wstring FindCheatEnginePath() {
    const wchar_t* paths[] = {
        L"C:\\Program Files\\Cheat Engine 7.0\\Cheat Engine.exe",
        L"C:\\Program Files (x86)\\Cheat Engine 7.0\\Cheat Engine.exe",
        L"C:\\Program Files\\Cheat Engine 7.1\\Cheat Engine.exe",
        L"C:\\Program Files (x86)\\Cheat Engine 7.1\\Cheat Engine.exe",
        L"C:\\Program Files\\Cheat Engine 7.2\\Cheat Engine.exe",
        L"C:\\Program Files (x86)\\Cheat Engine 7.2\\Cheat Engine.exe",
        L"C:\\Program Files\\Cheat Engine\\Cheat Engine.exe",
        L"C:\\Program Files (x86)\\Cheat Engine\\Cheat Engine.exe"
    };
    
    for (auto path : paths) {
        if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) {
            return path;
        }
    }
    return L"";
}

// Get game version from executable
std::string GetGameVersion(const std::string& gamePath) {
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeA(gamePath.c_str(), &handle);
    if (size > 0) {
        std::vector<char> buffer(size);
        if (GetFileVersionInfoA(gamePath.c_str(), handle, size, buffer.data())) {
            VS_FIXEDFILEINFO* fileInfo = nullptr;
            UINT len = 0;
            if (VerQueryValueA(buffer.data(), "\\", (LPVOID*)&fileInfo, &len)) {
                int major = HIWORD(fileInfo->dwFileVersionMS);
                int minor = LOWORD(fileInfo->dwFileVersionMS);
                int patch = HIWORD(fileInfo->dwFileVersionLS);
                return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
            }
        }
    }
    return "Unknown";
}

// Select cheat tables from folder
std::vector<std::string> GetAvailableTables(const std::string& tablesPath) {
    std::vector<std::string> tables;
    if (!fs::exists(tablesPath)) return tables;
    
    for (const auto& entry : fs::directory_iterator(tablesPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".CT") {
            tables.push_back(entry.path().filename().string());
        }
    }
    return tables;
}

// Simple pattern scan (for demonstration)
bool ScanPattern(HANDLE hProcess, uintptr_t start, size_t size, const std::vector<byte>& pattern) {
    std::vector<byte> buffer(size);
    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(hProcess, (LPCVOID)start, buffer.data(), size, &bytesRead))
        return false;
    
    for (size_t i = 0; i <= bytesRead - pattern.size(); i++) {
        bool found = true;
        for (size_t j = 0; j < pattern.size(); j++) {
            if (buffer[i + j] != pattern[j]) {
                found = false;
                break;
            }
        }
        if (found) return true;
    }
    return false;
}

int main() {
    PrintBanner();
    
    // Get current directory
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    fs::path currentDir = fs::path(exePath).parent_path();
    fs::path tablesPath = currentDir / "cheat_tables";
    
    // Check if tables folder exists
    if (!fs::exists(tablesPath)) {
        SetColor(LIGHT_RED);
        std::cout << "[-] Error: cheat_tables folder not found!\n";
        SetColor(WHITE);
        std::cout << "    Please make sure this executable is in the same directory as the cheat_tables folder.\n";
        system("pause");
        return 1;
    }
    
    // Check if game is running
    std::wstring targetProcess = L"RE9.exe";
    SetColor(LIGHT_YELLOW);
    std::cout << "[*] Scanning for RE9 process..." << std::endl;
    SetColor(WHITE);
    
    DWORD pid = GetProcessIdByName(targetProcess);
    if (pid == 0) {
        SetColor(LIGHT_RED);
        std::cout << "[-] RE9 is not running. Please launch the game first.\n";
        SetColor(WHITE);
        system("pause");
        return 1;
    }
    
    SetColor(LIGHT_GREEN);
    std::cout << "[+] Found RE9 (PID: " << pid << ")\n";
    SetColor(WHITE);
    
    // Try to open process
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess) {
        wchar_t gamePath[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, NULL, gamePath, MAX_PATH)) {
            std::string gamePathA(gamePath, gamePath + wcslen(gamePath));
            std::string version = GetGameVersion(gamePathA);
            SetColor(LIGHT_GREEN);
            std::cout << "[+] Game version: " << version << std::endl;
            SetColor(WHITE);
        }
        CloseHandle(hProcess);
    }
    
    // Check Cheat Engine
    std::wstring cePath = FindCheatEnginePath();
    if (!cePath.empty()) {
        SetColor(LIGHT_GREEN);
        std::cout << "[+] Cheat Engine found: " << std::string(cePath.begin(), cePath.end()) << std::endl;
        SetColor(WHITE);
    } else {
        SetColor(LIGHT_YELLOW);
        std::cout << "[!] Cheat Engine not found in default locations.\n";
        std::cout << "    You can still use tables manually.\n";
        SetColor(WHITE);
    }
    
    // Get available tables
    auto tables = GetAvailableTables(tablesPath.string());
    if (tables.empty()) {
        SetColor(LIGHT_RED);
        std::cout << "[-] No cheat tables found in cheat_tables folder!\n";
        SetColor(WHITE);
        system("pause");
        return 1;
    }
    
    SetColor(LIGHT_GREEN);
    std::cout << "[+] Found " << tables.size() << " cheat tables:\n";
    SetColor(LIGHT_CYAN);
    for (size_t i = 0; i < tables.size(); i++) {
        std::cout << "    " << (i+1) << ". " << tables[i] << std::endl;
    }
    SetColor(WHITE);
    
    // Select tables to load
    std::cout << "\nSelect tables to enable (enter numbers separated by spaces, or 'all'):\n";
    std::cout << "> ";
    
    std::string input;
    std::cin.ignore();
    std::getline(std::cin, input);
    
    std::vector<int> selections;
    if (input == "all" || input == "ALL") {
        for (size_t i = 0; i < tables.size(); i++) {
            selections.push_back(i);
        }
    } else {
        std::stringstream ss(input);
        int num;
        while (ss >> num) {
            if (num >= 1 && num <= (int)tables.size()) {
                selections.push_back(num - 1);
            }
        }
    }
    
    if (selections.empty()) {
        SetColor(LIGHT_RED);
        std::cout << "[-] No valid tables selected.\n";
        SetColor(WHITE);
        system("pause");
        return 1;
    }
    
    SetColor(LIGHT_GREEN);
    std::cout << "[+] Selected " << selections.size() << " tables.\n";
    SetColor(WHITE);
    
    // Simulate loading
    std::cout << "\n[*] Preparing to attach to RE9..." << std::endl;
    Sleep(1000);
    
    for (int idx : selections) {
        std::cout << "[*] Loading " << tables[idx] << "..." << std::endl;
        Sleep(500);
        
        // Simulate AOB scan (just for show)
        if (idx % 2 == 0) {
            SetColor(LIGHT_GREEN);
            std::cout << "    [+] AOB pattern found, applying patch..." << std::endl;
            SetColor(WHITE);
        } else {
            SetColor(LIGHT_YELLOW);
            std::cout << "    [!] Using direct memory address (fallback mode)..." << std::endl;
            SetColor(WHITE);
        }
        Sleep(300);
    }
    
    SetColor(LIGHT_GREEN);
    std::cout << "\n[SUCCESS] All selected cheats have been enabled!\n";
    SetColor(LIGHT_YELLOW);
    std::cout << "\nNote: This is a demonstration. In the full version, Cheat Engine would be\n";
    std::cout << "      launched automatically with the selected tables.\n";
    SetColor(WHITE);
    
    std::cout << "\nPress any key to exit...\n";
    system("pause > nul");
    return 0;
}