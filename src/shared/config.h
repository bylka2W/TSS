#ifndef TSS_CONFIG_H
#define TSS_CONFIG_H

#include <stdio.h>
#include <string.h>

static inline int tss_config_read_mode(const char* path, int default_mode) {
    FILE* f = fopen(path, "r");
    if (!f) return default_mode;
    char line[256];
    int mode = default_mode;
    while (fgets(line, sizeof(line), f)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (strncmp(p, "mode", 4) != 0) continue;
        p += 4;
        while (*p == ' ' || *p == '\t' || *p == '=') p++;
        if (strcmp(p, "off\r\n") == 0 || strcmp(p, "off\n") == 0)           mode = 0;
        else if (strcmp(p, "easu\r\n") == 0 || strcmp(p, "easu\n") == 0)    mode = 1;
        else if (strcmp(p, "easu_rcas\r\n") == 0 || strcmp(p, "easu_rcas\n") == 0) mode = 2;
        else if (strcmp(p, "debug\r\n") == 0 || strcmp(p, "debug\n") == 0)  mode = 3;
    }
    fclose(f);
    return mode;
}

#endif
