#ifndef TSS_LOG_H
#define TSS_LOG_H

#include <windows.h>
#include <stdio.h>

#define TSS_LOG(lvl, fmt, ...) \
    do { \
        char _buf[512]; \
        _snprintf(_buf, sizeof(_buf), "[TSS " lvl "] " fmt "\n", ##__VA_ARGS__); \
        OutputDebugStringA(_buf); \
    } while(0)

#define TSS_INFO(fmt, ...)  TSS_LOG("INFO",  fmt, ##__VA_ARGS__)
#define TSS_WARN(fmt, ...)  TSS_LOG("WARN",  fmt, ##__VA_ARGS__)
#define TSS_ERROR(fmt, ...) TSS_LOG("ERROR", fmt, ##__VA_ARGS__)

#endif
