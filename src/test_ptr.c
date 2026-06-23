#include <windows.h>
#include <stdio.h>
#include <inttypes.h>

typedef void* (*Fn)();

int main() {
    HMODULE dll = LoadLibraryA("test_ptr.dll");
    if (!dll) { printf("LoadLibrary failed: %lu\n", GetLastError()); return 1; }
    Fn f = (Fn)GetProcAddress(dll, "TSS_Mem");
    if (!f) { printf("GetProcAddress failed\n"); return 1; }
    uintptr_t r = (uintptr_t)f();
    printf("return = 0x%llX (%llu)\n", r, r);
    FreeLibrary(dll);
    return 0;
}
