#include "rgl_local.h"
#include "rgl_framebuffer.h"

enum {
    FBO_COLOR_MULTISAMPLE,
    FBO_COLOR_GAMMA,
    FBO_COLOR_BLOOM1,
    FBO_COLOR_BLOOM2,
    FBO_COLOR_HDR,

    NUM_COLOR_ATTACHMENTS
};

framebuffer_t *fbo, *intermediate;
shader_t *pintShader;

extern "C" void R_InitIntermediateFramebuffer(void);
extern "C" const char *R_FramebufferFailureReason(GLenum ret);
extern "C" void R_ValidateFramebuffer(const char *name);

extern "C" uint32_t R_HDRFormat(void);
extern "C" uint32_t R_BloomFormat(void);

extern "C" void R_AddColorBuffer(uint32_t format, uint32_t buffer, uint32_t attachment)
{
    nglBindRenderbuffer(GL_RENDERBUFFER, buffer);

    if (!N_stricmp("MSAA", r_multisampleType->s) && r_multisampleAmount->i)
        nglRenderbufferStorageMultisample(GL_RENDERBUFFER, r_multisampleAmount->i, format, fbo->width, fbo->height);
    else
        nglRenderbufferStorage(GL_RENDERBUFFER, format, fbo->width, fbo->height);

    nglBindRenderbuffer(GL_RENDERBUFFER, 0);
}

extern "C" void R_AddDepthBuffer(uint32_t buffer)
{
    nglBindRenderbuffer(GL_RENDERBUFFER, buffer);

    // multisampling?
    if (!N_stricmp("MSAA", r_multisampleType->s) && r_multisampleAmount->i)
        nglRenderbufferStorageMultisample(GL_RENDERBUFFER, r_multisampleAmount->i, GL_DEPTH24_STENCIL8, fbo->width, fbo->height);
    else
        nglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbo->width, fbo->height);

    nglBindRenderbuffer(GL_RENDERBUFFER, 0);
}

extern "C" void R_InitFancyFramebuffer(void)
{
    uint32_t numColorBuffers;

    nglGenFramebuffers(1, (GLuint *)&fbo->fboId);

    fbo->width = r_screenwidth->i;
    fbo->height = r_screenheight->i;

    nglBindFramebuffer(GL_FRAMEBUFFER, fbo->fboId);

    // super-sampling?
    if (!N_stricmpn("SSAA", r_multisampleType->s, sizeof("SSAA"))) {
        fbo->width *= r_multisampleAmount->i;
        fbo->height *= r_multisampleAmount->i;
    }

    for (uint32_t i = 0; i < NUM_COLOR_ATTACHMENTS; i++) {
        nglGenRenderbuffers(1, (GLuint *)&fbo->colorIds[i]);
    }
    nglGenRenderbuffers(1, (GLuint *)&fbo->depthId);

    nglBindRenderbuffer(GL_RENDERBUFFER, fbo->colorIds[FBO_COLOR_MULTISAMPLE]);
    nglRenderbufferStorage(GL_RENDERBUFFER, R_TexFormat(), fbo->width, fbo->height);
    nglBindRenderbuffer(GL_RENDERBUFFER, 0);

//    nglBindRenderbuffer(GL_RENDERBUFFER, fbo->colorIds[FBO_COLOR_BLOOM1]);
//    nglRenderbufferStorage(GL_RENDERBUFFER, R_BloomFormat(), fbo->width, fbo->height);
//    nglBindRenderbuffer(GL_RENDERBUFFER, 0);
//
//    nglBindRenderbuffer(GL_RENDERBUFFER, fbo->colorIds[FBO_COLOR_BLOOM2]);
//    nglRenderbufferStorage(GL_RENDERBUFFER, R_BloomFormat(), fbo->width, fbo->height);
//    nglBindRenderbuffer(GL_RENDERBUFFER, 0);
//
    nglBindRenderbuffer(GL_RENDERBUFFER, fbo->depthId);
    nglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbo->width, fbo->height);
    nglBindRenderbuffer(GL_RENDERBUFFER, 0);

    nglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, fbo->colorIds[FBO_COLOR_MULTISAMPLE]);
    nglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fbo->depthId);

    nglDrawBuffer(GL_COLOR_ATTACHMENT0);

    R_ValidateFramebuffer("fancy");

    nglBindFramebuffer(GL_FRAMEBUFFER, 0);
}

extern "C" void RE_InitFramebuffers(void)
{
    fbo = (framebuffer_t *)ri.Malloc(sizeof(framebuffer_t), &fbo, "GLfbo");
    intermediate = (framebuffer_t *)ri.Malloc(sizeof(framebuffer_t), &intermediate, "GLinterFbo");

    memset(fbo->colorIds, 0, sizeof(fbo->colorIds));
    fbo->depthId = 0;
    fbo->fboId = 0;

    memset(intermediate->colorIds, 0, sizeof(intermediate->colorIds));
    intermediate->depthId = 0;
    intermediate->fboId = 0;

    R_InitFancyFramebuffer();
    R_InitIntermediateFramebuffer();
}

extern "C" void RE_ShutdownFramebuffers(void)
{
    if (!fbo || !intermediate)
        return;

    if (fbo->fboId) {
        nglDeleteFramebuffers(1, (GLuint *)&fbo->fboId);
        nglDeleteRenderbuffers(1, (GLuint *)&fbo->depthId);
        for (uint32_t i = 0; i < NUM_COLOR_ATTACHMENTS; ++i) {
            if (fbo->colorIds[i])
                nglDeleteRenderbuffers(1, (GLuint *)&fbo->colorIds[i]);
        }

        fbo->fboId = 0;
        fbo->depthId = 0;
        memset(fbo->colorIds, 0, sizeof(fbo->colorIds));
        ri.Free(fbo);
    }
    if (intermediate->fboId) {
        nglDeleteFramebuffers(1, (GLuint *)&intermediate->fboId);
        nglDeleteTextures(1, (GLuint *)&intermediate->colorIds[0]);

        intermediate->colorIds[0] = 0;
        intermediate->fboId = 0;
        ri.Free(intermediate);
    }
}

extern "C" void R_InvalidateFramebuffers(void)
{
    if (fbo->fboId) {
        nglDeleteFramebuffers(1, (GLuint *)&fbo->fboId);
        nglDeleteRenderbuffers(1, (GLuint *)&fbo->depthId);
        for (uint32_t i = 0; i < NUM_COLOR_ATTACHMENTS; ++i)
            nglDeleteRenderbuffers(1, (GLuint *)&fbo->colorIds[i]);

        fbo->fboId = 0;
        fbo->depthId = 0;
        memset(fbo->colorIds, 0, sizeof(fbo->colorIds));
    }

    R_InitFancyFramebuffer();

    if (intermediate->fboId) {
        nglDeleteFramebuffers(1, (GLuint *)&intermediate->fboId);
        nglDeleteTextures(1, (GLuint *)&intermediate->colorIds[0]);

        intermediate->colorIds[0] = 0;
        intermediate->fboId = 0;
    }

    R_InitIntermediateFramebuffer();
}

/*
RE_UpdateFramebuffer: if graphics settings are changed in any way during runtime, this is called
to update and make a new valid set of framebuffers
*/
extern "C" void RE_UpdateFramebuffers(void)
{
    R_InvalidateFramebuffers();
}

extern "C" void RE_BeginFramebuffer(void)
{
    // let OpenGL do the job for us
    if (!r_useFramebuffer->i)
        return;

    nglBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo->fboId);
    nglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nglViewport(0, 0, fbo->width, fbo->height);
}

extern "C" void RE_EndFramebuffer(void)
{
    if (!r_useFramebuffer->i)
        return;

    nglBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediate->fboId);
    nglBindFramebuffer(GL_READ_FRAMEBUFFER, fbo->fboId);

    GLenum blit = GL_NEAREST;
    GLenum clear = GL_COLOR_BUFFER_BIT;
    
    // check if ssaa is on (intermediate's dimensions will always be the viewport's dimensions)
    if (fbo->width != intermediate->width || fbo->height != intermediate->height)
        blit = GL_LINEAR;

    nglBlitFramebuffer(0, 0, fbo->width, fbo->height,
                       0, 0, intermediate->width, intermediate->height,
                       GL_COLOR_BUFFER_BIT,
                       blit);
    
    nglBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    nglBindFramebuffer(GL_READ_FRAMEBUFFER, intermediate->fboId);
    nglClear(GL_COLOR_BUFFER_BIT);
    nglDisable(GL_DEPTH_TEST);

    // the intermediate fbo's texture isn't an engine texture, so binding it manually is the only option
    
    R_UnbindTexture();
    nglBindTexture(GL_TEXTURE_2D, intermediate->colorIds[0]);

    nglBegin(GL_TRIANGLE_FAN);

    nglVertex3f( 1.0f,  1.0f, 0.0f); nglTexCoord2f(0.0f, 0.0f);
    nglVertex3f( 1.0f, -1.0f, 0.0f); nglTexCoord2f(0.0f, 1.0f);
    nglVertex3f(-1.0f, -1.0f, 0.0f); nglTexCoord2f(1.0f, 1.0f);
    nglVertex3f(-1.0f,  1.0f, 0.0f); nglTexCoord2f(1.0f, 0.0f);

    nglEnd();
    R_UnbindTexture();

    nglBindFramebuffer(GL_FRAMEBUFFER, 0);
}

extern "C" const char *R_FramebufferFailureReason(GLenum ret)
{
    switch (ret) {
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        return "(GL_FRAMBUFFER_INCOMPLETE_MULTISAMPLE) Framebuffer's Renderbuffer attachments don't all have the same amount of samples";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        return "(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) Framebuffer attachment is invalid";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        return "(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) Framebuffer doesn't have anything attached to it";
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        return "(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS) Framebuffer's dimensions are invalid";
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
        return "(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS) Framebuffer attachment is layered, but improperly done config";
    case GL_FRAMEBUFFER_UNSUPPORTED:
        return "(GL_FRAMEBUFFER_UNSUPPORTED) OpenGL implementation on current hardware doesn't support framebuffer config";
    case GL_FRAMEBUFFER_UNDEFINED:
        return "(GL_FRAMEBUFFER_UNDEFINED) Framebuffer is bound to either draw or read, but the default framebuffer doesn't exist";
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        return "(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER) There is no color buffer attached to the framebuffer's glDrawBuffer";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        return "(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER) There is no color buffer attached to the framebuffer's glReadBuffer";
    };
}

extern "C" void R_ValidateFramebuffer(const char *name)
{
    GLenum ret = nglCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (ret != GL_FRAMEBUFFER_COMPLETE)
        ri.Con_Printf(INFO, "WARNIING: OpenGL %s framebuffer is incomplete, reason: %s", name, R_FramebufferFailureReason(ret));
}

extern "C" void R_InitIntermediateFramebuffer(void)
{
    nglGenFramebuffers(1, (GLuint *)&intermediate->fboId);
    nglGenTextures(1, (GLuint *)&intermediate->colorIds[0]);

    intermediate->width = r_screenwidth->i;
    intermediate->height = r_screenheight->i;

    nglBindFramebuffer(GL_FRAMEBUFFER, intermediate->fboId);

    nglBindTexture(GL_TEXTURE_2D, intermediate->colorIds[0]);
    nglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, intermediate->width, intermediate->height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    nglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    nglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    nglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    nglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    nglBindTexture(GL_TEXTURE_2D, 0);

    nglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, intermediate->colorIds[0], 0);

    R_ValidateFramebuffer("intermediate");

    nglBindFramebuffer(GL_FRAMEBUFFER, 0);
}

extern "C" uint32_t R_HDRFormat(void)
{
    switch (r_textureDetail->i) {
    case TEX_GPUvsGod:
    case TEX_xtreme:
    case TEX_high:
        return GL_RGBA32F;
    case TEX_medium:
    case TEX_low:
        return GL_RGBA16F;
    case TEX_msdos: // not allowed when using msdos level
        return 0;
    };
    ri.N_Error("R_HDRFormat: r_textureDetail has invalid value");
}

extern "C" uint32_t R_BloomFormat(void)
{
    switch (r_textureDetail->i) {
    case TEX_GPUvsGod:
    case TEX_xtreme:
    case TEX_high:
        return GL_RGBA32F;
    case TEX_medium:
    case TEX_low:
        return GL_RGBA16F;
    };
}