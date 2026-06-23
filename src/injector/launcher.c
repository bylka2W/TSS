#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>

static BOOL find_hl_process(DWORD* out_pid) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return FALSE;
    PROCESSENTRY32 pe = { sizeof(pe) };
    BOOL found = FALSE;
    if (Process32First(snap, &pe)) do {
        if (_stricmp(pe.szExeFile, "hl.exe") == 0) {
            *out_pid = pe.th32ProcessID;
            found = TRUE;
            break;
        }
    } while (Process32Next(snap, &pe));
    CloseHandle(snap);
    return found;
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    char exe_dir[MAX_PATH];
    GetModuleFileNameA(NULL, exe_dir, sizeof(exe_dir));
    char* last_slash = strrchr(exe_dir, '\\');
    if (last_slash) *last_slash = '\0';

    char dll_path[MAX_PATH];
    _snprintf(dll_path, sizeof(dll_path), "%s\\TSS_loader.dll", exe_dir);

    if (GetFileAttributesA(dll_path) == INVALID_FILE_ATTRIBUTES) {
        char err[512];
        _snprintf(err, sizeof(err), "TSS_loader.dll not found at:\n%s", dll_path);
        MessageBoxA(NULL, err, "TSS Launcher Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    DWORD existing_pid = 0;
    if (find_hl_process(&existing_pid)) {
        HANDLE hExisting = OpenProcess(PROCESS_ALL_ACCESS, FALSE, existing_pid);
        if (hExisting) {
            TerminateProcess(hExisting, 1);
            CloseHandle(hExisting);
            Sleep(500);
        }
    }

    char run_game_path[MAX_PATH];
    _snprintf(run_game_path, sizeof(run_game_path), "%s\\RUN_GAME.exe", exe_dir);

    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    sei.lpVerb = "open";
    sei.lpFile = run_game_path;
    sei.lpParameters = "-launch hl.exe";
    sei.lpDirectory = exe_dir;
    sei.nShow = SW_SHOWNORMAL;

    BOOL ok = ShellExecuteExA(&sei);
    if (!ok) {
        char err[512];
        _snprintf(err, sizeof(err), "Failed to start RUN_GAME.exe via ShellExecute\nError: %d", GetLastError());
        MessageBoxA(NULL, err, "TSS Launcher Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    DWORD hl_pid = 0;
    for (int i = 0; i < 300; i++) {
        if (find_hl_process(&hl_pid)) break;
        Sleep(100);
    }

    if (sei.hProcess) CloseHandle(sei.hProcess);

    if (!hl_pid) {
        MessageBoxA(NULL, "hl.exe did not start within 30s", "TSS Launcher Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    Sleep(1000);

    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
        FALSE, hl_pid
    );
    if (!hProcess) {
        char err[512];
        _snprintf(err, sizeof(err), "Cannot open hl.exe (PID: %d)\nError: %d\n\nTry running as Administrator.", hl_pid, GetLastError());
        MessageBoxA(NULL, err, "TSS Launcher Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    size_t dll_path_len = strlen(dll_path) + 1;
    void* remote_mem = VirtualAllocEx(hProcess, NULL, dll_path_len,
                                       MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_mem) {
        MessageBoxA(NULL, "VirtualAllocEx failed", "TSS Launcher Error", MB_OK | MB_ICONERROR);
        CloseHandle(hProcess);
        return 1;
    }

    if (!WriteProcessMemory(hProcess, remote_mem, dll_path, dll_path_len, NULL)) {
        MessageBoxA(NULL, "WriteProcessMemory failed", "TSS Launcher Error", MB_OK | MB_ICONERROR);
        VirtualFreeEx(hProcess, remote_mem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    void* loadlib = (void*)GetProcAddress(kernel32, "LoadLibraryA");
    if (!loadlib) {
        MessageBoxA(NULL, "GetProcAddress(LoadLibraryA) failed", "TSS Launcher Error", MB_OK | MB_ICONERROR);
        VirtualFreeEx(hProcess, remote_mem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    HANDLE hRemoteThread = CreateRemoteThread(
        hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)loadlib,
        remote_mem, 0, NULL
    );

    if (!hRemoteThread) {
        char err[512];
        _snprintf(err, sizeof(err), "CreateRemoteThread failed.\nError: %d\n\n"
                  "Try running as Administrator.", GetLastError());
        MessageBoxA(NULL, err, "TSS Launcher Error", MB_OK | MB_ICONERROR);
        VirtualFreeEx(hProcess, remote_mem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    WaitForSingleObject(hRemoteThread, 10000);
    CloseHandle(hRemoteThread);
    VirtualFreeEx(hProcess, remote_mem, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return 0;
}
