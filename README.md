# 🛡️ NoCapture

## 📝 Overview

NoCapture is a Windows application that prevents screen capture by intercepting and obscuring visual output from target applications. It uses the Windows API `SetWindowDisplayAffinity` to protect application windows from being captured by screen recording tools.

**Note**: This project is for educational purposes only.

## ✨ Features

- Process listing with window handles
- Protection injection into target applications
- Easy protection removal
- Real-time process list updates

## Usage Guide

1. Run NoCapture.exe
2. Select a process from the list
3. Click "Inject" to enable screen capture protection
4. Click "Detach" to disable protection
5. Use "Refresh" to update the process list

## Technical Notes

### Display Affinity Values

| Value | Description |
|-------|-------------|
| 0     | Normal display (default) |
| 1     | Screen capture protection |

### Compatibility

- Windows 10 (all versions)
- Windows 11 (all versions)

---

*This documentation is provided for educational purposes only. Use of NoCapture must comply with all applicable laws and regulations.*
