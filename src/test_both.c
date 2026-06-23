#include <windows.h>
#include <stdio.h>

int main() {
    HMODULE dll;
    FARPROC proc;
    DWORD64 ret;

    // Test test_ptr.dll first
    dll = LoadLibraryA("C:\\TSS\\src\\test_ptr.dll");
    if (!dll) { printf("test_ptr.dll LoadLibrary failed: %lu\n", GetLastError()); }
    else {
        printf("test_ptr.dll loaded: %p\n", dll);
        proc = GetProcAddress(dll, "TSS_Mem");
        if (!proc) printf("TSS_Mem GetProcAddress failed: %lu\n", GetLastError());
        else { ret = proc(); printf("TSS_Mem returned: 0x%llX\n", ret); }
        FreeLibrary(dll);
    }

    // Then test_ret.dll
    dll = LoadLibraryA("C:\\TSS\\src\\test_ret.dll");
    if (!dll) { printf("test_ret.dll LoadLibrary failed: %lu\n", GetLastError()); }
    else {
        printf("test_ret.dll loaded: %p\n", dll);
        proc = GetProcAddress(dll, "S");
        if (!proc) printf("S GetProcAddress failed: %lu\n", GetLastError());
        else { ret = proc(); printf("S returned: 0x%llX\n", ret); }
        FreeLibrary(dll);
    }

    return 0;
}
