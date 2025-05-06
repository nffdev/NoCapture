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

        void InitializeComponent(void) {
            this->dataGridView1 = (gcnew System::Windows::Forms::DataGridView());
            this->SuspendLayout();

            // 
            // dataGridView1
            // 
            this->dataGridView1->AllowUserToAddRows = false;
            this->dataGridView1->AllowUserToDeleteRows = false;
            this->dataGridView1->AllowUserToResizeRows = false;
            this->dataGridView1->ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode::AutoSize;
            this->dataGridView1->Location = System::Drawing::Point(12, 12);
            this->dataGridView1->Name = L"dataGridView1";
            this->dataGridView1->Size = System::Drawing::Size(660, 400);
            this->dataGridView1->TabIndex = 0;

            this->refreshButton = (gcnew System::Windows::Forms::Button());
            this->refreshButton->Location = System::Drawing::Point(12, 420);
            this->refreshButton->Size = System::Drawing::Size(100, 30);
            this->refreshButton->Text = L"Refresh";
            this->refreshButton->Click += gcnew EventHandler(this, &MyForm::RefreshButton_Click);
            this->Controls->Add(this->refreshButton);

            this->dataGridView1->Columns->Add("Process", "Process");
            this->dataGridView1->Columns->Add("PID", "PID");

            DataGridViewButtonColumn^ injectButtonColumn = gcnew DataGridViewButtonColumn();
            injectButtonColumn->HeaderText = "Inject";
            injectButtonColumn->Text = "Inject";
            injectButtonColumn->UseColumnTextForButtonValue = true;
            this->dataGridView1->Columns->Add(injectButtonColumn);

            DataGridViewButtonColumn^ detachButtonColumn = gcnew DataGridViewButtonColumn();
            detachButtonColumn->HeaderText = "Detach";
            detachButtonColumn->Text = "Detach";
            detachButtonColumn->UseColumnTextForButtonValue = true;
            this->dataGridView1->Columns->Add(detachButtonColumn);

            DataGridViewButtonColumn^ hideButtonColumn = gcnew DataGridViewButtonColumn();
            hideButtonColumn->HeaderText = "Hide";
            hideButtonColumn->Text = "Hide";
            hideButtonColumn->UseColumnTextForButtonValue = true;
            this->dataGridView1->Columns->Add(hideButtonColumn);

            this->dataGridView1->Columns->Add("IsInjected", "Is injected");
            this->dataGridView1->Columns->Add("WindowHandle", "Window Handle");

            // 
            // MyForm
            // 
            this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->ClientSize = System::Drawing::Size(680, 450);
            this->Controls->Add(this->dataGridView1);
            this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
            this->Name = L"MyForm";
            this->Text = L"NoCapture";
            this->ResumeLayout(false);

            this->dataGridView1->CellClick += gcnew DataGridViewCellEventHandler(this, &MyForm::dataGridView1_CellClick);
        }

        void LoadProcessList() {
            HANDLE hProcessSnap;
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);

            hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hProcessSnap == INVALID_HANDLE_VALUE) {
                MessageBox::Show("Failed to create process snapshot.");
                return;
            }

            if (Process32First(hProcessSnap, &pe32)) {
                do {
                    String^ processName = gcnew String(pe32.szExeFile);
                    int pid = pe32.th32ProcessID;

                    std::vector<HWND> hwnds = FindWindowHandles(pid);
                    String^ handleStr = "N/A";

                    if (!hwnds.empty()) {
                        handleStr = ((INT64)hwnds[0]).ToString(); 
                    }

                    this->dataGridView1->Rows->Add(processName, pid, "Inject", "Detach", "Hide", "No", handleStr);
                } while (Process32Next(hProcessSnap, &pe32));
            }
            CloseHandle(hProcessSnap);
        }

        void RefreshButton_Click(System::Object^ sender, System::EventArgs^ e) {
            this->dataGridView1->Rows->Clear();
            LoadProcessList();
        }

        void dataGridView1_CellClick(System::Object^ sender, DataGridViewCellEventArgs^ e) {
            if (e->RowIndex >= 0 && e->ColumnIndex == 2) { 
                String^ processName = this->dataGridView1->Rows[e->RowIndex]->Cells[0]->Value->ToString();
                int pid = Convert::ToInt32(this->dataGridView1->Rows[e->RowIndex]->Cells[1]->Value);
                String^ handleStr = this->dataGridView1->Rows[e->RowIndex]->Cells[6]->Value->ToString();

                if (handleStr == "N/A") {
                    MessageBox::Show("No window handle found for this process.");
                    return;
                }

                HWND hwnd = (HWND)Convert::ToInt64(handleStr);
                HANDLE procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
                if (procHandle == NULL) {
                    MessageBox::Show("Failed to open process: " + processName);
                    return;
                }

                DWORD_PTR lAffinity = 1;
                HMODULE hUser32 = LoadLibrary(L"user32.dll");
                if (hUser32 == NULL) {
                    CloseHandle(procHandle);
                    MessageBox::Show("Failed to load user32.dll");
                    return;
                }

                FARPROC lpSetWindowDisplayAffinity = GetProcAddress(hUser32, "SetWindowDisplayAffinity");
                if (lpSetWindowDisplayAffinity == NULL) {
                    FreeLibrary(hUser32);
                    CloseHandle(procHandle);
                    MessageBox::Show("Failed to get SetWindowDisplayAffinity address");
                    return;
                }

                SetAffinity(procHandle, hwnd, lAffinity, lpSetWindowDisplayAffinity);

                FreeLibrary(hUser32);
                CloseHandle(procHandle);

                this->dataGridView1->Rows[e->RowIndex]->Cells[5]->Value = "Yes";
            }
            else if (e->RowIndex >= 0 && (e->ColumnIndex == 3 || e->ColumnIndex == 4)) { 
                String^ processName = this->dataGridView1->Rows[e->RowIndex]->Cells[0]->Value->ToString();
                int pid = Convert::ToInt32(this->dataGridView1->Rows[e->RowIndex]->Cells[1]->Value);

                if (e->ColumnIndex == 3) {
                    HWND hwnd = (HWND)Convert::ToInt64(this->dataGridView1->Rows[e->RowIndex]->Cells[6]->Value->ToString());
                    UnhookProcess(pid, hwnd);
                    this->dataGridView1->Rows[e->RowIndex]->Cells[5]->Value = "No";
                    MessageBox::Show("Detached from process: " + processName + " (PID: " + pid + ")");
                }
                else if (e->ColumnIndex == 4) {
                    MessageBox::Show("Hiding process: " + processName + " (PID: " + pid + ")");
                }
            }
        }

        void UnhookProcess(int pid, HWND hwnd) {
            HANDLE procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
            if (procHandle == NULL) {
                MessageBox::Show("Failed to open process for unhooking.");
                return;
            }

            HMODULE hUser32 = LoadLibrary(L"user32.dll");
            if (hUser32 == NULL) {
                CloseHandle(procHandle);
                MessageBox::Show("Failed to load user32.dll");
                return;
            }

            FARPROC lpSetWindowDisplayAffinity = GetProcAddress(hUser32, "SetWindowDisplayAffinity");
            if (lpSetWindowDisplayAffinity == NULL) {
                FreeLibrary(hUser32);
                CloseHandle(procHandle);
                MessageBox::Show("Failed to get SetWindowDisplayAffinity address");
                return;
            }

            DWORD_PTR lAffinity = 0;
            SetAffinity(procHandle, hwnd, lAffinity, lpSetWindowDisplayAffinity);

            FreeLibrary(hUser32);
            CloseHandle(procHandle);
        }

        void SetAffinity(HANDLE procHandle, HWND lhWnd, DWORD lAffinity, FARPROC lpSetWindowDisplayAffinity) {
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

            void* returnAddress = VirtualAllocEx(procHandle, NULL, 8, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (returnAddress == NULL) {
                MessageBox::Show("Failed to allocate memory for return value in target process.");
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
                MessageBox::Show("Failed to allocate memory in the target process.");
                VirtualFreeEx(procHandle, returnAddress, 8, MEM_RELEASE);
                return;
            }

            BOOL success = WriteProcessMemory(procHandle, addr, _affinitySet, sizeof(_affinitySet), NULL);
            if (!success) {
                MessageBox::Show("Failed to write to process memory.");
                VirtualFreeEx(procHandle, addr, sizeof(_affinitySet), MEM_RELEASE);
                VirtualFreeEx(procHandle, returnAddress, 8, MEM_RELEASE);
                return;
            }

            HANDLE thread = CreateRemoteThread(procHandle, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(addr), NULL, 0, NULL);
            if (thread == NULL) {
                MessageBox::Show("Failed to create remote thread.");
                VirtualFreeEx(procHandle, addr, sizeof(_affinitySet), MEM_RELEASE);
                VirtualFreeEx(procHandle, returnAddress, 8, MEM_RELEASE);
                return;
            }

            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);

            BYTE returnValue;
            success = ReadProcessMemory(procHandle, returnAddress, &returnValue, sizeof(BYTE), NULL);
            if (!success) {
                MessageBox::Show("Failed to read return value from process memory.");
            }
            else {
                MessageBox::Show("SetWindowDisplayAffinity returned: " + (returnValue ? "TRUE" : "FALSE"));
            }

            VirtualFreeEx(procHandle, addr, sizeof(_affinitySet), MEM_RELEASE);
            VirtualFreeEx(procHandle, returnAddress, 8, MEM_RELEASE);
        }
    };
}