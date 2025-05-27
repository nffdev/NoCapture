#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <random>
#include <ctime>

BYTE _affinitySet[] = {
    0x48, 0x83, 0xEC, 0x28,                         // sub rsp, 0x28
    0x48, 0xC7, 0xC1, 0x00, 0x00, 0x00, 0x00,       // mov rcx, 0 (hwnd placeholder)
    0x48, 0xC7, 0xC2, 0x00, 0x00, 0x00, 0x00,       // mov rdx, 0 (affinity value placeholder)
    0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // movabs rax, 0 (function address placeholder)
    0xFF, 0xD0,                                     // call rax
    0x48, 0xA3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // mov [absolute_address], rax
    0x48, 0x83, 0xC4, 0x28,                         // add rsp, 0x28
    0xC3                                            // ret
};

void SetAffinity(HANDLE procHandle, HWND lhWnd, DWORD lAffinity, FARPROC lpSetWindowDisplayAffinity) {
    void* returnAddress = VirtualAllocEx(procHandle, NULL, 8, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (returnAddress == NULL) {
        std::cerr << "Failed to allocate memory for return value in target process" << std::endl;
        return;
    }

    for (int i = 0; i < 4; i++) {
        _affinitySet[7 + i] = reinterpret_cast<BYTE*>(&lhWnd)[i];
        _affinitySet[14 + i] = reinterpret_cast<BYTE*>(&lAffinity)[i];
    }
    for (int i = 0; i < 8; i++) {
        _affinitySet[20 + i] = reinterpret_cast<BYTE*>(&lpSetWindowDisplayAffinity)[i];
        _affinitySet[32 + i] = reinterpret_cast<BYTE*>(&returnAddress)[i];
    }

    void* addr = VirtualAllocEx(procHandle, NULL, sizeof(_affinitySet), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (addr == NULL) {
        std::cerr << "Failed to allocate memory in the target process" << std::endl;
        VirtualFreeEx(procHandle, returnAddress, 8, MEM_RELEASE);
        return;
    }

    BOOL success = WriteProcessMemory(procHandle, addr, _affinitySet, sizeof(_affinitySet), NULL);
    if (!success) {
        std::cerr << "Failed to write to process memory" << std::endl;
        VirtualFreeEx(procHandle, addr, sizeof(_affinitySet), MEM_RELEASE);
        VirtualFreeEx(procHandle, returnAddress, 8, MEM_RELEASE);
        return;
    }

    HANDLE thread = CreateRemoteThread(procHandle, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(addr), NULL, 0, NULL);
    if (thread == NULL) {
        std::cerr << "Failed to create remote thread" << std::endl;
        VirtualFreeEx(procHandle, addr, sizeof(_affinitySet), MEM_RELEASE);
        VirtualFreeEx(procHandle, returnAddress, 8, MEM_RELEASE);
        return;
    }

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    BYTE returnValue;
    success = ReadProcessMemory(procHandle, returnAddress, &returnValue, sizeof(BYTE), NULL);
    if (!success) {
        std::cerr << "Failed to read return value from process memory" << std::endl;
    }
    else {
        std::cout << "SetWindowDisplayAffinity returned: " << (returnValue ? "TRUE" : "FALSE") << std::endl;
    }

    VirtualFreeEx(procHandle, addr, sizeof(_affinitySet), MEM_RELEASE);
    VirtualFreeEx(procHandle, returnAddress, 8, MEM_RELEASE);
}

std::vector<DWORD> FindTargetProcessIds(const std::vector<std::wstring>& targetExecutables) {
    std::vector<DWORD> processIds;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create process snapshot" << std::endl;
        return processIds;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnapshot, &pe32)) {
        do {
            for (const auto& exeName : targetExecutables) {
                if (_wcsicmp(pe32.szExeFile, exeName.c_str()) == 0) {
                    processIds.push_back(pe32.th32ProcessID);
                }
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    else {
        std::cerr << "Failed to get first process" << std::endl;
    }
    CloseHandle(hSnapshot);
    return processIds;
}

void RandomizeConsoleName() {
    std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
    
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    
    std::uniform_int_distribution<> length_dist(8, 20);
    int length = length_dist(rng);
    
    std::uniform_int_distribution<> char_dist(0, static_cast<int>(chars.size() - 1));
    std::wstring randomName = L"";
    for (int i = 0; i < length; ++i) {
        randomName += chars[char_dist(rng)];
    }
    
    SetConsoleTitle(randomName.c_str());
}

struct EnumWindowsData {
    DWORD processId;
    std::vector<HWND> hwnds;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);
    DWORD windowProcessId;
    GetWindowThreadProcessId(hwnd, &windowProcessId);

    if (windowProcessId == data->processId && IsWindowVisible(hwnd) && (GetWindow(hwnd, GW_OWNER) == NULL)) {
        data->hwnds.push_back(hwnd);
    }
    return TRUE;
}

std::vector<HWND> FindWindows(DWORD processId) {
    EnumWindowsData data;
    data.processId = processId;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
    return data.hwnds;
}

int main(int argc, char* argv[]) {
    RandomizeConsoleName();
    
    std::vector<std::wstring> targetExecutables;

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            std::wstring exeName = std::wstring(argv[i], argv[i] + strlen(argv[i]));
            targetExecutables.push_back(exeName);
        }
    }
    else {
        targetExecutables = { L"explorer.exe", L"arc.exe", L"chrome.exe" };
    }

    std::wcout << L"Target executables: ";
    for (const auto& exe : targetExecutables) {
        std::wcout << exe << L" ";
    }
    std::wcout << std::endl;

    while (true) {
        std::vector<DWORD> processIds = FindTargetProcessIds(targetExecutables);
        if (processIds.empty()) {
            std::cerr << "Target processes not found" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        for (DWORD processId : processIds) {
            std::vector<HWND> hwnds = FindWindows(processId);
            if (!hwnds.empty()) {
                std::cout << "Found windows for process ID: " << processId << std::endl;
                for (HWND hwnd : hwnds) {
                    std::cout << "Window handle: " << hwnd << std::endl;
                    HANDLE procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
                    if (procHandle == NULL) {
                        std::cerr << "Failed to open process" << std::endl;
                        continue;
                    }

                    DWORD_PTR lAffinity = 1;
                    HMODULE hUser32 = LoadLibrary(L"user32.dll");
                    if (hUser32 == NULL) {
                        std::cerr << "Failed to load user32.dll" << std::endl;
                        CloseHandle(procHandle);
                        continue;
                    }

                    FARPROC lpSetWindowDisplayAffinity = GetProcAddress(hUser32, "SetWindowDisplayAffinity");
                    if (lpSetWindowDisplayAffinity == NULL) {
                        std::cerr << "Failed to get SetWindowDisplayAffinity address" << std::endl;
                        FreeLibrary(hUser32);
                        CloseHandle(procHandle);
                        continue;
                    }

                    SetAffinity(procHandle, hwnd, lAffinity, lpSetWindowDisplayAffinity);

                    FreeLibrary(hUser32);
                    CloseHandle(procHandle);
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}