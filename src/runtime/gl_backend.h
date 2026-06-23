#ifndef TSS_GL_BACKEND_H
#define TSS_GL_BACKEND_H

#include <windows.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// State machine states
#define TSS_STATE_WAIT_GL    0
#define TSS_STATE_STABLE_CTX 1
#define TSS_STATE_INIT_TSS   2
#define TSS_STATE_ACTIVE     3
#define STABLE_THRESHOLD     5

// Original SwapBuffers type
typedef BOOL (WINAPI *SwapBuffers_t)(HDC);

// Exported by gl_backend.c
extern SwapBuffers_t g_original_SwapBuffers;
void gl_resolve_all(HMODULE opengl32);
BOOL WINAPI hook_SwapBuffers(HDC hdc);

// Exported by hooks.c
void* install_detour(void* target, void* hook);
void  remove_detour(void* target);

// Exported by loader.c
extern void* g_tss_ctx;
extern int fb_w, fb_h;
extern int g_startup_mode, g_current_mode;
extern volatile int g_state;
extern int g_stable_count;

// TSS function pointers (from loader.c)
typedef void* (*TSS_Init_t)(int w, int h, int mode);
typedef void  (*TSS_Deinit_t)(void* ctx);
typedef void  (*TSS_ProcessFrame_t)(void* ctx, const unsigned char* src, unsigned char* dst);
typedef void  (*TSS_SetMode_t)(void* ctx, int mode);
extern TSS_Init_t TSS_Init_fn;
extern TSS_Deinit_t TSS_Deinit_fn;
extern TSS_ProcessFrame_t TSS_ProcessFrame_fn;
extern TSS_SetMode_t TSS_SetMode_fn;

#ifdef __cplusplus
}
#endif

#endif
