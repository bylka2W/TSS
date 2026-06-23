#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdint.h>

struct Detour { unsigned char saved[5]; void* trampoline; };
static struct Detour g_detour;

static void* alloc_exec(size_t size) {
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}

void* install_detour(void* target, void* hook) {
    DWORD old;
    memcpy(g_detour.saved, target, 5);
    void* tramp = alloc_exec(32);
    if (!tramp) return NULL;
    memcpy(tramp, g_detour.saved, 5);
    int32_t back_disp = (int32_t)((intptr_t)target + 5 - (intptr_t)tramp - 10);
    unsigned char* back_jmp = (unsigned char*)tramp + 5;
    back_jmp[0] = 0xE9;
    memcpy(back_jmp + 1, &back_disp, 4);
    VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old);
    int32_t hook_disp = (int32_t)((intptr_t)hook - (intptr_t)target - 5);
    unsigned char jmp_code[5] = { 0xE9 };
    memcpy(jmp_code + 1, &hook_disp, 4);
    memcpy(target, jmp_code, 5);
    VirtualProtect(target, 5, old, &old);
    g_detour.trampoline = tramp;
    return tramp;
}

void remove_detour(void* target) {
    DWORD old;
    VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old);
    memcpy(target, g_detour.saved, 5);
    VirtualProtect(target, 5, old, &old);
}
