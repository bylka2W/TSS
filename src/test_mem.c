#include <windows.h>
#include <stdio.h>
#include <inttypes.h>

typedef void* (*TSS_Mem_Fn)();
typedef void* (*TSS_AllocPage_Fn)();

int main() {
    HMODULE dll = LoadLibraryA("tss_mem.dll");
    if (!dll) { printf("LoadLibrary failed: %lu\n", GetLastError()); return 1; }

    TSS_Mem_Fn init = (TSS_Mem_Fn)GetProcAddress(dll, "TSS_Mem");
    TSS_AllocPage_Fn alloc = (TSS_AllocPage_Fn)GetProcAddress(dll, "TSS_AllocPage");
    if (!init || !alloc) { printf("GetProcAddress failed\n"); return 1; }

    void *ctl = init();
    printf("ctl = %p\n", ctl);

    void *p1 = alloc();
    void *p2 = alloc();
    void *p3 = alloc();

    printf("p1 = %p\n", p1);
    printf("p2 = %p\n", p2);
    printf("p3 = %p\n", p3);

    if (p1 && (uintptr_t)p2 - (uintptr_t)p1 == 4096 && (uintptr_t)p3 - (uintptr_t)p2 == 4096) {
        printf("PASS: pages contiguous +4KB\n");
    } else {
        printf("FAIL\n");
    }

    FreeLibrary(dll);
    return 0;
}
