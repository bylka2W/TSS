#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/TSSTransform3D.h"
#include "../include/TSSCollision3D.h"
#include "../include/TSSSOAEntities.h"

#define SCREEN_W 800
#define SCREEN_H 600
#define PHYSICS_HZ 20.0f
#define RENDER_HZ 144.0f
#define MOVE_SPEED 5.0f
#define MOUSE_SENSITIVITY 0.002f

#pragma warning(disable:4204)

static float buffer[800 * 600 * 4];
static int keys[256];
static int mouseLocked = 0;
static POINT mouseCenter;
static TSSSOAWorld* world = NULL;
static TSSCamera3D* camera = NULL;
static TSSVec3 playerPos = {0, 1.6f, 5};
static TSSQuat playerRot = {0, 0, 0, 1};
static TSSVec3 playerVel = {0, 0, 0};
static float moveForward = 0, moveRight = 0, moveUp = 0;
static LARGE_INTEGER perfFreq, lastTime;
static float physicsDt = 1.0f / PHYSICS_HZ;
static float accumulator = 0.0f;
static HWND ghwnd;
static HDC ghdc;
static BITMAPINFO bmi;
static MSG msg;

void InitGDI(int w, int h) {
    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
}

void InitScene(void) {
    TSSBoundingBox box;
    
    box.center.x = 0; box.center.y = 0; box.center.z = -10;
    box.halfExtents.x = 15; box.halfExtents.y = 5; box.halfExtents.z = 1;
    TSSSOAAddBox(world, box);
    
    box.center.x = 0; box.center.y = 0; box.center.z = 20;
    box.halfExtents.x = 15; box.halfExtents.y = 5; box.halfExtents.z = 1;
    TSSSOAAddBox(world, box);
    
    box.center.x = -10; box.center.y = 0; box.center.z = 5;
    box.halfExtents.x = 1; box.halfExtents.y = 5; box.halfExtents.z = 15;
    TSSSOAAddBox(world, box);
    
    box.center.x = 10; box.center.y = 0; box.center.z = 5;
    box.halfExtents.x = 1; box.halfExtents.y = 5; box.halfExtents.z = 15;
    TSSSOAAddBox(world, box);
    
    box.center.x = 3; box.center.y = 0; box.center.z = 5;
    box.halfExtents.x = 2; box.halfExtents.y = 3; box.halfExtents.z = 2;
    TSSSOAAddBox(world, box);
    
    box.center.x = -3; box.center.y = 0; box.center.z = 10;
    box.halfExtents.x = 1; box.halfExtents.y = 1; box.halfExtents.z = 1;
    TSSSOAAddBox(world, box);
}

void RenderFloor(void) {
    int gridSize = 20;
    float gridSpacing = 2.0f;
    
    int x, z;
    for (z = -gridSize; z <= gridSize; z++) {
        for (x = -gridSize; x <= gridSize; x++) {
            float fx = (float)x * gridSpacing;
            float fz = (float)z * gridSpacing;
            
            float brightness = ((x + z) % 2 == 0) ? 0.3f : 0.2f;
            
            int minX = (int)(fx * 50 + 400);
            int maxX = (int)((fx + gridSpacing) * 50 + 400);
            int minY = (int)(-fz * 50 + 300);
            int maxY = (int)(-(fz + gridSpacing) * 50 + 300);
            
            if (minX < 0) minX = 0;
            if (maxX > 800) maxX = 800;
            if (minY < 0) minY = 0;
            if (maxY > 600) maxY = 600;
            
            int py, px;
            for (py = minY; py <= maxY; py++) {
                for (px = minX; px <= maxX; px++) {
                    int idx = (py * 800 + px) * 4;
                    buffer[idx] = brightness * 100;
                    buffer[idx + 1] = brightness * 100;
                    buffer[idx + 2] = brightness * 120;
                    buffer[idx + 3] = 255;
                }
            }
        }
    }
}

void RenderBox(TSSBoundingBox box, float r, float g, float b) {
    TSSVec3 minB = TSSVec3_Sub(box.center, box.halfExtents);
    TSSVec3 maxB = TSSVec3_Add(box.center, box.halfExtents);
    
    TSSVec3 p0 = minB;
    TSSVec3 p1; p1.x = maxB.x; p1.y = minB.y; p1.z = minB.z;
    TSSVec3 p2; p2.x = maxB.x; p2.y = maxB.y; p2.z = minB.z;
    TSSVec3 p3; p3.x = minB.x; p3.y = maxB.y; p3.z = minB.z;
    TSSVec3 p4; p4.x = minB.x; p4.y = minB.y; p4.z = maxB.z;
    TSSVec3 p5; p5.x = maxB.x; p5.y = minB.y; p5.z = maxB.z;
    TSSVec3 p6 = maxB;
    TSSVec3 p7; p7.x = minB.x; p7.y = maxB.y; p7.z = maxB.z;
    
    TSSVec3 corners[8] = {p0, p1, p2, p3, p4, p5, p6, p7};
    
    int indices[36] = {
        0,1,2, 0,2,3,
        4,6,5, 4,7,6,
        0,4,5, 0,5,1,
        2,6,7, 2,7,3,
        0,3,7, 0,7,4,
        1,5,6, 1,6,2
    };
    
    TSSVec3 projected[8];
    int i;
    for (i = 0; i < 8; i++) {
        TSSVec3 rel = TSSVec3_Sub(corners[i], playerPos);
        
        TSSVec3 euler = TSSQuat_ToEuler(playerRot);
        float cosY = cosf(-euler.y);
        float sinY = sinf(-euler.y);
        float cosX = cosf(-euler.x);
        float sinX = sinf(-euler.x);
        
        float rx = rel.x * cosY - rel.z * sinY;
        float rz = rel.x * sinY + rel.z * cosY;
        float ry = rel.y;
        
        float tx = rx * cosX + rz * sinX;
        float tz = -rx * sinX + rz * cosX;
        
        if (tz > -0.1f) {
            projected[i].x = -9999;
            continue;
        }
        
        projected[i].x = (tx / tz) * 300.0f + 400.0f;
        projected[i].y = -(ry / tz) * 300.0f + 300.0f;
        projected[i].z = tz;
    }
    
    for (i = 0; i < 36; i++) {
        int i0 = indices[i * 3];
        int i1 = indices[i * 3 + 1];
        int i2 = indices[i * 3 + 2];
        
        if (projected[i0].x < -9000 || projected[i1].x < -9000 || projected[i2].x < -9000) continue;
        
        float minX = projected[i0].x;
        if (projected[i1].x < minX) minX = projected[i1].x;
        if (projected[i2].x < minX) minX = projected[i2].x;
        float maxX = projected[i0].x;
        if (projected[i1].x > maxX) maxX = projected[i1].x;
        if (projected[i2].x > maxX) maxX = projected[i2].x;
        float minY = projected[i0].y;
        if (projected[i1].y < minY) minY = projected[i1].y;
        if (projected[i2].y < minY) minY = projected[i2].y;
        float maxY = projected[i0].y;
        if (projected[i1].y > maxY) maxY = projected[i1].y;
        if (projected[i2].y > maxY) maxY = projected[i2].y;
        
        int px, py;
        int pxMin = (int)((minX > 0) ? minX : 0);
        int pxMax = (int)((maxX < 799) ? maxX : 799);
        int pyMin = (int)((minY > 0) ? minY : 0);
        int pyMax = (int)((maxY < 599) ? maxY : 599);
        
        for (py = pyMin; py <= pyMax; py++) {
            for (px = pxMin; px <= pxMax; px++) {
                int idx = (py * 800 + px) * 4;
                buffer[idx] = r * 200;
                buffer[idx + 1] = g * 200;
                buffer[idx + 2] = b * 200;
                buffer[idx + 3] = 255;
            }
        }
    }
}

void UpdatePlayer(float dt) {
    TSSVec3 eulerY = TSSQuat_ToEuler(playerRot);
    
    TSSVec3 forward;
    forward.x = sinf(eulerY.y);
    forward.y = 0;
    forward.z = -cosf(eulerY.y);
    
    TSSVec3 right;
    right.x = cosf(eulerY.y);
    right.y = 0;
    right.z = sinf(eulerY.y);
    
    TSSVec3 moveDir;
    moveDir.x = forward.x * moveForward + right.x * moveRight;
    moveDir.y = moveUp;
    moveDir.z = forward.z * moveForward + right.z * moveRight;
    
    float moveLen = sqrtf(moveDir.x*moveDir.x + moveDir.z*moveDir.z);
    if (moveLen > 0.01f) {
        moveDir.x /= moveLen;
        moveDir.z /= moveLen;
    }
    
    playerVel.x = moveDir.x * MOVE_SPEED;
    playerVel.z = moveDir.z * MOVE_SPEED;
    playerVel.y = moveDir.y * MOVE_SPEED;
    
    TSSVec3 newPos;
    newPos.x = playerPos.x + playerVel.x * dt;
    newPos.y = playerPos.y + playerVel.y * dt;
    newPos.z = playerPos.z + playerVel.z * dt;
    
    TSSVec3 resolved = TSSSOAResolveCollision(world, newPos, 0.3f);
    playerPos = resolved;
    
    if (playerPos.y < 1.6f) playerPos.y = 1.6f;
}

void HandleMouseLook(void) {
    if (!mouseLocked) return;
    
    POINT mousePos;
    GetCursorPos(&mousePos);
    
    float dx = (float)(mousePos.x - mouseCenter.x) * MOUSE_SENSITIVITY;
    float dy = (float)(mousePos.y - mouseCenter.y) * MOUSE_SENSITIVITY;
    
    TSSVec3 upAxis; upAxis.x = 0; upAxis.y = 1; upAxis.z = 0;
    TSSQuat yaw = TSSQuat_FromAxisAngle(upAxis, -dx);
    TSSVec3 rightAxis; rightAxis.x = 1; rightAxis.y = 0; rightAxis.z = 0;
    TSSQuat pitchQuat = TSSQuat_FromAxisAngle(rightAxis, -dy);
    
    playerRot = TSSQuat_Multiply(yaw, playerRot);
    
    TSSVec3 euler = TSSQuat_ToEuler(playerRot);
    if (euler.x > 1.57f) euler.x = 1.57f;
    if (euler.x < -1.57f) euler.x = -1.57f;
    playerRot = TSSQuat_FromEuler(euler.x, euler.y, euler.z);
    
    SetCursorPos(mouseCenter.x, mouseCenter.y);
}

void HandleInput(void) {
    moveForward = 0;
    moveRight = 0;
    moveUp = 0;
    
    if (keys['W']) moveForward += 1.0f;
    if (keys['S']) moveForward -= 1.0f;
    if (keys['A']) moveRight -= 1.0f;
    if (keys['D']) moveRight += 1.0f;
    if (keys[' ']) moveUp += 1.0f;
    if (keys['E']) moveUp -= 1.0f;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN) keys[wParam] = 1;
    if (msg == WM_KEYUP) keys[wParam] = 0;
    if (msg == WM_LBUTTONDOWN && !mouseLocked) {
        mouseLocked = 1;
        ShowCursor(0);
        GetCursorPos(&mouseCenter);
    }
    if (msg == WM_KEYDOWN && wParam == VK_ESCAPE) {
        mouseLocked = 0;
        ShowCursor(1);
    }
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void InitWindow(const char* title) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = title;
    RegisterClass(&wc);
    
    ghwnd = CreateWindow(title, title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, SCREEN_W + 16, SCREEN_H + 38,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    ShowWindow(ghwnd, SW_SHOWNORMAL);
    ghdc = GetDC(ghwnd);
    InitGDI(SCREEN_W, SCREEN_H);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    printf("\n========================================\n");
    printf("  TSS 3D Shooter Frame Gen v6.0\n");
    printf("========================================\n");
    printf("  Physics: %.0f Hz | Render: %.0f Hz\n", PHYSICS_HZ, RENDER_HZ);
    printf("  Features:\n");
    printf("    - SoA Entity System\n");
    printf("    - 3D Collision Detection\n");
    printf("    - Camera Smoothing\n");
    printf("    - Vector-based Motion\n");
    printf("========================================\n\n");
    printf("Controls:\n");
    printf("  WASD - Move\n");
    printf("  SPACE - Jump\n");
    printf("  E - Crouch\n");
    printf("  Mouse - Look around\n");
    printf("  Click - Lock mouse\n");
    printf("  ESC - Exit\n\n");
    
    QueryPerformanceFrequency(&perfFreq);
    QueryPerformanceCounter(&lastTime);
    
    InitWindow("TSS 3D Shooter - Frame Generator v6.0");
    
    world = TSSSOACreateWorld(1024);
    InitScene();
    
    int running = 1;
    int frameCount = 0;
    float fpsAccum = 0;
    
    while (running) {
        LARGE_INTEGER curTime;
        QueryPerformanceCounter(&curTime);
        float deltaTime = (float)(curTime.QuadPart - lastTime.QuadPart) / (float)perfFreq.QuadPart;
        lastTime = curTime;
        
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) running = 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        HandleInput();
        HandleMouseLook();
        
        accumulator += deltaTime;
        int physicsSteps = 0;
        
        while (accumulator >= physicsDt && physicsSteps < 5) {
            UpdatePlayer(physicsDt);
            accumulator -= physicsDt;
            physicsSteps++;
        }
        
        float alpha = accumulator / physicsDt;
        if (alpha > 1.0f) alpha = 1.0f;
        
        memset(buffer, 15, sizeof(buffer));
        RenderFloor();
        
        unsigned int i;
        for (i = 0; i < world->boxCount; i++) {
            TSSBoundingBox box = world->boundingBoxes[i];
            float shade = 0.5f + 0.5f * ((i % 3) == 0);
            RenderBox(box, shade, shade * 0.8f, shade * 0.6f);
        }
        
        StretchDIBits(ghdc, 0, 0, SCREEN_W, SCREEN_H, 0, 0, SCREEN_W, SCREEN_H,
                      buffer, &bmi, DIB_RGB_COLORS, SRCCOPY);
        
        frameCount++;
        fpsAccum += deltaTime;
        
        if (fpsAccum >= 1.0f) {
            float fps = (float)frameCount / fpsAccum;
            TSSVec3 camEuler = TSSQuat_ToEuler(playerRot);
            printf("\rFPS: %6.1f | Pos: (%.1f, %.1f, %.1f) | Pitch: %.1f | Yaw: %.1f | Physics: %d Hz    ",
                   fps, playerPos.x, playerPos.y, playerPos.z, 
                   camEuler.x * 180.0f / 3.14159f, camEuler.y * 180.0f / 3.14159f, physicsSteps);
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
    
    if (world) TSSSOADestroyWorld(world);
    ReleaseDC(ghwnd, ghdc);
    
    return 0;
}
