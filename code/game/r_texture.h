#ifndef _R_TEXTURE_
#define _R_TEXTURE_

#pragma once

#include "g_bff.h"

typedef struct
{
    uint32_t magFilter;
    uint32_t minFilter;
    uint32_t wrapS;
    uint32_t wrapT;
    uint32_t slot;
    
    int width;
    int height;
    int channels;
    GLuint id;

    byte* data;
} texture_t;

void R_UpdateTextures(void);
texture_t* R_CreateTexture(const char *filepath, const char *name, const void *texture);
void I_CacheTextures(bffinfo_t *info);
texture_t* R_GetTexture(const char *name);

#endif