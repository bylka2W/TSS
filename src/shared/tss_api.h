#ifndef TSS_API_H
#define TSS_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TSS_MODE_ORIGINAL  = 0,
    TSS_MODE_EASU      = 1,
    TSS_MODE_RCAS      = 2,
    TSS_MODE_DEBUG     = 3,
} TSS_Mode;

typedef struct TssContext TssContext;

TssContext* TSS_Init(int width, int height, int mode);
void        TSS_Deinit(TssContext* ctx);
void        TSS_ProcessFrame(TssContext* ctx, const unsigned char* src, unsigned char* dst);
void        TSS_SetMode(TssContext* ctx, int mode);
void        TSS_GetOutputSize(TssContext* ctx, int* out_w, int* out_h);

#ifdef __cplusplus
}
#endif

#endif
