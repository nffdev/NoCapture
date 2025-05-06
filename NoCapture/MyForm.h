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
    };
}