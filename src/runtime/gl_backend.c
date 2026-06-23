#include "gl_backend.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// GL types
typedef int GLint;
typedef unsigned int GLenum;
typedef unsigned int GLsizei;
typedef void GLvoid;
typedef double GLdouble;
typedef HGLRC (WINAPI *wglGetCurrentContext_t)(void);

typedef BOOL (WINAPI *wglMakeCurrent_t)(HDC, HGLRC);

// OpenGL function pointers (static to gl_backend)
static void (*glReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*) = NULL;
static void (*glDrawPixels)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid*) = NULL;
static void (*glRasterPos2i)(GLint, GLint) = NULL;
static void (*glGetIntegerv)(GLenum, GLint*) = NULL;
static void (*glPixelStorei)(GLenum, GLint) = NULL;
static void (*glOrtho)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble) = NULL;
static void (*glMatrixMode)(GLenum) = NULL;
static void (*glPushMatrix)(void) = NULL;
static void (*glPopMatrix)(void) = NULL;
static void (*glViewport)(GLint, GLint, GLsizei, GLsizei) = NULL;
static void (*glLoadIdentity)(void) = NULL;
static wglGetCurrentContext_t wglGetCurrentContext_fn = NULL;

#define GL_RGBA             0x1908
#define GL_UNSIGNED_BYTE    0x1401
#define GL_VIEWPORT         0x0BA2
#define GL_PACK_ALIGNMENT   0x0D05
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_MATRIX_MODE      0x0BA0
#define GL_PROJECTION       0x1701
#define GL_MODELVIEW        0x1700

SwapBuffers_t g_original_SwapBuffers = NULL;

static BOOL last_f1 = FALSE, last_f2 = FALSE, last_f3 = FALSE, last_f4 = FALSE;

void gl_resolve_all(HMODULE opengl32) {
    glReadPixels = (void*)GetProcAddress(opengl32, "glReadPixels");
    glDrawPixels = (void*)GetProcAddress(opengl32, "glDrawPixels");
    glRasterPos2i = (void*)GetProcAddress(opengl32, "glRasterPos2i");
    glGetIntegerv = (void*)GetProcAddress(opengl32, "glGetIntegerv");
    glPixelStorei = (void*)GetProcAddress(opengl32, "glPixelStorei");
    glOrtho = (void*)GetProcAddress(opengl32, "glOrtho");
    glMatrixMode = (void*)GetProcAddress(opengl32, "glMatrixMode");
    glPushMatrix = (void*)GetProcAddress(opengl32, "glPushMatrix");
    glPopMatrix = (void*)GetProcAddress(opengl32, "glPopMatrix");
    glViewport = (void*)GetProcAddress(opengl32, "glViewport");
    glLoadIdentity = (void*)GetProcAddress(opengl32, "glLoadIdentity");
    wglGetCurrentContext_fn = (void*)GetProcAddress(opengl32, "wglGetCurrentContext");
}

static void read_tss_ini(void) {
    FILE *f = fopen("tss.ini", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (strncmp(p, "mode", 4) == 0) {
            p += 4;
            while (*p == ' ' || *p == '\t' || *p == '=') p++;
            if (strcmp(p, "off\r\n") == 0 || strcmp(p, "off\n") == 0) g_startup_mode = 0;
            else if (strcmp(p, "easu\r\n") == 0 || strcmp(p, "easu\n") == 0) g_startup_mode = 1;
            else if (strcmp(p, "easu_rcas\r\n") == 0 || strcmp(p, "easu_rcas\n") == 0) g_startup_mode = 2;
            else if (strcmp(p, "debug\r\n") == 0 || strcmp(p, "debug\n") == 0) g_startup_mode = 3;
        }
    }
    fclose(f);
}

static void downsample_bilinear(const unsigned char* src, int src_w, int src_h,
                                 unsigned char* dst, int dst_w, int dst_h) {
    int x, y, c;
    for (y = 0; y < dst_h; y++) {
        float sy = (y + 0.5f) * src_h / (float)dst_h - 0.5f;
        if (sy < 0) sy = 0;
        if (sy > src_h - 1) sy = (float)(src_h - 1);
        int sy0 = (int)sy, sy1 = sy0 + 1 < src_h ? sy0 + 1 : sy0;
        float fy = sy - (float)sy0;
        for (x = 0; x < dst_w; x++) {
            float sx = (x + 0.5f) * src_w / (float)dst_w - 0.5f;
            if (sx < 0) sx = 0;
            if (sx > src_w - 1) sx = (float)(src_w - 1);
            int sx0 = (int)sx, sx1 = sx0 + 1 < src_w ? sx0 + 1 : sx0;
            float fx = sx - (float)sx0;
            for (c = 0; c < 4; c++) {
                float v = (1-fy)*((1-fx)*src[(sy0*src_w+sx0)*4+c] + fx*src[(sy0*src_w+sx1)*4+c])
                        +    fy *((1-fx)*src[(sy1*src_w+sx0)*4+c] + fx*src[(sy1*src_w+sx1)*4+c]);
                dst[(y*dst_w+x)*4+c] = (unsigned char)(v + 0.5f);
            }
        }
    }
}

static void handle_keys(void) {
    SHORT (WINAPI *GetAsyncKeyState)(int) = NULL;
    HMODULE user32 = GetModuleHandleA("user32.dll");
    if (user32) GetAsyncKeyState = (void*)GetProcAddress(user32, "GetAsyncKeyState");
    if (!GetAsyncKeyState) return;
    BOOL f1 = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
    BOOL f2 = (GetAsyncKeyState(VK_F2) & 0x8000) != 0;
    BOOL f3 = (GetAsyncKeyState(VK_F3) & 0x8000) != 0;
    BOOL f4 = (GetAsyncKeyState(VK_F4) & 0x8000) != 0;
    if (f1 && !last_f1 && TSS_SetMode_fn && g_tss_ctx) { TSS_SetMode_fn(g_tss_ctx, 0); g_current_mode = 0; }
    if (f2 && !last_f2 && TSS_SetMode_fn && g_tss_ctx) { TSS_SetMode_fn(g_tss_ctx, 1); g_current_mode = 1; }
    if (f3 && !last_f3 && TSS_SetMode_fn && g_tss_ctx) { TSS_SetMode_fn(g_tss_ctx, 2); g_current_mode = 2; }
    if (f4 && !last_f4 && TSS_SetMode_fn && g_tss_ctx) { TSS_SetMode_fn(g_tss_ctx, 3); g_current_mode = 3; }
    last_f1 = f1; last_f2 = f2; last_f3 = f3; last_f4 = f4;
}

BOOL WINAPI hook_SwapBuffers(HDC hdc) {
    handle_keys();

    switch (g_state) {
        case TSS_STATE_WAIT_GL:
            if (wglGetCurrentContext_fn && wglGetCurrentContext_fn()) {
                g_state = TSS_STATE_STABLE_CTX;
                g_stable_count = 0;
            }
            break;

        case TSS_STATE_STABLE_CTX:
            if (wglGetCurrentContext_fn && wglGetCurrentContext_fn()) {
                if (glGetIntegerv) {
                    GLint vp[4];
                    glGetIntegerv(GL_VIEWPORT, vp);
                    if (vp[2] >= 320 && vp[3] >= 240) {
                        fb_w = vp[2]; fb_h = vp[3];
                        g_stable_count++;
                        if (g_stable_count >= STABLE_THRESHOLD) {
                            g_state = TSS_STATE_INIT_TSS;
                        }
                    } else {
                        g_stable_count = 0;
                    }
                }
            } else {
                g_state = TSS_STATE_WAIT_GL;
                g_stable_count = 0;
            }
            break;

        case TSS_STATE_INIT_TSS: {
            static BOOL ini_loaded = FALSE;
            if (!ini_loaded) { read_tss_ini(); ini_loaded = TRUE; }

            if (TSS_Init_fn && fb_w >= 320 && fb_h >= 240) {
                g_tss_ctx = TSS_Init_fn(fb_w, fb_h, g_startup_mode);
                g_current_mode = g_startup_mode;
            }

            static BOOL msg_shown = FALSE;
            if (!msg_shown && fb_w >= 320 && fb_h >= 240) {
                char buf[512];
                const char* mode_names[] = {"OFF", "EASU", "EASU+RCAS", "DEBUG"};
                const char* mode_name = mode_names[g_startup_mode < 4 ? g_startup_mode : 3];
                _snprintf(buf, sizeof(buf),
                    "TSS Loader injected via launcher\n\n"
                    "Resolution: %d x %d\nTSS: %s\nMode: %s\n\n"
                    "F1=Off  F2=EASU  F3=RCAS  F4=Debug",
                    fb_w, fb_h,
                    (g_tss_ctx && TSS_ProcessFrame_fn) ? "YES" : "NO",
                    mode_name);
                MessageBoxA(NULL, buf, "TSS Loader", MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
                msg_shown = TRUE;
            }

            g_state = TSS_STATE_ACTIVE;
            break;
        }

        case TSS_STATE_ACTIVE:
            if (!wglGetCurrentContext_fn || !wglGetCurrentContext_fn()) {
                g_state = TSS_STATE_WAIT_GL;
                g_stable_count = 0;
                break;
            }
            if (glGetIntegerv) {
                GLint vp[4];
                glGetIntegerv(GL_VIEWPORT, vp);
                if (vp[2] >= 320 && vp[3] >= 240) {
                    if (vp[2] != fb_w || vp[3] != fb_h) {
                        fb_w = vp[2]; fb_h = vp[3];
                        if (g_tss_ctx && TSS_Deinit_fn) { TSS_Deinit_fn(g_tss_ctx); g_tss_ctx = NULL; }
                        if (TSS_Init_fn) g_tss_ctx = TSS_Init_fn(fb_w, fb_h, g_current_mode);
                    }
                } else {
                    fb_w = 0; fb_h = 0;
                }
            }
            if (g_tss_ctx && TSS_ProcessFrame_fn && glReadPixels && glDrawPixels && fb_w >= 320 && fb_h >= 240) {
                static unsigned char* rgba_buf = NULL;
                static unsigned char* tss_out = NULL;
                static int rgba_cap = 0, tss_cap = 0;
                int rgba_needed = fb_w * fb_h * 4;
                int tss_needed = fb_w * fb_h * 4 * 4;
                if (!rgba_buf || rgba_cap < rgba_needed) {
                    if (rgba_buf) free(rgba_buf);
                    rgba_buf = malloc(rgba_needed); rgba_cap = rgba_needed;
                    if (rgba_buf) memset(rgba_buf, 0, rgba_needed);
                }
                if (!tss_out || tss_cap < tss_needed) {
                    if (tss_out) free(tss_out);
                    tss_out = malloc(tss_needed); tss_cap = tss_needed;
                    if (tss_out) memset(tss_out, 0, tss_needed);
                }
                if (!rgba_buf || !tss_out) goto swap;
                if (glPixelStorei) glPixelStorei(GL_PACK_ALIGNMENT, 1);
                glReadPixels(0, 0, fb_w, fb_h, GL_RGBA, GL_UNSIGNED_BYTE, rgba_buf);
                TSS_ProcessFrame_fn(g_tss_ctx, rgba_buf, tss_out);
                downsample_bilinear(tss_out, fb_w * 2, fb_h * 2, rgba_buf, fb_w, fb_h);
                {
                    unsigned char mc[4]; const char* labels = "OG\0\0GR\0\0CY\0\0RE";
                    int idx = g_current_mode < 4 ? g_current_mode : 3;
                    mc[0] = (unsigned char)labels[idx*3]; mc[1] = (unsigned char)labels[idx*3+1];
                    mc[2] = 0; mc[3] = 255;
                    unsigned char* top_row = rgba_buf + (fb_h - 1) * fb_w * 4;
                    for (int i = 0; i < 20 && i < fb_w; i++) {
                        top_row[i*4+0] = mc[0]; top_row[i*4+1] = mc[1];
                        top_row[i*4+2] = mc[2]; top_row[i*4+3] = 255;
                    }
                }
                if (glPixelStorei) glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                GLint saved_viewport[4], saved_matrix_mode;
                glGetIntegerv(GL_VIEWPORT, saved_viewport);
                glGetIntegerv(GL_MATRIX_MODE, &saved_matrix_mode);
                glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
                glOrtho(0, (GLdouble)fb_w, 0, (GLdouble)fb_h, -1, 1);
                glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
                glViewport(0, 0, fb_w, fb_h);
                if (glRasterPos2i) glRasterPos2i(0, 0);
                glDrawPixels(fb_w, fb_h, GL_RGBA, GL_UNSIGNED_BYTE, rgba_buf);
                glMatrixMode(GL_PROJECTION); glPopMatrix();
                glMatrixMode(GL_MODELVIEW); glPopMatrix();
                glViewport(saved_viewport[0], saved_viewport[1], saved_viewport[2], saved_viewport[3]);
                if (saved_matrix_mode != GL_PROJECTION && saved_matrix_mode != GL_MODELVIEW)
                    glMatrixMode((GLenum)saved_matrix_mode);
            }
            break;
    }

swap:
    return g_original_SwapBuffers ? g_original_SwapBuffers(hdc) : FALSE;
}
