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

    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
        EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);
        DWORD windowProcessId;
        GetWindowThreadProcessId(hwnd, &windowProcessId);

        if (windowProcessId == data->processId && IsWindowVisible(hwnd) && (GetWindow(hwnd, GW_OWNER) == NULL)) {
            data->hwnds.push_back(hwnd);
        }
        return TRUE;
    }

    static std::vector<HWND> FindWindowHandles(DWORD processId) {
        EnumWindowsData data;
        data.processId = processId;
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
        return data.hwnds;
    }

    public ref class MyForm : public System::Windows::Forms::Form {
    public:
        MyForm(void) {
            InitializeComponent();
            LoadProcessList();
        }

    protected:
        ~MyForm() {
            if (components) {
                delete components;
            }
        }

    private:
        System::Windows::Forms::DataGridView^ dataGridView1;
        System::ComponentModel::Container^ components;
        System::Windows::Forms::Button^ refreshButton;
    };
}