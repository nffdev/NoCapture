#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <string>
#include <vector>

namespace NoCapture {
    using namespace System;
    using namespace System::ComponentModel;
    using namespace System::Collections;
    using namespace System::Windows::Forms;
    using namespace System::Data;
    using namespace System::Drawing;

    struct EnumWindowsData {
        DWORD processId;
        std::vector<HWND> hwnds;
    };
}