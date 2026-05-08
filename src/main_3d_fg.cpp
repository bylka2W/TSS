#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/TSSTransform3D.h"

#define SCREEN_W 800
#define SCREEN_H 600
#define PHYSICS_HZ 30.0f
#define RENDER_HZ 144.0f
#define MAX_ENTITIES 50

#pragma warning(disable:4204)

typedef struct {
    float x, y, z;
    float r, g, b;
} RenderVertex;

typedef struct {
    RenderVertex vertices[8];
    int indices[36];
    float size;
} RenderCube;

RenderCube cubes[MAX_ENTITIES];
int cubeCount = 0;
TSSFrameGenerator3D fg = NULL;
LARGE_INTEGER perfFreq, lastTime, curTime;
float physicsDt = 1.0f / PHYSICS_HZ;
float accumulator = 0.0f;
HWND ghwnd;
HDC ghdc;
BITMAPINFO bmi;
MSG msg;

void InitGDI(int w, int h) {
    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
}

void InitWindow(const char* title) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = DefWindowProc;
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

void CreateCube(RenderCube* cube, float size) {
    cube->size = size;
    float h = size * 0.5f;
    
    cube->vertices[0].x = -h; cube->vertices[0].y = -h; cube->vertices[0].z = -h; cube->vertices[0].r = 1; cube->vertices[0].g = 0; cube->vertices[0].b = 0;
    cube->vertices[1].x = h; cube->vertices[1].y = -h; cube->vertices[1].z = -h; cube->vertices[1].r = 0; cube->vertices[1].g = 1; cube->vertices[1].b = 0;
    cube->vertices[2].x = h; cube->vertices[2].y = h; cube->vertices[2].z = -h; cube->vertices[2].r = 0; cube->vertices[2].g = 0; cube->vertices[2].b = 1;
    cube->vertices[3].x = -h; cube->vertices[3].y = h; cube->vertices[3].z = -h; cube->vertices[3].r = 1; cube->vertices[3].g = 1; cube->vertices[3].b = 0;
    cube->vertices[4].x = -h; cube->vertices[4].y = -h; cube->vertices[4].z = h; cube->vertices[4].r = 1; cube->vertices[4].g = 0; cube->vertices[4].b = 1;
    cube->vertices[5].x = h; cube->vertices[5].y = -h; cube->vertices[5].z = h; cube->vertices[5].r = 0; cube->vertices[5].g = 1; cube->vertices[5].b = 1;
    cube->vertices[6].x = h; cube->vertices[6].y = h; cube->vertices[6].z = h; cube->vertices[6].r = 1; cube->vertices[6].g = 1; cube->vertices[6].b = 1;
    cube->vertices[7].x = -h; cube->vertices[7].y = h; cube->vertices[7].z = h; cube->vertices[7].r = 0; cube->vertices[7].g = 0; cube->vertices[7].b = 0;
    
    int idx[] = {
        0,1,2, 0,2,3,
        4,6,5, 4,7,6,
        0,4,5, 0,5,1,
        2,6,7, 2,7,3,
        0,3,7, 0,7,4,
        1,5,6, 1,6,2
    };
    memcpy(cube->indices, idx, sizeof(idx));
}

void ProjectVertex(RenderVertex* out, float x, float y, float z, float r, float g, float b, TSSMat4 viewProj, float screenW, float screenH) {
    TSSVec3 vpos = {x, y, z};
    vpos = TSSMat4_TransformPoint(viewProj, vpos);
    
    if (vpos.z < 0.1f) {
        out->x = -9999; out->y = -9999; out->z = -9999;
        return;
    }
    
    float ndcX = vpos.x / vpos.z;
    float ndcY = vpos.y / vpos.z;
    
    out->x = (ndcX + 1.0f) * 0.5f * screenW;
    out->y = (1.0f - ndcY) * 0.5f * screenH;
    out->z = vpos.z;
    out->r = r; out->g = g; out->b = b;
}

void RenderCubeToBuffer(RenderCube* cube, TSSMat4 transform, TSSMat4 viewProj, float* buffer, int w, int h) {
    RenderVertex projected[8];
    int i;
    
    for (i = 0; i < 8; i++) {
        TSSVec3 vpos = {cube->vertices[i].x, cube->vertices[i].y, cube->vertices[i].z};
        vpos = TSSMat4_TransformPoint(transform, vpos);
        ProjectVertex(&projected[i], vpos.x, vpos.y, vpos.z, cube->vertices[i].r, cube->vertices[i].g, cube->vertices[i].b, viewProj, (float)w, (float)h);
    }
    
    for (i = 0; i < 36; i++) {
        int i0 = cube->indices[i * 3];
        int i1 = cube->indices[i * 3 + 1];
        int i2 = cube->indices[i * 3 + 2];
        
        RenderVertex* v0 = &projected[i0];
        RenderVertex* v1 = &projected[i1];
        RenderVertex* v2 = &projected[i2];
        
        if (v0->x < -9000 || v1->x < -9000 || v2->x < -9000) continue;
        
        int minX = (int)fmaxf(0.0f, fminf(v0->x, fminf(v1->x, v2->x)));
        int maxX = (int)fminf((float)(w - 1), fmaxf(v0->x, fmaxf(v1->x, v2->x)));
        int minY = (int)fmaxf(0.0f, fminf(v0->y, fminf(v1->y, v2->y)));
        int maxY = (int)fminf((float)(h - 1), fmaxf(v0->y, fmaxf(v1->y, v2->y)));
        
        float ex = v1->x - v0->x;
        float ey = v1->y - v0->y;
        float fx = v2->x - v0->x;
        float fy = v2->y - v0->y;
        float det = ex * fy - ey * fx;
        if ((det > -0.0001f) && (det < 0.0001f)) continue;
        
        float avgZ = (v0->z + v1->z + v2->z) / 3.0f;
        
        float r = (v0->r + v1->r + v2->r) / 3.0f;
        float g = (v0->g + v1->g + v2->g) / 3.0f;
        float b = (v0->b + v1->b + v2->b) / 3.0f;
        
        int py, px;
        for (py = minY; py <= maxY; py++) {
            for (px = minX; px <= maxX; px++) {
                float wx = (float)px - v0->x;
                float wy = (float)py - v0->y;
                
                float s = (wx * fy - wy * fx) / det;
                float t = (wy * ex - wx * ey) / det;
                
                if ((s >= 0) && (t >= 0) && (s + t <= 1)) {
                    float depth = 1.0f / (1.0f - s - t) / (1.0f / avgZ + 0.0001f);
                    if ((depth > 0) && (depth < 1000)) {
                        int idx = (py * w + px) * 4;
                        buffer[idx] = r * 255;
                        buffer[idx + 1] = g * 255;
                        buffer[idx + 2] = b * 255;
                        buffer[idx + 3] = 255;
                    }
                }
            }
        }
    }
}

void AddRandomCube(void) {
    if (cubeCount >= MAX_ENTITIES) return;
    
    float x = ((rand() % 1000) - 500) * 0.01f;
    float y = ((rand() % 1000) - 500) * 0.01f;
    float z = -5.0f - ((rand() % 500) * 0.01f);
    
    float vx = ((rand() % 200) - 100) * 0.1f;
    float vy = ((rand() % 200) - 100) * 0.1f;
    float vz = ((rand() % 200) - 100) * 0.1f;
    
    TSSVec3 pos = {x, y, z};
    TSSVec3 vel = {vx, vy, vz};
    TSSFG3D_AddEntity(fg, pos, vel, 1.0f);
    
    CreateCube(&cubes[cubeCount], 0.5f);
    cubeCount++;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    printf("\n========================================\n");
    printf("  TSS 3D Frame Generator v5.0\n");
    printf("========================================\n");
    printf("  Physics: %.0f Hz | Render: %.0f Hz\n", PHYSICS_HZ, RENDER_HZ);
    printf("  Features:\n");
    printf("    - 3D Transform System\n");
    printf("    - Quaternion Slerp\n");
    printf("    - Vector-based Motion\n");
    printf("    - Sub-pixel Rendering\n");
    printf("========================================\n\n");
    printf("Controls:\n");
    printf("  SPACE - Add new cube\n");
    printf("  R     - Reset simulation\n");
    printf("  ESC   - Exit\n\n");
    
    srand((unsigned int)time(NULL));
    QueryPerformanceFrequency(&perfFreq);
    QueryPerformanceCounter(&lastTime);
    
    InitWindow("TSS 3D Frame Generator v5.0");
    
    fg = TSSCreateFrameGenerator3D(MAX_ENTITIES, PHYSICS_HZ, RENDER_HZ);
    
    TSSCamera3D* cam = TSSFG3D_GetCamera(fg);
    TSSVec3 camPos = {0, 0, 3};
    TSSCamera3D_SetPosition(cam, camPos);
    TSSCamera3D_SetFov(cam, 60.0f);
    TSSCamera3D_SetAspectRatio(cam, (float)SCREEN_W / (float)SCREEN_H);
    
    {
        int i;
        for (i = 0; i < 3; i++) {
            AddRandomCube();
        }
    }
    
    {
        int running = 1;
        int frameCount = 0;
        float fpsAccum = 0;
        
        while (running) {
            QueryPerformanceCounter(&curTime);
            float deltaTime = (float)(curTime.QuadPart - lastTime.QuadPart) / (float)perfFreq.QuadPart;
            lastTime = curTime;
            
            if (deltaTime > 0.1f) deltaTime = 0.1f;
            
            accumulator += deltaTime;
            
            {
                int physicsSteps = 0;
                while ((accumulator >= physicsDt) && (physicsSteps < 5)) {
                    TSSFG3D_UpdatePhysics(fg, physicsDt);
                    accumulator -= physicsDt;
                    physicsSteps++;
                }
                
                float renderAlpha = accumulator / physicsDt;
                if (renderAlpha > 1.0f) renderAlpha = 1.0f;
                
                TSSFG3D_Interpolate(fg, renderAlpha);
                
                {
                    float time = (float)GetTickCount() * 0.001f;
                    TSSVec3 vcamPos;
                    vcamPos.x = sinf(time) * 3.0f;
                    vcamPos.y = 1.0f;
                    vcamPos.z = cosf(time) * 3.0f;
                    TSSQuat camRot = TSSQuat_FromEuler(0, time * 0.5f, 0);
                    TSSFG3D_SetCameraTransform(fg, vcamPos, camRot);
                    TSSFG3D_UpdateCamera(fg, renderAlpha);
                }
                
                {
                    static float buffer[800 * 600 * 4];
                    memset(buffer, 20, sizeof(buffer));
                    
                    TSSMat4 viewProj = TSSFG3D_GetViewProjectionMatrix(fg);
                    
                    {
                        int i;
                        int entityCount = TSSFG3D_GetEntityCount(fg);
                        for (i = 0; i < entityCount && i < cubeCount; i++) {
                            TSSEntity3D* e = TSSFG3D_GetEntity(fg, (unsigned int)i);
                            if (e && e->active) {
                                TSSTransform t = e->current;
                                TSSMat4 transform = TSSTransform_ToMatrix(&t);
                                RenderCubeToBuffer(&cubes[i], transform, viewProj, buffer, SCREEN_W, SCREEN_H);
                            }
                        }
                    }
                    
                    StretchDIBits(ghdc, 0, 0, SCREEN_W, SCREEN_H, 0, 0, SCREEN_W, SCREEN_H,
                                  buffer, &bmi, DIB_RGB_COLORS, SRCCOPY);
                }
            }
            
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT || (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)) {
                    running = 0;
                }
                if (msg.message == WM_KEYDOWN) {
                    if (msg.wParam == VK_SPACE) {
                        AddRandomCube();
                    }
                    if (msg.wParam == 0x52) {
                        TSSDestroyFrameGenerator3D(fg);
                        fg = TSSCreateFrameGenerator3D(MAX_ENTITIES, PHYSICS_HZ, RENDER_HZ);
                        cubeCount = 0;
                        {
                            int i;
                            for (i = 0; i < 3; i++) AddRandomCube();
                        }
                    }
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            
            frameCount++;
            fpsAccum += deltaTime;
            
            if (fpsAccum >= 1.0f) {
                float fps = (float)frameCount / fpsAccum;
                printf("\rFPS: %6.1f | Cubes: %2d | Latency: %.1f ms    ",
                       fps, cubeCount, TSSFG3D_GetLatencyMs(fg));
                fflush(stdout);
                frameCount = 0;
                fpsAccum = 0;
            }
            
            {
                float targetFrame = 1.0f / RENDER_HZ;
                float sleepTime = targetFrame - deltaTime;
                if (sleepTime > 0.001f) {
                    Sleep((DWORD)(sleepTime * 900));
                }
            }
        }
        
        if (fg) TSSDestroyFrameGenerator3D(fg);
    }
    
    ReleaseDC(ghwnd, ghdc);
    
    return 0;
}
