#include <windows.h>
#include <stdio.h>

int main() {
    HMODULE dll = LoadLibraryA("C:\\B-Plus\\zig\\injector.dll");
    if (!dll) { printf("LoadLibrary failed: %lu\n", GetLastError()); return 1; }
    printf("Loaded: %p\n", dll);

    FARPROC proc = GetProcAddress(dll, "TSS_Init");
    if (!proc) { printf("GetProcAddress failed: %lu\n", GetLastError()); return 1; }
    printf("Proc: %p\n", proc);

    DWORD64 ret = proc();
    printf("TSS_Init returned: %llu (0x%llX)\n", ret, ret);

    FreeLibrary(dll);
    return 0;
}
