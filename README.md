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
