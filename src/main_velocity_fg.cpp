#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/TSSVelocityExtrapolator.h"

#define SCREEN_W 800
#define SCREEN_H 600
#define PHYSICS_HZ 30.0f
#define RENDER_HZ 144.0f
#define MAX_BALLS 20

typedef struct {
    float x, y;
    float radius;
    float r, g, b;
} RenderBall;

RenderBall balls[MAX_BALLS];
int ballCount = 0;
TSSFrameGenerator fg = NULL;
LARGE_INTEGER perfFreq, lastTime, curTime;
float physicsDt = 1.0f / PHYSICS_HZ;
float accumulator = 0.0f;

HWND ghwnd;
HDC ghdc;
BITMAPINFO bmi;
MSG msg;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY || msg == WM_KEYDOWN && wParam == VK_ESCAPE) {
        PostQuitMessage(0);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void InitGDI(int w, int h) {
    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
}

void RenderFrameBuffer(RenderBall* balls, int count) {
    static float buffer[SCREEN_W * SCREEN_H * 4];
    memset(buffer, 0, sizeof(buffer));
    
    for (int i = 0; i < count; i++) {
        RenderBall* b = &balls[i];
        int cx = (int)b->x;
        int cy = (int)b->y;
        int r = (int)b->radius;
        
        for (int py = cy - r - 1; py <= cy + r + 1; py++) {
            for (int px = cx - r - 1; px <= cx + r + 1; px++) {
                if (px < 0 || px >= SCREEN_W || py < 0 || py >= SCREEN_H) continue;
                
                float dx = (float)px - b->x;
                float dy = (float)py - b->y;
                float dist = sqrtf(dx*dx + dy*dy);
                
                if (dist < r) {
                    float coverage = 1.0f - (dist / r);
                    int idx = (py * SCREEN_W + px) * 4;
                    
                    float alpha = coverage * coverage;
                    buffer[idx]     += b->r * alpha;
                    buffer[idx + 1] += b->g * alpha;
                    buffer[idx + 2] += b->b * alpha;
                    buffer[idx + 3] = 255.0f;
                }
            }
        }
    }
    
    StretchDIBits(ghdc, 0, 0, SCREEN_W, SCREEN_H, 0, 0, SCREEN_W, SCREEN_H,
                  buffer, &bmi, DIB_RGB_COLORS, SRCCOPY);
}

void InitWindow(const char* title) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = title;
    RegisterClass(&wc);
    
    ghwnd = CreateWindow(title, title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_W + 16, SCREEN_H + 38,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    ShowWindow(ghwnd, SW_SHOWNORMAL);
    ghdc = GetDC(ghwnd);
    
    InitGDI(SCREEN_W, SCREEN_H);
}

void AddRandomBall() {
    if (ballCount >= MAX_BALLS) return;
    
    RenderBall* b = &balls[ballCount];
    b->x = (float)(100 + rand() % (SCREEN_W - 200));
    b->y = (float)(100 + rand() % (SCREEN_H - 200));
    b->radius = 10.0f + (rand() % 20);
    b->r = (rand() % 200) + 55;
    b->g = (rand() % 200) + 55;
    b->b = (rand() % 200) + 55;
    
    TSSVector2 pos = {b->x, b->y};
    TSSVector2 vel = {
        ((rand() % 400) - 200) * 2.0f,
        ((rand() % 400) - 200) * 2.0f
    };
    
    TSSFGAddEntity(fg, pos, vel, b->radius, (uint8_t)b->r, (uint8_t)b->g, (uint8_t)b->b);
    ballCount++;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    printf("\n========================================\n");
    printf("  TSS Velocity Extrapolation Frame Gen\n");
    printf("========================================\n");
    printf("  Physics: %.0f Hz | Render: %.0f Hz\n", PHYSICS_HZ, RENDER_HZ);
    printf("  Extrapolation: ON | Jitter Comp: ON\n");
    printf("  Latency: %.1f ms\n", 1000.0f / PHYSICS_HZ);
    printf("========================================\n\n");
    printf("Controls:\n");
    printf("  SPACE - Add new ball\n");
    printf("  R     - Reset simulation\n");
    printf("  ESC   - Exit\n\n");
    
    srand((unsigned int)time(NULL));
    QueryPerformanceFrequency(&perfFreq);
    QueryPerformanceCounter(&lastTime);
    
    InitWindow("TSS Velocity Extrapolation - Frame Generator v4.0");
    
    fg = TSSCreateFrameGenerator(MAX_BALLS, PHYSICS_HZ, RENDER_HZ);
    TSSFGSetBoundary(fg, (float)SCREEN_W, (float)SCREEN_H);
    
    for (int i = 0; i < 5; i++) {
        AddRandomBall();
    }
    
    int running = 1;
    int frameCount = 0;
    float fpsTimer = 0;
    float fpsAccum = 0;
    
    while (running) {
        QueryPerformanceCounter(&curTime);
        float deltaTime = (float)(curTime.QuadPart - lastTime.QuadPart) / 
                         (float)perfFreq.QuadPart;
        lastTime = curTime;
        
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        accumulator += deltaTime;
        
        int physicsSteps = 0;
        while (accumulator >= physicsDt && physicsSteps < 5) {
            TSSFGUpdate(fg, physicsDt);
            accumulator -= physicsDt;
            physicsSteps++;
        }
        
        int entityCount = TSSFGGetEntityCount(fg);
        for (int i = 0; i < entityCount && i < MAX_BALLS; i++) {
            TSSEntity* e = TSSFGGetEntity(fg, (uint32_t)i);
            if (e && e->active) {
                balls[i].x = e->predictedPosition.x;
                balls[i].y = e->predictedPosition.y;
            }
        }
        
        RenderFrameBuffer(balls, ballCount);
        
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = 0;
            }
            if (msg.message == WM_KEYDOWN) {
                if (msg.wParam == VK_SPACE) {
                    AddRandomBall();
                }
                if (msg.wParam == 0x52) {
                    TSSDestroyFrameGenerator(fg);
                    fg = TSSCreateFrameGenerator(MAX_BALLS, PHYSICS_HZ, RENDER_HZ);
                    TSSFGSetBoundary(fg, (float)SCREEN_W, (float)SCREEN_H);
                    ballCount = 0;
                    for (int i = 0; i < 5; i++) AddRandomBall();
                }
                if (msg.wParam == VK_ESCAPE) {
                    running = 0;
                }
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        frameCount++;
        fpsAccum += deltaTime;
        
        if (fpsAccum >= 1.0f) {
            float fps = (float)frameCount / fpsAccum;
            int entCount = TSSFGGetEntityCount(fg);
            TSSEntity* firstEnt = (entCount > 0) ? TSSFGGetEntity(fg, 0) : NULL;
            float speed = firstEnt ? TSSCalculateVelocityMagnitude(firstEnt->velocity) : 0;
            printf("\rFPS: %6.1f | Physics: %2d Hz | Balls: %2d | Speed: %6.1f px/s    ",
                   fps, physicsSteps, ballCount, speed);
            fflush(stdout);
            frameCount = 0;
            fpsAccum = 0;
        }
        
        float targetFrame = 1.0f / RENDER_HZ;
        float sleepTime = targetFrame - deltaTime;
        if (sleepTime > 0.001f) {
            Sleep((DWORD)(sleepTime * 900));
        }
    }
    
    if (fg) TSSDestroyFrameGenerator(fg);
    ReleaseDC(ghwnd, ghdc);
    
    return 0;
}
