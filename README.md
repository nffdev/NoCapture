# 🛡️ NoCapture

## 📝 Overview

NoCapture is a Windows application that prevents screen capture by intercepting and obscuring visual output from target applications. It uses the Windows API `SetWindowDisplayAffinity` to protect application windows from being captured by screen recording tools.

**Note**: This project is for educational purposes only.

## ✨ Features

- Process listing with window handles
- Protection injection into target applications
- Easy protection removal
- Real-time process list updates

## 📚 How It Works

NoCapture works by injecting code into target processes to call the Windows API `SetWindowDisplayAffinity`. This API modifies how windows are displayed, making them impossible to capture by standard screen recording tools.

The protection process follows these steps:

1. User selects an application to protect
2. NoCapture opens the target process with necessary access rights
3. Shellcode is injected into the target process
4. The shellcode calls `SetWindowDisplayAffinity` with the appropriate parameter
5. The application window is now protected against screen capture

## 🛠️ Technical Implementation

### Code Injection

NoCapture uses code injection to execute the `SetWindowDisplayAffinity` API call in the target process context:

```cpp
void SetAffinity(HANDLE procHandle, HWND lhWnd, DWORD lAffinity, FARPROC lpSetWindowDisplayAffinity) {
    // Shellcode definition
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
    
    // Memory allocation and shellcode execution
    void* returnAddress = VirtualAllocEx(procHandle, NULL, 8, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    void* addr = VirtualAllocEx(procHandle, NULL, sizeof(_affinitySet), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(procHandle, addr, _affinitySet, sizeof(_affinitySet), NULL);
    HANDLE thread = CreateRemoteThread(procHandle, NULL, 0, (LPTHREAD_START_ROUTINE)addr, NULL, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
}
```

### Window Enumeration

```cpp
static std::vector<HWND> FindWindowHandles(DWORD processId) {
    EnumWindowsData data;
    data.processId = processId;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
    return data.hwnds;
}
```
