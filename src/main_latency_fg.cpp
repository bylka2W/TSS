#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "TSSInputLatency.h"
#include "TSSLowLatency.h"
#include "TSSTransform3D.h"

#define TARGET_FPS 144.0f
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

typedef struct {
    float posX, posY, posZ;
    float yaw, pitch;
    float velX, velY;
} CameraState;

static const char* g_windowClass = "TSSLowLatencyDemo";
static HWND g_hwnd = NULL;
static HDC g_hdc = NULL;
static TSSInputLatency* g_input = NULL;
static TSSLowLatency* g_latency = NULL;
static CameraState g_camera = {0};
static bool g_running = true;
static int g_mouseX = WINDOW_WIDTH / 2, g_mouseY = WINDOW_HEIGHT / 2;
static bool g_mouseCaptured = false;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
            g_running = false;
            return 0;
        case WM_DESTROY:
            g_running = false;
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                g_mouseCaptured = false;
                ClipCursor(NULL);
                ShowCursor(TRUE);
            }
            return 0;
        case WM_INPUT: {
            UINT dwSize = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            static BYTE lpb[sizeof(RAWINPUT)];
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
                RAWINPUT* raw = (RAWINPUT*)lpb;
                if (raw->header.dwType == RIM_TYPEMOUSE) {
                    float dx = (float)raw->data.mouse.lLastX;
                    float dy = (float)raw->data.mouse.lLastY;
                    uint64_t now = TSSInputGetTimeMicroseconds();
                    TSSInputPushMouseEvent(g_input, dx, dy, 0, now);
                }
            }
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void RegisterWindow(void) {
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = g_windowClass;
    RegisterClassEx(&wc);
}

static void CreateWindow(void) {
    g_hwnd = CreateWindowEx(
        0, g_windowClass, "TSS Low Latency FG Demo",
        WS_OVERLAPPEDWINDOW,
        100, 100, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    ShowWindow(g_hwnd, SW_SHOW);
    g_hdc = GetDC(g_hwnd);
}

static void RenderFrame(void) {
    static int frameCount = 0;
    frameCount++;
    
    float clearColor[3] = {0.05f, 0.05f, 0.08f};
    if (g_mouseCaptured) {
        float pulse = 0.5f + 0.5f * sinf((float)frameCount * 0.05f);
        clearColor[0] = 0.1f + pulse * 0.1f;
    }
    
    SelectObject(g_hdc, GetStockObject(DC_BRUSH));
    SetDCBrushColor(g_hdc, RGB(
        (int)(clearColor[0] * 255),
        (int)(clearColor[1] * 255),
        (int)(clearColor[2] * 255)
    ));
    
    RECT rect;
    GetClientRect(g_hwnd, &rect);
    Rectangle(g_hdc, 0, 0, rect.right, rect.bottom);
    
    SetBkMode(g_hdc, TRANSPARENT);
    SetTextColor(g_hdc, RGB(200, 200, 200));
    
    char debug[512];
    int y = 20;
    
    snprintf(debug, sizeof(debug), "TSS Low-Latency Frame Generation Demo");
    TextOut(g_hdc, 20, y, debug, strlen(debug)); y += 25;
    
    snprintf(debug, sizeof(debug), "Target FPS: %.1f", TARGET_FPS);
    TextOut(g_hdc, 20, y, debug, strlen(debug)); y += 20;
    
    float inputLag = TSSLowLatencyGetInputLag(g_latency);
    snprintf(debug, sizeof(debug), "Estimated Input Lag: %.2f ms", inputLag);
    TextOut(g_hdc, 20, y, debug, strlen(debug)); y += 20;
    
    float progress = TSSLowLatencyGetFrameProgress(g_latency);
    snprintf(debug, sizeof(debug), "Frame Progress: %.1f%%", progress * 100.0f);
    TextOut(g_hdc, 20, y, debug, strlen(debug)); y += 20;
    
    float deltaTime = g_latency->deltaTime_ms;
    snprintf(debug, sizeof(debug), "Frame Time: %.2f ms", deltaTime);
    TextOut(g_hdc, 20, y, debug, strlen(debug)); y += 20;
    
    float camX = g_camera.posX;
    float camY = g_camera.posY;
    snprintf(debug, sizeof(debug), "Camera: (%.2f, %.2f)", camX, camY);
    TextOut(g_hdc, 20, y, debug, strlen(debug)); y += 20;
    
    snprintf(debug, sizeof(debug), "Yaw: %.2f, Pitch: %.2f", g_camera.yaw, g_camera.pitch);
    TextOut(g_hdc, 20, y, debug, strlen(debug)); y += 20;
    
    y += 20;
    snprintf(debug, sizeof(debug), "Controls:");
    TextOut(g_hdc, 20, y, debug, strlen(debug)); y += 20;
    snprintf(debug, sizeof(debug), "  Click to capture mouse");
    TextOut(g_hdc, 20, y, debug, strlen(debug)); y += 20;
    snprintf(debug, sizeof(debug), "  ESC to release mouse");
    TextOut(g_hdc, 20, y, debug, strlen(debug)); y += 20;
    
    int crossX = WINDOW_WIDTH / 2;
    int crossY = WINDOW_HEIGHT / 2;
    HPEN greenPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
    HPEN oldPen = (HPEN)SelectObject(g_hdc, greenPen);
    
    MoveToEx(g_hdc, crossX - 20, crossY, NULL);
    LineTo(g_hdc, crossX + 20, crossY);
    MoveToEx(g_hdc, crossX, crossY - 20, NULL);
    LineTo(g_hdc, crossX, crossY + 20);
    
    SelectObject(g_hdc, oldPen);
    DeleteObject(greenPen);
}

static void UpdateCamera(void) {
    TSSVec2 yawPitch = {g_camera.yaw, g_camera.pitch};
    
    TSSVec2 predicted = TSSInputGetPredictedCamera(g_input, yawPitch, g_latency->deltaTime_ms);
    
    TSSVec2 extrap = TSSInputGetExtrapolatedCamera(g_input, yawPitch, g_latency->deltaTime_ms);
    
    if (g_input->enableExtrap) {
        g_camera.yaw = extrap.x;
        g_camera.pitch = extrap.y;
    } else {
        g_camera.yaw = predicted.x;
        g_camera.pitch = predicted.y;
    }
    
    float moveSpeed = 5.0f;
    float sinYaw = sinf(g_camera.yaw * (float)M_PI / 180.0f);
    float cosYaw = cosf(g_camera.yaw * (float)M_PI / 180.0f);
    
    g_camera.posX += g_camera.velX * cosYaw * 0.016f;
    g_camera.posZ += g_camera.velX * sinYaw * 0.016f;
    g_camera.posX -= g_camera.velY * sinYaw * 0.016f;
    g_camera.posZ += g_camera.velY * cosYaw * 0.016f;
}

static MSG msg;
static void ProcessMessages(void) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            g_running = false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    
    RegisterWindow();
    CreateWindow();
    
    g_input = TSSInputCreate();
    g_latency = TSSLowLatencyCreate();
    
    TSSInputSetSensitivity(g_input, 0.15f);
    TSSInputSetExtrapolation(g_input, true, 0.3f);
    TSSInputSetSmoothing(g_input, true, 0.1f);
    
    TSSLowLatencySetMode(g_latency, TSS_LATENCY_MODE_LOW);
    TSSLowLatencySetRefreshRate(g_latency, 144.0f);
    TSSLowLatencySetVSync(g_latency, false);
    TSSLowLatencySetQueueDepth(g_latency, TSS_RENDER_QUEUE_DEPTH);
    
    RAWINPUTDEVICE rid[1];
    rid[0].usUsagePage = 0x01;
    rid[0].usUsage = 0x02;
    rid[0].dwFlags = RIDEVT_NOLEGACY;
    rid[0].hwndTarget = g_hwnd;
    RegisterRawInputDevices(rid, 1, sizeof(rid[0]));
    
    uint64_t frameCount = 0;
    uint64_t startTime = TSSGetTimeMicroseconds();
    uint64_t lastFPSUpdate = startTime;
    int currentFPS = 0;
    
    while (g_running) {
        ProcessMessages();
        
        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            if (!g_mouseCaptured) {
                g_mouseCaptured = true;
                ShowCursor(FALSE);
            }
        }
        
        TSSLowLatencyBeginFrame(g_latency);
        TSSInputBeginFrame(g_input);
        
        UpdateCamera();
        
        TSSLowLatencyWaitForRenderTime(g_latency);
        TSSLowLatencyBeginGPUWork(g_latency);
        
        RenderFrame();
        
        TSSLowLatencyEndGPUWork(g_latency);
        TSSLowLatencySignalFrameReady(g_latency);
        TSSLowLatencyEndFrame(g_latency);
        TSSInputEndFrame(g_input);
        
        frameCount++;
        uint64_t now = TSSGetTimeMicroseconds();
        if (now - lastFPSUpdate >= 1000000) {
            currentFPS = (int)(frameCount * 1000000.0 / (now - lastFPSUpdate));
            lastFPSUpdate = now;
            frameCount = 0;
        }
        
        float targetFrameTime = 1000000.0f / TARGET_FPS;
        uint64_t frameEnd = TSSGetTimeMicroseconds();
        if (frameEnd - now < targetFrameTime) {
            TSSLowLatencySpinYield();
        }
    }
    
    TSSInputDestroy(g_input);
    TSSLowLatencyDestroy(g_latency);
    
    ReleaseDC(g_hwnd, g_hdc);
    DestroyWindow(g_hwnd);
    UnregisterClass(g_windowClass, GetModuleHandle(NULL));
    
    return 0;
}
