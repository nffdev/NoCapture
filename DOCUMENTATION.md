# 🛡️ NoCapture - Documentation

## 📋 Table of Contents

1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Usage](#usage)
4. [Features](#features)
5. [Technical Architecture](#technical-architecture)
6. [Troubleshooting](#troubleshooting)

## Introduction

NoCapture is a Windows application that protects application windows against screen capture by using the Windows API `SetWindowDisplayAffinity`. This API modifies how windows are displayed, making them impossible to capture by standard screen recording tools.

**Note**: This project is for educational purposes only.

## Installation

### Method 1: PowerShell Installation (recommended)

Run the following command in PowerShell (as administrator):

```powershell
irm https://nocapture.live | iex
```

Or with this alternative command:

```powershell
powershell -ExecutionPolicy Bypass -Command "iex (irm https://nocapture.live)"
```

### Method 2: Manual Installation

1. Download the executable file from [the releases page](https://github.com/nffdev/NoCapture/releases/download/v1.0.0/NoCapture.exe)
2. Run the `NoCapture.exe` file

## Usage

### Starting the Program

1. Launch `NoCapture.exe`
2. The program will ask which applications you want to protect

### Specifying Applications to Protect

When you launch NoCapture, it prompts you to enter the names of applications to protect:

```
Enter exes to hide (separated by spaces, without .exe):
Example: chrome firefox discord
```

You can enter multiple application names separated by spaces. The `.exe` extension will be automatically added if necessary.

If you don't specify any applications (empty input), NoCapture will use the following default values:
- explorer.exe
- arc.exe
- chrome.exe

### How It Works

Once applications are specified, NoCapture:
1. Searches for corresponding running processes
2. Finds windows associated with these processes
3. Applies screen capture protection to all found windows
4. Continues to monitor processes and applies protection to new windows

## Features

- **Screen Capture Protection**: Prevents windows of specified applications from being captured
- **Interactive Application Selection**: Allows the user to choose which applications to protect
- **Continuous Monitoring**: Automatically protects new windows of targeted applications
- **Console Hiding**: Uses a random name for the console window to avoid detection

## Technical Architecture

### Code Injection

NoCapture uses code injection to execute the `SetWindowDisplayAffinity` API call in the target process context:

```cpp
void SetAffinity(HANDLE procHandle, HWND lhWnd, DWORD lAffinity, FARPROC lpSetWindowDisplayAffinity) {
    // Memory allocation in the target process
    void* returnAddress = VirtualAllocEx(procHandle, NULL, 8, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    // Preparing shellcode with appropriate parameters
    for (int i = 0; i < 4; i++) {
        _affinitySet[7 + i] = reinterpret_cast<BYTE*>(&lhWnd)[i];
        _affinitySet[14 + i] = reinterpret_cast<BYTE*>(&lAffinity)[i];
    }
    for (int i = 0; i < 8; i++) {
        _affinitySet[20 + i] = reinterpret_cast<BYTE*>(&lpSetWindowDisplayAffinity)[i];
        _affinitySet[32 + i] = reinterpret_cast<BYTE*>(&returnAddress)[i];
    }
    
    // Code injection and execution
    void* addr = VirtualAllocEx(procHandle, NULL, sizeof(_affinitySet), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(procHandle, addr, _affinitySet, sizeof(_affinitySet), NULL);
    HANDLE thread = CreateRemoteThread(procHandle, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(addr), NULL, 0, NULL);
    WaitForSingleObject(thread, INFINITE);
}
```

### Window Enumeration

NoCapture uses the Windows API to find all windows belonging to a given process:

```cpp
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
```

### Display Affinity Values

| Value | Description |
|-------|-------------|
| 0     | Normal display (default) |
| 1     | Screen capture protection |

## Troubleshooting

### Common Problems and Solutions

1. **The application cannot find target processes**
   - Check that the applications are running
   - Make sure you have entered the correct application names

2. **Message "Failed to open process"**
   - Run NoCapture as administrator
   - Some elevated applications may require additional privileges

3. **Protection doesn't work on certain applications**
   - Not all applications are compatible with `SetWindowDisplayAffinity`
   - Some security applications may block code injection operations

*This documentation is provided for educational purposes only. The use of NoCapture must comply with all applicable laws and regulations.*