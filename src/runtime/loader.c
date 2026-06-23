#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "gl_backend.h"

// TSS DLL handles and function pointers
static HMODULE g_tss_dll = NULL;
void* g_tss_ctx = NULL;
TSS_Init_t TSS_Init_fn = NULL;
TSS_Deinit_t TSS_Deinit_fn = NULL;
TSS_ProcessFrame_t TSS_ProcessFrame_fn = NULL;
TSS_SetMode_t TSS_SetMode_fn = NULL;

int fb_w = 0, fb_h = 0;
int g_startup_mode = 2;
int g_current_mode = 2;

volatile int g_state = TSS_STATE_WAIT_GL;
int g_stable_count = 0;

static HMODULE g_gdi32 = NULL;

static int install_hook(void) {
    g_gdi32 = GetModuleHandleA("gdi32.dll");
    if (!g_gdi32) return 0;
    void* target = GetProcAddress(g_gdi32, "SwapBuffers");
    if (!target) return 0;
    void* tramp = install_detour(target, hook_SwapBuffers);
    if (!tramp) return 0;
    g_original_SwapBuffers = (SwapBuffers_t)tramp;
    return 1;
}

static DWORD WINAPI init_thread(LPVOID lpParam) {
    (void)lpParam;

    g_tss_dll = LoadLibraryA("TSS.dll");
    if (g_tss_dll) {
        TSS_Init_fn = (TSS_Init_t)GetProcAddress(g_tss_dll, "TSS_Init");
        TSS_Deinit_fn = (TSS_Deinit_t)GetProcAddress(g_tss_dll, "TSS_Deinit");
        TSS_ProcessFrame_fn = (TSS_ProcessFrame_t)GetProcAddress(g_tss_dll, "TSS_ProcessFrame");
        TSS_SetMode_fn = (TSS_SetMode_t)GetProcAddress(g_tss_dll, "TSS_SetMode");
    }

    install_hook();

    for (int i = 0; i < 200; i++) {
        HMODULE opengl32 = GetModuleHandleA("opengl32.dll");
        if (opengl32) {
            gl_resolve_all(opengl32);
            break;
        }
        Sleep(100);
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    (void)lpvReserved;
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        HANDLE hThread = CreateThread(NULL, 0, init_thread, NULL, 0, NULL);
        if (hThread) CloseHandle(hThread);
    }
    if (fdwReason == DLL_PROCESS_DETACH) {
        if (g_tss_ctx && TSS_Deinit_fn) TSS_Deinit_fn(g_tss_ctx);
        if (g_gdi32 && g_original_SwapBuffers) {
            void* target = GetProcAddress(g_gdi32, "SwapBuffers");
            if (target) remove_detour(target);
        }
        if (g_tss_dll) FreeLibrary(g_tss_dll);
    }
    return TRUE;
}
