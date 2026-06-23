#include <windows.h>
#include <stdio.h>

int main() {
    HMODULE dll = LoadLibraryA("C:\\TSS\\src\\test_ret.dll");
    if (!dll) { printf("LoadLibrary failed: %lu\n", GetLastError()); return 1; }
    printf("Loaded: %p\n", dll);
    
    FARPROC proc = GetProcAddress(dll, "S");
    if (!proc) { printf("GetProcAddress failed: %lu\n", GetLastError()); return 1; }
    printf("Proc: %p\n", proc);
    
    DWORD64 ret = proc();
    printf("ret = %llu (0x%llX)\n", ret, ret);
    
    FreeLibrary(dll);
    return 0;
}
