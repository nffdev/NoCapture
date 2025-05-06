#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>

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