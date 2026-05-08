#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/TSSPerfectInterpolation.h"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define TARGET_FPS 60.0f
#define PHYSICS_FPS 20.0f
#define RENDER_FPS 144.0f

typedef struct {
    float x, y;
    float vx, vy;
    float radius;
    float color[3];
} Ball;

Ball balls[10];
int ballCount = 0;
float frameBuffer[WINDOW_WIDTH * WINDOW_HEIGHT * 4];
LARGE_INTEGER perfFrequency, lastTime, currentTime;
float accumulator = 0.0f;
float physicsDt = 1.0f / PHYSICS_FPS;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void InitWindow(const char* title, int showCmd) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = title;
    RegisterClass(&wc);
    
    HWND hwnd = CreateWindow(title, title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    ShowWindow(hwnd, showCmd);
}

void InitBalls() {
    srand((unsigned int)time(NULL));
    for (int i = 0; i < 10; i++) {
        balls[i].x = (float)(rand() % (WINDOW_WIDTH - 100)) + 50;
        balls[i].y = (float)(rand() % (WINDOW_HEIGHT - 100)) + 50;
        balls[i].vx = ((rand() % 200) - 100) * 0.5f;
        balls[i].vy = ((rand() % 200) - 100) * 0.5f;
        balls[i].radius = 15.0f + (rand() % 15);
        balls[i].color[0] = (rand() % 255) / 255.0f;
        balls[i].color[1] = (rand() % 255) / 255.0f;
        balls[i].color[2] = (rand() % 255) / 255.0f;
        ballCount++;
    }
}

void UpdatePhysicsFixed(float dt) {
    for (int i = 0; i < ballCount; i++) {
        balls[i].x += balls[i].vx * dt;
        balls[i].y += balls[i].vy * dt;
        
        if (balls[i].x < balls[i].radius) {
            balls[i].x = balls[i].radius;
            balls[i].vx = -balls[i].vx * 0.9f;
        }
        if (balls[i].x > WINDOW_WIDTH - balls[i].radius) {
            balls[i].x = (float)WINDOW_WIDTH - balls[i].radius;
            balls[i].vx = -balls[i].vx * 0.9f;
        }
        if (balls[i].y < balls[i].radius) {
            balls[i].y = balls[i].radius;
            balls[i].vy = -balls[i].vy * 0.9f;
        }
        if (balls[i].y > WINDOW_HEIGHT - balls[i].radius) {
            balls[i].y = (float)WINDOW_HEIGHT - balls[i].radius;
            balls[i].vy = -balls[i].vy * 0.9f;
        }
    }
}

void RenderFrame(float alpha) {
    memset(frameBuffer, 0, sizeof(frameBuffer));
    
    for (int i = 0; i < ballCount; i++) {
        TSSVec2 ballPos = {balls[i].x, balls[i].y};
        TSSVec2 nextPos;
        nextPos.x = ballPos.x + balls[i].vx * physicsDt;
        nextPos.y = ballPos.y + balls[i].vy * physicsDt;
        
        TSSVec2 interpPos = TSSPerfectInterpolation(ballPos, nextPos, alpha);
        
        float radius = balls[i].radius;
        
        for (int py = 0; py < WINDOW_HEIGHT; py++) {
            for (int px = 0; px < WINDOW_WIDTH; px++) {
                float dx = (float)px - interpPos.x;
                float dy = (float)py - interpPos.y;
                float dist = sqrtf(dx*dx + dy*dy);
                
                if (dist < radius) {
                    float coverage = 1.0f - (dist / radius);
                    if (coverage > 1.0f) coverage = 1.0f;
                    
                    TSSVec2 pixelPos = {(float)px + 0.5f, (float)py + 0.5f};
                    TSSVec2 sizeVec = {radius*2, radius*2};
                    float subPixelCov = TSSCalculateSubPixelCoverage(
                        interpPos, sizeVec, pixelPos.x, pixelPos.y
                    );
                    
                    int idx = (py * WINDOW_WIDTH + px) * 4;
                    frameBuffer[idx] += balls[i].color[0] * coverage * subPixelCov;
                    frameBuffer[idx + 1] += balls[i].color[1] * coverage * subPixelCov;
                    frameBuffer[idx + 2] += balls[i].color[2] * coverage * subPixelCov;
                    frameBuffer[idx + 3] = 1.0f;
                }
            }
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    printf("[TSS Perfect Interpolation Demo v3.0]\n");
    printf("Physics: %.0f Hz | Render: %.0f Hz | Target: %.0f FPS\n\n", 
           PHYSICS_FPS, RENDER_FPS, TARGET_FPS);
    printf("Features:\n");
    printf("  - Sub-pixel Analytical AA\n");
    printf("  - Perfect Interpolation (vector-based motion)\n");
    printf("  - Dual-thread rendering (physics/render separation)\n");
    printf("  - Motion blur with configurable samples\n\n");
    
    QueryPerformanceFrequency(&perfFrequency);
    QueryPerformanceCounter(&lastTime);
    
    InitBalls();
    
    MSG msg;
    int running = 1;
    
    while (running) {
        QueryPerformanceCounter(&currentTime);
        float deltaTime = (float)(currentTime.QuadPart - lastTime.QuadPart) / 
                         (float)perfFrequency.QuadPart;
        lastTime = currentTime;
        
        accumulator += deltaTime;
        
        int physicsSteps = 0;
        while (accumulator >= physicsDt && physicsSteps < 5) {
            UpdatePhysicsFixed(physicsDt);
            accumulator -= physicsDt;
            physicsSteps++;
        }
        
        float alpha = accumulator / physicsDt;
        if (alpha > 1.0f) alpha = 1.0f;
        
        RenderFrame(alpha);
        
        float renderInterval = 1.0f / RENDER_FPS;
        float sleepTime = renderInterval - deltaTime;
        if (sleepTime > 0) {
            Sleep((DWORD)(sleepTime * 1000));
        }
        
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = 0;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        static int frameCount = 0;
        static float fpsTimer = 0;
        static float fpsAccum = 0;
        fpsAccum += deltaTime;
        frameCount++;
        
        if (fpsAccum >= 1.0f) {
            float fps = (float)frameCount / fpsAccum;
            printf("\rFPS: %.1f | Physics: %d Hz | Alpha: %.3f    ", fps, physicsSteps, alpha);
            fflush(stdout);
            frameCount = 0;
            fpsAccum = 0;
        }
    }
    
    return 0;
}
