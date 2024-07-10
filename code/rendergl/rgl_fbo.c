#include "rgl_local.h"

/*
=============
R_CheckFBO
=============
*/
static qboolean R_CheckFBO( const fbo_t * fbo )
{
	GLenum code = nglCheckFramebufferStatus( GL_FRAMEBUFFER );

	if ( code == GL_FRAMEBUFFER_COMPLETE ) {
		return qtrue;
    }

	// an error occurred
	switch ( code ) {
	case GL_FRAMEBUFFER_UNSUPPORTED:
		ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) Unsupported framebuffer format\n", fbo->name );
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete attachment\n", fbo->name );
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing attachment\n", fbo->name );
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing draw buffer\n", fbo->name );
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing read buffer\n", fbo->name );
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete multisample\n", fbo->name );
		break;
	default:
		ri.Printf( PRINT_WARNING, "R_CheckFBO: (%s) unknown error 0x%X\n", fbo->name, code );
		break;
	};

	return qfalse;
}

static fbo_t *FBO_Create( const char *name, int width, int height )
{
    fbo_t *fbo;
	int i;
	qboolean exists = qfalse;

    if ( strlen( name ) >= MAX_NPATH ) {
        ri.Error( ERR_DROP, "FBO_Create: \"%s\" too long", name );
    }
    if ( width <= 0 || width > glContext.maxRenderBufferSize ) {
        ri.Error( ERR_DROP, "FBO_Create: bad width %i", width );
    }
    if ( height <= 0 || height > glContext.maxRenderBufferSize ) {
        ri.Error( ERR_DROP, "FBO_Create: bad height %i", height );
    }

    if ( rg.numFBOs == MAX_RENDER_FBOs ) {
        ri.Error( ERR_DROP, "FBO_Create: MAX_RENDER_FBOs hit" );
    }

	for ( i = 0; i < rg.numFBOs; i++ ) {
		if ( !N_stricmp( name, rg.fbos[i]->name ) ) {
			exists = qtrue;
			fbo = rg.fbos[i];
			break;
		}
	}
	if ( !exists ) {
    	fbo = rg.fbos[rg.numFBOs] = ri.Hunk_Alloc( sizeof( *fbo ), h_low );
		rg.numFBOs++;
	}
    N_strncpyz( fbo->name, name, sizeof( fbo->name ) );
    fbo->width = width;
    fbo->height = height;

    nglGenFramebuffers( 1, &fbo->frameBuffer );

    return fbo;
}

static void FBO_CreateBuffer( fbo_t *fbo, int format, int32_t index, int multisample )
{
	uint32_t *pRenderBuffer;
	GLenum attachment;
	qboolean absent;

	switch ( format ) {
	case GL_RGB:
	case GL_RGBA:
	case GL_RGB8:
	case GL_RGBA8:
	case GL_RGB16F_ARB:
	case GL_RGBA16F_ARB:
	case GL_RGB32F_ARB:
	case GL_RGBA32F_ARB:
		fbo->colorFormat = format;
		pRenderBuffer = &fbo->colorBuffers[index];
		attachment = GL_COLOR_ATTACHMENT0 + index;
		break;
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT16_ARB:
	case GL_DEPTH_COMPONENT24_ARB:
	case GL_DEPTH_COMPONENT32_ARB:
		fbo->depthFormat = format;
		pRenderBuffer = &fbo->depthBuffer;
		attachment = GL_DEPTH_ATTACHMENT;
		break;
	case GL_STENCIL_INDEX:
	case GL_STENCIL_INDEX1:
	case GL_STENCIL_INDEX4:
	case GL_STENCIL_INDEX8:
	case GL_STENCIL_INDEX16:
		fbo->stencilFormat = format;
		pRenderBuffer = &fbo->stencilBuffer;
		attachment = GL_STENCIL_ATTACHMENT;
		break;
	case GL_DEPTH_STENCIL:
	case GL_DEPTH24_STENCIL8:
		fbo->packedDepthStencilFormat = format;
		pRenderBuffer = &fbo->packedDepthStencilBuffer;
		attachment = 0; // special for stencil and depth
		break;
	default:
		ri.Printf( PRINT_WARNING, "FBO_CreateBuffer: invalid format %d\n", format );
		return;
	};

	absent = *pRenderBuffer == 0;
	if ( absent ) {
		nglGenRenderbuffers( 1, pRenderBuffer );
    }

	GL_BindFramebuffer( GL_FRAMEBUFFER, fbo->frameBuffer );
    nglBindRenderbuffer( GL_RENDERBUFFER, *pRenderBuffer );
	if ( multisample && glContext.ARB_framebuffer_multisample ) {
//		if ( glContext.NV_framebuffer_multisample_coverage && r_multisampleType->i == AntiAlias_CSAA ) {
//			nglRenderBufferStorageMultisampleCoverageNV( GL_RENDERBUFFER, multisample, 8, format, fbo->width, fbo->height );
//		} else {
	        nglRenderbufferStorageMultisample( GL_RENDERBUFFER, multisample, format, fbo->width, fbo->height );
//		}
    } else {
		nglRenderbufferStorage( GL_RENDERBUFFER, format, fbo->width, fbo->height );
    }

	if ( absent ) {
		if ( attachment == 0 ) {
            nglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, *pRenderBuffer );
            nglFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *pRenderBuffer );
		} else {
            nglFramebufferRenderbuffer( GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, *pRenderBuffer );
		}
	}
    nglBindRenderbuffer( GL_RENDERBUFFER, 0 );
}

void FBO_AttachImage( fbo_t *fbo, texture_t *image, GLenum attachment )
{
    int32_t index;

	GL_BindFramebuffer( GL_FRAMEBUFFER, fbo->frameBuffer );
	GL_BindTexture( TB_DIFFUSEMAP, image );
    nglFramebufferTexture2D( GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, image->id, 0 );
    index = attachment - GL_COLOR_ATTACHMENT0;
    if ( index >= 0 && index <= 15 ) {
        fbo->colorImage[index] = image;
    }
}

void FBO_Bind( fbo_t *fbo )
{
    if ( !glContext.ARB_framebuffer_object ) {
        ri.Printf( PRINT_WARNING, "FBO_Bind() called without framebuffers enabled!\n" );
        return;
    }

    if ( glState.currentFbo == fbo ) {
        return;
    }

    GL_BindFramebuffer( GL_FRAMEBUFFER, fbo ? fbo->frameBuffer : 0 );
    glState.currentFbo = fbo;
	GL_CheckErrors();
}

static void FBO_Clear( fbo_t *fbo )
{
	int i;

	if ( !fbo ) {
		return;
	}

	if ( fbo->frameBuffer ) {
		nglDeleteFramebuffers( 1, &fbo->frameBuffer );
	}
	for ( i = 0; i < glContext.maxColorAttachments; i++ ) {
		if ( fbo->colorBuffers[i] ) {
			nglDeleteRenderbuffers( 1, &fbo->colorBuffers[i] );
		}
	}

	if ( fbo->depthBuffer ) {
		nglDeleteRenderbuffers( 1, &fbo->depthBuffer );
	}

	if ( fbo->stencilBuffer ) {
		nglDeleteRenderbuffers( 1, &fbo->stencilBuffer );
	}

	if ( fbo->frameBuffer ) {
		nglDeleteFramebuffers( 1, &fbo->frameBuffer );
	}
}

static void FBO_List_f( void )
{
	int i, j;
	uint64_t renderBufferMemoryUsed;
	GLint type;

	renderBufferMemoryUsed = 0;

	ri.Printf( PRINT_INFO, " name             width      height\n" );
	ri.Printf( PRINT_INFO, "----------------------------------------------------------\n" );
	for ( i = 0; i < rg.numFBOs; i++ ) {
		for ( j = 0; j < glContext.maxColorAttachments; j++ ) {
			if ( rg.fbos[i]->colorBuffers[j] ) {
				GL_BindFramebuffer( GL_FRAMEBUFFER, rg.fbos[i]->frameBuffer );
	
				nglGetFramebufferAttachmentParameteriv( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + j, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
					&type );

				renderBufferMemoryUsed = 4 * rg.fbos[i]->width * rg.fbos[i]->height;
			}
		}
		ri.Printf( PRINT_INFO, "%s %i %i\n", rg.fbos[i]->name, rg.fbos[i]->width, rg.fbos[i]->height );
	}
	ri.Printf( PRINT_INFO, " %u total FBOs\n", rg.numFBOs );
	ri.Printf( PRINT_INFO, " %0.02lf MB render buffer memory\n", (double)( renderBufferMemoryUsed / ( 1024 * 1024 ) ) );
}

extern void R_CreateBuiltinTextures( void );

static void FBO_Restart_f( void )
{
	int hdrFormat, multisample;
	int width, height;
	int i;

	FBO_Clear( rg.renderFbo );
	FBO_Clear( rg.msaaResolveFbo );
	FBO_Clear( rg.hdrDepthFbo );
	FBO_Clear( rg.smaaBlendFbo );
	FBO_Clear( rg.smaaEdgesFbo );
	FBO_Clear( rg.smaaWeightsFbo );
	FBO_Clear( rg.ssaaResolveFbo );
	FBO_Clear( rg.screenShadowFbo );
	FBO_Clear( rg.screenSsaoFbo );
	FBO_Clear( rg.textureScratchFbo[0] );
	FBO_Clear( rg.textureScratchFbo[1] );
	FBO_Clear( rg.targetLevelsFbo );

	ri.Printf( PRINT_INFO, "------- FBO_Restart -------\n" );

	if ( !glContext.ARB_framebuffer_object || !r_arb_framebuffer_object->i ) {
		return;
	}

	multisample = 0;

	GL_CheckErrors();

	R_IssuePendingRenderCommands();

	width = glConfig.vidWidth;
	height = glConfig.vidHeight;
	switch ( r_multisampleType->i ) {
	case AntiAlias_2xSSAA:
		width *= 2;
		height *= 2;
		break;
	case AntiAlias_4xSSAA:
		width *= 4;
		height *= 4;
		break;
	};

	hdrFormat = GL_RGBA8;
	if ( r_hdr->i && glContext.ARB_texture_float ) {
		hdrFormat = GL_RGBA16F_ARB;
		ri.Printf( PRINT_DEVELOPER, "Using HDR framebuffer format.\n" );
	}

	if ( glContext.ARB_framebuffer_multisample ) {
		nglGetIntegerv( GL_MAX_SAMPLES, &multisample );
	}

	if ( r_multisampleAmount->i < multisample ) {
		multisample = r_multisampleAmount->i;
	}

	if ( multisample < 2 || !glContext.ARB_framebuffer_blit ) {
		multisample = 0;
	}

	if ( multisample != r_multisampleAmount->i ) {
		ri.Cvar_Set( "r_multisampleAmount", va( "%i", multisample ) );
	}

	if ( r_multisampleType->i >= AntiAlias_2xMSAA && r_multisampleType->i <= AntiAlias_32xMSAA ) {
		rg.renderFbo = FBO_Create( "_render", width, height );
		FBO_CreateBuffer( rg.renderFbo, hdrFormat, 0, multisample );
		FBO_CreateBuffer( rg.renderFbo, GL_DEPTH24_STENCIL8, 0, multisample );
		if ( r_bloom->i ) {
			GLuint buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			FBO_CreateBuffer( rg.renderFbo, hdrFormat, 1, multisample );
			nglDrawBuffers( 2, buffers );
		}
		R_CheckFBO( rg.renderFbo );

		rg.msaaResolveFbo = FBO_Create( "_msaaResolve", width, height );
		FBO_AttachImage( rg.msaaResolveFbo, rg.renderImage, GL_COLOR_ATTACHMENT0 );
		FBO_AttachImage( rg.msaaResolveFbo, rg.renderDepthImage, GL_DEPTH_ATTACHMENT );
		R_CheckFBO( rg.msaaResolveFbo );
	}
	if ( r_multisampleType->i == AntiAlias_SMAA ) {
		rg.renderFbo = FBO_Create( "_render", width, height );
		FBO_CreateBuffer( rg.renderFbo, hdrFormat, 0, multisample );
		FBO_CreateBuffer( rg.renderFbo, GL_DEPTH24_STENCIL8, 0, multisample );
		if ( r_bloom->i ) {
			GLuint buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			FBO_CreateBuffer( rg.renderFbo, hdrFormat, 1, multisample );
			nglDrawBuffers( 2, buffers );
		}
		R_CheckFBO( rg.renderFbo );

		rg.smaaBlendFbo = FBO_Create( "_smaaBlend", width, height );
		FBO_AttachImage( rg.smaaBlendFbo, rg.smaaBlendImage, GL_COLOR_ATTACHMENT0 );
		R_CheckFBO( rg.smaaBlendFbo );

		rg.smaaEdgesFbo = FBO_Create( "_smaaEdges", width, height );
		FBO_AttachImage( rg.smaaEdgesFbo, rg.smaaEdgesImage, GL_COLOR_ATTACHMENT0 );
		R_CheckFBO( rg.smaaEdgesFbo );

		rg.smaaWeightsFbo = FBO_Create( "_smaaWeights", width, height );
		FBO_AttachImage( rg.smaaWeightsFbo, rg.smaaWeightsImage, GL_COLOR_ATTACHMENT0 );
		R_CheckFBO( rg.smaaWeightsFbo );
	}
	if ( multisample && r_multisampleType->i >= AntiAlias_2xSSAA && r_multisampleType->i <= AntiAlias_4xSSAA ) {
		rg.ssaaResolveFbo = FBO_Create( "_ssaaResolve", glConfig.vidWidth, glConfig.vidHeight );
		FBO_CreateBuffer( rg.ssaaResolveFbo, hdrFormat, 0, multisample );
		R_CheckFBO( rg.ssaaResolveFbo );
	}

	// clear render buffer
	// this fixes the corrupt screen bug with r_hdr 1 on older hardware
	if ( rg.renderFbo ) {
		GL_BindFramebuffer( GL_FRAMEBUFFER, rg.renderFbo->frameBuffer );
		nglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}
	if ( rg.hdrDepthImage ) {
		rg.hdrDepthFbo = FBO_Create( "_hdrDepth", rg.hdrDepthImage->width, rg.hdrDepthImage->height );
		FBO_CreateBuffer( rg.hdrDepthFbo, GL_RGBA16F, 0, multisample );
		R_CheckFBO( rg.hdrDepthFbo );
	}

	if ( rg.textureScratchImage[0] ) {
		for ( i = 0; i < 2; i++ ) {
			rg.textureScratchFbo[i] = FBO_Create( va( "_texturescratch%d", i ), rg.textureScratchImage[i]->width, rg.textureScratchImage[i]->height );
			FBO_AttachImage( rg.textureScratchFbo[i], rg.textureScratchImage[i], GL_COLOR_ATTACHMENT0 );
			R_CheckFBO( rg.textureScratchFbo[i] );
		}
	}

	if ( rg.screenShadowImage ) {
		rg.screenShadowFbo = FBO_Create( "_screenshadow", rg.screenShadowImage->width, rg.screenShadowImage->height );
		FBO_AttachImage( rg.screenShadowFbo, rg.screenShadowImage, GL_COLOR_ATTACHMENT0 );
		R_CheckFBO( rg.screenShadowFbo );
	}

	if ( rg.screenSsaoImage ) {
		rg.screenSsaoFbo = FBO_Create( "_screenssao", rg.screenSsaoImage->width, rg.screenSsaoImage->height );
		FBO_AttachImage( rg.screenSsaoFbo, rg.screenSsaoImage, GL_COLOR_ATTACHMENT0 );
		R_CheckFBO( rg.screenSsaoFbo );
	}

	if ( rg.targetLevelsImage ) {
		rg.targetLevelsFbo = FBO_Create( "_targetlevels", rg.targetLevelsImage->width, rg.targetLevelsImage->height );
		FBO_AttachImage( rg.targetLevelsFbo, rg.targetLevelsImage, GL_COLOR_ATTACHMENT0 );
		R_CheckFBO( rg.targetLevelsFbo );
	}

/*
	if ( rg.quarterImage[0] ) {
		for ( i = 0; i < 2; i++ ) {
			rg.quarterFbo[i] = FBO_Create( va( "_quarter%d", i ), rg.quarterImage[i]->width, rg.quarterImage[i]->height );
//			FBO_CreateBuffer( rg.quarterFbo[i], GL_RGBA8, i, 0 );
			FBO_AttachImage( rg.quarterFbo[i], rg.quarterImage[i], GL_COLOR_ATTACHMENT0 + i );
			R_CheckFBO( rg.quarterFbo[i] );
		}
	}
*/
}

void FBO_Init( void )
{
	int hdrFormat, multisample;
	int width, height;
	int i;

	ri.Printf( PRINT_INFO, "------- FBO_Init -------\n" );

	if ( !glContext.ARB_framebuffer_object || !r_arb_framebuffer_object->i ) {
		return;
	}

	rg.numFBOs = 0;
	multisample = 0;

	GL_CheckErrors();

	R_IssuePendingRenderCommands();

	width = glConfig.vidWidth;
	height = glConfig.vidHeight;
	switch ( r_multisampleType->i ) {
	case AntiAlias_2xSSAA:
		width *= 2;
		height *= 2;
		break;
	case AntiAlias_4xSSAA:
		width *= 4;
		height *= 4;
		break;
	};

	hdrFormat = GL_RGBA8;
	if ( r_hdr->i && glContext.ARB_texture_float ) {
		hdrFormat = GL_RGBA16F_ARB;
		ri.Printf( PRINT_DEVELOPER, "Using HDR framebuffer format.\n" );
	}

	if ( glContext.ARB_framebuffer_multisample ) {
		nglGetIntegerv( GL_MAX_SAMPLES, &multisample );
	}

	if ( r_multisampleAmount->i < multisample ) {
		multisample = r_multisampleAmount->i;
	}

	if ( multisample < 2 || !glContext.ARB_framebuffer_blit ) {
		multisample = 0;
	}

	if ( multisample != r_multisampleAmount->i ) {
		ri.Cvar_Set( "r_multisampleAmount", va( "%i", multisample ) );
	}

	if ( glContext.ARB_pixel_buffer_object ) {
		ri.Printf( PRINT_INFO, "Allocating pixel buffer object for framebuffer data streaming...\n" );

		nglGenBuffers( 2, rg.pixelPackBuffer );
		nglBindBuffer( GL_PIXEL_PACK_BUFFER, rg.pixelPackBuffer[0] );
		nglBufferData( GL_PIXEL_PACK_BUFFER, width * height * 4, NULL, GL_STREAM_DRAW );
		nglBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

		nglBindBuffer( GL_PIXEL_PACK_BUFFER, rg.pixelPackBuffer[1] );
		nglBufferData( GL_PIXEL_PACK_BUFFER, width * height * 4, NULL, GL_STREAM_DRAW );
		nglBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
	}

	if ( multisample && r_multisampleType->i >= AntiAlias_2xMSAA && r_multisampleType->i <= AntiAlias_32xMSAA ) {
		rg.renderFbo = FBO_Create( "_render", width, height );
		FBO_CreateBuffer( rg.renderFbo, hdrFormat, 0, multisample );
		FBO_CreateBuffer( rg.renderFbo, GL_DEPTH24_STENCIL8, 0, multisample );
		if ( r_bloom->i ) {
			GLuint buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			FBO_CreateBuffer( rg.renderFbo, hdrFormat, 1, multisample );
			nglDrawBuffers( 2, buffers );
		}
		R_CheckFBO( rg.renderFbo );
	}

	if ( multisample && r_multisampleType->i <= AntiAlias_32xMSAA && glContext.ARB_framebuffer_multisample ) {
		rg.msaaResolveFbo = FBO_Create( "_msaaResolve", width, height );
		FBO_CreateBuffer( rg.msaaResolveFbo, hdrFormat, 0, multisample );
		FBO_CreateBuffer( rg.msaaResolveFbo, GL_DEPTH24_STENCIL8, 0, multisample );
//		FBO_AttachImage( rg.msaaResolveFbo, rg.renderImage, GL_COLOR_ATTACHMENT0 );
//		FBO_AttachImage( rg.msaaResolveFbo, rg.renderDepthImage, GL_DEPTH_ATTACHMENT );
		R_CheckFBO( rg.msaaResolveFbo );
	}
	if ( r_multisampleType->i == AntiAlias_SMAA ) {
		rg.smaaBlendFbo = FBO_Create( "_smaaBlend", width, height );
		FBO_AttachImage( rg.smaaBlendFbo, rg.smaaBlendImage, GL_COLOR_ATTACHMENT0 );
		R_CheckFBO( rg.smaaBlendFbo );

		rg.smaaEdgesFbo = FBO_Create( "_smaaEdges", width, height );
		FBO_AttachImage( rg.smaaEdgesFbo, rg.smaaEdgesImage, GL_COLOR_ATTACHMENT0 );
		R_CheckFBO( rg.smaaEdgesFbo );

		rg.smaaWeightsFbo = FBO_Create( "_smaaWeights", width, height );
		FBO_AttachImage( rg.smaaWeightsFbo, rg.smaaWeightsImage, GL_COLOR_ATTACHMENT0 );
		R_CheckFBO( rg.smaaWeightsFbo );
	}
	if ( multisample && r_multisampleType->i >= AntiAlias_2xSSAA && r_multisampleType->i <= AntiAlias_4xSSAA ) {
		rg.renderFbo = FBO_Create( "_render", width, height );
		FBO_CreateBuffer( rg.renderFbo, hdrFormat, 0, multisample );
		FBO_CreateBuffer( rg.renderFbo, GL_DEPTH24_STENCIL8, 0, multisample );
		if ( r_bloom->i ) {
			GLuint buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			FBO_CreateBuffer( rg.renderFbo, hdrFormat, 1, multisample );
			nglDrawBuffers( 2, buffers );
		}
		R_CheckFBO( rg.renderFbo );

		rg.ssaaResolveFbo = FBO_Create( "_ssaaResolve", glConfig.vidWidth, glConfig.vidHeight );
		FBO_CreateBuffer( rg.ssaaResolveFbo, hdrFormat, 0, multisample );
		R_CheckFBO( rg.ssaaResolveFbo );
	}

	if ( r_multisampleType->i >= AntiAlias_2xSSAA && r_multisampleType-> i<= AntiAlias_4xSSAA ) {
		rg.ssaaResolveFbo = FBO_Create( "_ssaa", glConfig.vidWidth, glConfig.vidHeight );
		FBO_CreateBuffer( rg.ssaaResolveFbo, GL_RGBA8, 0, 0 );
		R_CheckFBO( rg.ssaaResolveFbo );
	}

	// clear render buffer
	// this fixes the corrupt screen bug with r_hdr 1 on older hardware
	if ( rg.renderFbo ) {
		GL_BindFramebuffer( GL_FRAMEBUFFER, rg.renderFbo->frameBuffer );
		nglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}
	if ( rg.hdrDepthImage ) {
		rg.hdrDepthFbo = FBO_Create( "_hdrDepth", rg.hdrDepthImage->width, rg.hdrDepthImage->height );
		FBO_CreateBuffer( rg.hdrDepthFbo, GL_RGBA16F, 0, multisample );
		R_CheckFBO( rg.hdrDepthFbo );
	}

	if ( rg.textureScratchImage[0] ) {
		for ( i = 0; i < 2; i++ ) {
			rg.textureScratchFbo[i] = FBO_Create( va( "_texturescratch%d", i ), rg.textureScratchImage[i]->width, rg.textureScratchImage[i]->height );
			FBO_AttachImage( rg.textureScratchFbo[i], rg.textureScratchImage[i], GL_COLOR_ATTACHMENT0 );
			R_CheckFBO( rg.textureScratchFbo[i] );
		}
	}

	if ( rg.screenShadowImage ) {
		rg.screenShadowFbo = FBO_Create( "_screenshadow", rg.screenShadowImage->width, rg.screenShadowImage->height );
		FBO_AttachImage( rg.screenShadowFbo, rg.screenShadowImage, GL_COLOR_ATTACHMENT0 );
		R_CheckFBO( rg.screenShadowFbo );
	}

	if ( rg.screenSsaoImage ) {
		rg.screenSsaoFbo = FBO_Create( "_screenssao", rg.screenSsaoImage->width, rg.screenSsaoImage->height );
		FBO_AttachImage( rg.screenSsaoFbo, rg.screenSsaoImage, GL_COLOR_ATTACHMENT0 );
		R_CheckFBO( rg.screenSsaoFbo );
	}

	if ( rg.targetLevelsImage ) {
		rg.targetLevelsFbo = FBO_Create( "_targetlevels", rg.targetLevelsImage->width, rg.targetLevelsImage->height );
		FBO_AttachImage( rg.targetLevelsFbo, rg.targetLevelsImage, GL_COLOR_ATTACHMENT0 );
		R_CheckFBO( rg.targetLevelsFbo );
	}

/*
	if ( rg.quarterImage[0] ) {
		for ( i = 0; i < 2; i++ ) {
			rg.quarterFbo[i] = FBO_Create( va( "_quarter%d", i ), rg.quarterImage[i]->width, rg.quarterImage[i]->height );
//			FBO_CreateBuffer( rg.quarterFbo[i], GL_RGBA8, i, 0 );
			FBO_AttachImage( rg.quarterFbo[i], rg.quarterImage[i], GL_COLOR_ATTACHMENT0 + i );
			R_CheckFBO( rg.quarterFbo[i] );
		}
	}
	*/

	GL_CheckErrors();

	GL_BindFramebuffer( GL_FRAMEBUFFER, 0 );
	glState.currentFbo = NULL;

	ri.Cmd_AddCommand( "vid_restart_fbo", FBO_Restart_f );
	ri.Cmd_AddCommand( "fbolist", FBO_List_f );
}

void FBO_Shutdown( void )
{
	uint64_t i, j;
	fbo_t *fbo;

    ri.Printf( PRINT_INFO, "------- FBO_Shutdown -------\n" );

	if ( !glContext.ARB_framebuffer_object ) {
		return;
	}

	ri.Cmd_RemoveCommand( "vid_restart_fbo" );
	ri.Cmd_RemoveCommand( "fbolist" );

	FBO_Bind( NULL );

	for ( i = 0; i < rg.numFBOs; i++ ) {
		fbo = rg.fbos[i];

		for ( j = 0; j < glContext.maxColorAttachments; j++ ) {
			if ( fbo->colorBuffers[j] ) {
				nglDeleteRenderbuffers( 1, &fbo->colorBuffers[j] );
			}
		}

		if ( fbo->depthBuffer ) {
			nglDeleteRenderbuffers( 1, &fbo->depthBuffer );
		}

		if ( fbo->stencilBuffer ) {
			nglDeleteRenderbuffers( 1, &fbo->stencilBuffer );
		}

		if ( fbo->frameBuffer ) {
			nglDeleteFramebuffers( 1, &fbo->frameBuffer );
		}
	}

	if ( glContext.ARB_pixel_buffer_object ) {
		nglDeleteBuffers( 2, rg.pixelPackBuffer );
	}
}

void FBO_BlitFromTexture( fbo_t *srcFbo, struct texture_s *src, vec4_t inSrcTexCorners, vec2_t inSrcTexScale, fbo_t *dst,
	ivec4_t inDstBox, struct shaderProgram_s *shaderProgram, const vec4_t inColor, int blend )
{
	ivec4_t dstBox;
	vec4_t color;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];
	vec2_t invTexRes;
	fbo_t *oldFbo = glState.currentFbo;
	mat4_t projection;
	int width, height;
	vec4_t ortho;

	if ( !src ) {
		ri.Printf( PRINT_WARNING, "Tried to blit from a NULL texture! (%s)\n", srcFbo->name );
		return;
	}

	width  = dst ? dst->width  : glConfig.vidWidth;
	height = dst ? dst->height : glConfig.vidHeight;

	if ( inSrcTexCorners ) {
		VectorSet2( texCoords[0], inSrcTexCorners[0], inSrcTexCorners[1] );
		VectorSet2( texCoords[1], inSrcTexCorners[2], inSrcTexCorners[1] );
		VectorSet2( texCoords[2], inSrcTexCorners[2], inSrcTexCorners[3] );
		VectorSet2( texCoords[3], inSrcTexCorners[0], inSrcTexCorners[3] );
	}
	else {
		VectorSet2( texCoords[0], 0.0f, 1.0f );
		VectorSet2( texCoords[1], 1.0f, 1.0f );
		VectorSet2( texCoords[2], 1.0f, 0.0f );
		VectorSet2( texCoords[3], 0.0f, 0.0f );
	}

	// framebuffers are 0 bottom, Y up.
	if ( inDstBox ) {
		dstBox[0] = inDstBox[0];
		dstBox[1] = height - inDstBox[1] - inDstBox[3];
		dstBox[2] = inDstBox[0] + inDstBox[2];
		dstBox[3] = height - inDstBox[1];
	}
	else {
		VectorSet4( dstBox, 0, height, width, 0 );
	}

	if ( inSrcTexScale ) {
		VectorCopy2( invTexRes, inSrcTexScale );
	}
	else {
		VectorSet2( invTexRes, 1.0f, 1.0f );
	}

	if ( inColor ) {
		VectorCopy4( color, invTexRes );
	}
	else {
		VectorCopy4( color, colorWhite );
	}

	if ( !shaderProgram ) {
		shaderProgram = &rg.genericShader[0];
	}

	FBO_Bind( dst );

	nglViewport( 0, 0, width, height );
	nglScissor( 0, 0, width, height );

	GLSL_UseProgram( shaderProgram );
	GLSL_SetUniformVec4( shaderProgram, UNIFORM_COLOR, color );

//	Mat4Ortho( 0, width, height, 0, 0, 1, projection );

	GL_BindTexture( TB_COLORMAP, src );

	VectorSet4( quadVerts[0], dstBox[0], dstBox[1], 0.0f, 1.0f );
	VectorSet4( quadVerts[1], dstBox[2], dstBox[1], 0.0f, 1.0f );
	VectorSet4( quadVerts[2], dstBox[2], dstBox[3], 0.0f, 1.0f );
	VectorSet4( quadVerts[3], dstBox[0], dstBox[3], 0.0f, 1.0f );

	invTexRes[0] /= src->width;
	invTexRes[1] /= src->height;

	GL_State( blend );

	/*
	
	GLSL_SetUniformMatrix4( shaderProgram, UNIFORM_MODELVIEWPROJECTION, projection );
	GLSL_SetUniformVec2( shaderProgram, UNIFORM_INVTEXRES, invTexRes );
	GLSL_SetUniformVec2( shaderProgram, UNIFORM_AUTOEXPOSUREMINMAX, backend.refdef.autoExposureMinMax );
	GLSL_SetUniformVec3( shaderProgram, UNIFORM_TONEMINAVGMAXLINEAR, backend.refdef.toneMinAvgMaxLinear );
	*/

	RB_InstantQuad2( quadVerts, texCoords );

	FBO_Bind( oldFbo );
}

void FBO_Blit( fbo_t *src, ivec4_t inSrcBox, vec2_t srcTexScale, fbo_t *dst, ivec4_t dstBox, struct shaderProgram_s *shaderProgram, const vec4_t color, int blend )
{
	vec4_t srcTexCorners;

	if ( !src ) {
		ri.Printf( PRINT_WARNING, "Tried to blit from a NULL FBO!\n" );
		return;
	}

	if ( inSrcBox ) {
		srcTexCorners[0] =  inSrcBox[0]                / (float)src->width;
		srcTexCorners[1] = (inSrcBox[1] + inSrcBox[3]) / (float)src->height;
		srcTexCorners[2] = (inSrcBox[0] + inSrcBox[2]) / (float)src->width;
		srcTexCorners[3] =  inSrcBox[1]                / (float)src->height;
	}
	else {
		VectorSet4( srcTexCorners, 0.0f, 0.0f, 1.0f, 1.0f );
	}

//	FBO_FastBlit( src, inSrcBox, dst, dstBox, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST );
	FBO_BlitFromTexture( src, src->colorImage[0], srcTexCorners, srcTexScale, dst, dstBox, shaderProgram, color, blend | GLS_DEPTHTEST_DISABLE );
}

void FBO_FastBlit( fbo_t *src, ivec4_t srcBox, fbo_t *dst, ivec4_t dstBox, int buffers, int filter )
{
	ivec4_t srcBoxFinal, dstBoxFinal;
	GLuint srcFb, dstFb;

	if ( !glContext.ARB_framebuffer_blit ) {
		FBO_Blit( src, srcBox, NULL, dst, dstBox, NULL, NULL, 0 );
		return;
	}

	srcFb = src ? src->frameBuffer : 0;
	dstFb = dst ? dst->frameBuffer : 0;

	if ( !srcBox ) {
		int width =  src ? src->width  : glConfig.vidWidth;
		int height = src ? src->height : glConfig.vidHeight;

		VectorSet4( srcBoxFinal, 0, 0, width, height );
	}
	else {
		VectorSet4( srcBoxFinal, srcBox[0], srcBox[1], srcBox[0] + srcBox[2], srcBox[1] + srcBox[3] );
	}

	if ( !dstBox ) {
		int width  = dst ? dst->width  : glConfig.vidWidth;
		int height = dst ? dst->height : glConfig.vidHeight;

		VectorSet4( dstBoxFinal, 0, 0, width, height );
	}
	else {
		VectorSet4( dstBoxFinal, dstBox[0], dstBox[1], dstBox[0] + dstBox[2], dstBox[1] + dstBox[3] );
	}

	GL_BindFramebuffer( GL_READ_FRAMEBUFFER, srcFb );
	GL_BindFramebuffer( GL_DRAW_FRAMEBUFFER, dstFb );
	if ( glContext.ARB_pixel_buffer_object ) {
		GLubyte *src;

		rg.pixelPackBufferIndex = ( rg.pixelPackBufferIndex + 1 ) % 2;
		rg.pixelPackBufferNextIndex = ( rg.pixelPackBufferIndex + 1 ) % 2;

		nglBindBuffer( GL_PIXEL_PACK_BUFFER, rg.pixelPackBuffer[ rg.pixelPackBufferIndex ] );
		nglReadPixels( 0, 0, dstBoxFinal[0], dstBoxFinal[1], GL_RGBA, GL_UNSIGNED_BYTE, NULL );
		nglBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );

		nglBindBuffer( GL_PIXEL_PACK_BUFFER, rg.pixelPackBuffer[ rg.pixelPackBufferNextIndex ] );
		src = (GLubyte *)nglMapBuffer( GL_PIXEL_PACK_BUFFER, GL_READ_ONLY );
		if ( src ) {
			nglUnmapBuffer( GL_PIXEL_PACK_BUFFER );
		}
		nglBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
	}
	else {
		nglBlitFramebuffer( srcBoxFinal[0], srcBoxFinal[1], srcBoxFinal[2], srcBoxFinal[3],
		                      dstBoxFinal[0], dstBoxFinal[1], dstBoxFinal[2], dstBoxFinal[3],
							  buffers, filter );
	}
	GL_BindFramebuffer( GL_FRAMEBUFFER, 0 );
	glState.currentFbo = NULL;
}


void RB_ToneMap( fbo_t *hdrFbo, ivec4_t hdrBox, fbo_t *ldrFbo, ivec4_t ldrBox, int autoExposure )
{
	ivec4_t srcBox, dstBox;
	vec4_t color;
	static int lastFrameCount = 0;

	if ( autoExposure ) {
		if ( lastFrameCount == 0 || rg.frameCount < lastFrameCount || rg.frameCount - lastFrameCount > 5 ) {
			// determine average log luminance
			fbo_t *srcFbo, *dstFbo, *tmp;
			int size = 256;

			lastFrameCount = rg.frameCount;

			VectorSet4( dstBox, 0, 0, size, size );

			FBO_Blit( hdrFbo, hdrBox, NULL, rg.textureScratchFbo[0], dstBox, &rg.calclevels4xShader[0], NULL, 0 );

			srcFbo = rg.textureScratchFbo[0];
			dstFbo = rg.textureScratchFbo[1];

			// downscale to 1x1 texture
			while ( size > 1 ) {
				VectorSet4( srcBox, 0, 0, size, size );
				//size >>= 2;
				size >>= 1;
				VectorSet4( dstBox, 0, 0, size, size );

				if ( size == 1 ) {
					dstFbo = rg.targetLevelsFbo;
				}

				//FBO_Blit(targetFbo, srcBox, NULL, rg.textureScratchFbo[nextScratch], dstBox, &rg.calclevels4xShader[1], NULL, 0);
				FBO_FastBlit( srcFbo, srcBox, dstFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_LINEAR );

				tmp = srcFbo;
				srcFbo = dstFbo;
				dstFbo = tmp;
			}
		}

		// blend with old log luminance for gradual change
		VectorSet4( srcBox, 0, 0, 0, 0 );

		color[0] = 
		color[1] =
		color[2] = 1.0f;
		if ( glContext.ARB_texture_float ) {
			color[3] = 0.03f;
		} else {
			color[3] = 0.1f;
		}

		FBO_Blit( rg.targetLevelsFbo, srcBox, NULL, rg.calcLevelsFbo, NULL,  NULL, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}

	// tonemap
	color[0] =
	color[1] =
	color[2] = pow( 2, r_cameraExposure->f - autoExposure ); //exp2(r_cameraExposure->value);
	color[3] = 1.0f;

	if ( autoExposure ) {
		GL_BindTexture( TB_LEVELSMAP, rg.calcLevelsImage );
	} else {
		GL_BindTexture( TB_LEVELSMAP, rg.fixedLevelsImage );
	}

	FBO_FastBlit( hdrFbo, hdrBox, ldrFbo, NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR );
	FBO_Blit( hdrFbo, hdrBox, NULL, ldrFbo, ldrBox, &rg.tonemapShader, color, 0 );
}


/*
=============
RB_BokehBlur


Blurs a part of one framebuffer to another.

Framebuffers can be identical. 
=============
*/
void RB_BokehBlur(fbo_t *src, ivec4_t srcBox, fbo_t *dst, ivec4_t dstBox, float blur)
{
//	ivec4_t srcBox, dstBox;
	vec4_t color;
	
	blur *= 10.0f;

	if (blur < 0.004f)
		return;

	if (glContext.ARB_framebuffer_object)
	{
		// bokeh blur
		if (blur > 0.0f)
		{
			ivec4_t quarterBox;

			quarterBox[0] = 0;
			quarterBox[1] = rg.quarterFbo[0]->height;
			quarterBox[2] = rg.quarterFbo[0]->width;
			quarterBox[3] = -rg.quarterFbo[0]->height;

			// create a quarter texture
			FBO_Blit(NULL, NULL, NULL, rg.quarterFbo[0], NULL, NULL, NULL, 0);
			//FBO_FastBlit(src, srcBox, rg.quarterFbo[0], quarterBox, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}

#ifndef HQ_BLUR
		if (blur > 1.0f)
		{
			// create a 1/16th texture
			FBO_Blit(rg.quarterFbo[0], NULL, NULL, rg.textureScratchFbo[0], NULL, NULL, NULL, 0);
			//FBO_FastBlit(rg.quarterFbo[0], NULL, rg.textureScratchFbo[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}
#endif

		if (blur > 0.0f && blur <= 1.0f)
		{
			// Crossfade original with quarter texture
			VectorSet4(color, 1, 1, 1, blur);

			FBO_Blit(rg.quarterFbo[0], NULL, NULL, dst, dstBox, NULL, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		}
#ifndef HQ_BLUR
		// ok blur, but can see some pixelization
		else if (blur > 1.0f && blur <= 2.0f)
		{
			// crossfade quarter texture with 1/16th texture
			FBO_Blit(rg.quarterFbo[0], NULL, NULL, dst, dstBox, NULL, NULL, 0);

			VectorSet4(color, 1, 1, 1, blur - 1.0f);

			FBO_Blit(rg.textureScratchFbo[0], NULL, NULL, dst, dstBox, NULL, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		}
		else if (blur > 2.0f)
		{
			// blur 1/16th texture then replace
			int i;

			for (i = 0; i < 2; i++)
			{
				vec2_t blurTexScale;
				float subblur;

				subblur = ((blur - 2.0f) / 2.0f) / 3.0f * (float)(i + 1);

				blurTexScale[0] =
				blurTexScale[1] = subblur;

				color[0] =
				color[1] =
				color[2] = 0.5f;
				color[3] = 1.0f;

				if (i != 0)
					FBO_Blit(rg.textureScratchFbo[0], NULL, blurTexScale, rg.textureScratchFbo[1], NULL, &rg.bokehShader, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
				else
					FBO_Blit(rg.textureScratchFbo[0], NULL, blurTexScale, rg.textureScratchFbo[1], NULL, &rg.bokehShader, color, 0);
			}

			FBO_Blit(rg.textureScratchFbo[1], NULL, NULL, dst, dstBox, NULL, NULL, 0);
		}
#else // higher quality blur, but slower
		else if (blur > 1.0f)
		{
			// blur quarter texture then replace
			int i;

			src = rg.quarterFbo[0];
			dst = rg.quarterFbo[1];

			VectorSet4(color, 0.5f, 0.5f, 0.5f, 1);

			for (i = 0; i < 2; i++)
			{
				vec2_t blurTexScale;
				float subblur;

				subblur = (blur - 1.0f) / 2.0f * (float)(i + 1);

				blurTexScale[0] =
				blurTexScale[1] = subblur;

				color[0] =
				color[1] =
				color[2] = 1.0f;
				if (i != 0)
					color[3] = 1.0f;
				else
					color[3] = 0.5f;

				FBO_Blit(rg.quarterFbo[0], NULL, blurTexScale, rg.quarterFbo[1], NULL, &rg.bokehShader, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
			}

			FBO_Blit(rg.quarterFbo[1], NULL, NULL, dst, dstBox, NULL, NULL, 0);
		}
#endif
	}
}


static void RB_RadialBlur(fbo_t *srcFbo, fbo_t *dstFbo, int passes, float stretch, float x, float y, float w, float h, float xcenter, float ycenter, float alpha)
{
	ivec4_t srcBox, dstBox;
	int srcWidth, srcHeight;
	vec4_t color;
	const float inc = 1.f / passes;
	const float mul = powf(stretch, inc);
	float scale;

	alpha *= inc;
	VectorSet4(color, alpha, alpha, alpha, 1.0f);

	srcWidth  = srcFbo ? srcFbo->width  : glConfig.vidWidth;
	srcHeight = srcFbo ? srcFbo->height : glConfig.vidHeight;

	VectorSet4(srcBox, 0, 0, srcWidth, srcHeight);

	VectorSet4(dstBox, x, y, w, h);
	FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, 0);

	--passes;
	scale = mul;
	while (passes > 0)
	{
		float iscale = 1.f / scale;
		float s0 = xcenter * (1.f - iscale);
		float t0 = (1.0f - ycenter) * (1.f - iscale);

		srcBox[0] = s0 * srcWidth;
		srcBox[1] = t0 * srcHeight;
		srcBox[2] = iscale * srcWidth;
		srcBox[3] = iscale * srcHeight;
			
		FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

		scale *= mul;
		--passes;
	}
}


/*
static qboolean RB_UpdateSunFlareVis(void)
{
	GLuint sampleCount = 0;
	if (!glRefConfig.occlusionQuery)
		return qtrue;

	rg.sunFlareQueryIndex ^= 1;
	if (!rg.sunFlareQueryActive[rg.sunFlareQueryIndex])
		return qtrue;

	// debug code
	if (0)
	{
		int iter;
		for (iter=0 ; ; ++iter)
		{
			GLint available = 0;
			qglGetQueryObjectiv(rg.sunFlareQuery[rg.sunFlareQueryIndex], GL_QUERY_RESULT_AVAILABLE, &available);
			if (available)
				break;
		}

		ri.Printf(PRINT_DEVELOPER, "Waited %d iterations\n", iter);
	}
	
	qglGetQueryObjectuiv(rg.sunFlareQuery[rg.sunFlareQueryIndex], GL_QUERY_RESULT, &sampleCount);
	return sampleCount > 0;
}
*/

void RB_SunRays(fbo_t *srcFbo, ivec4_t srcBox, fbo_t *dstFbo, ivec4_t dstBox)
{
	vec4_t color;
	float dot;
	const float cutoff = 0.25f;
	qboolean colorize = qtrue;

//	float w, h, w2, h2;
	mat4_t mvp;
	vec4_t pos, hpos;

//	dot = DotProduct(rg.sunDirection, backend.viewParms.or.axis[0]);
	if (dot < cutoff)
		return;

//	if (!RB_UpdateSunFlareVis())
//		return;

	// From RB_DrawSun()
	{
		float dist;
		mat4_t trans, model;

//		Mat4Translation( backend.viewParms.or.origin, trans );
//		Mat4Multiply( backend.viewParms.world.modelMatrix, trans, model );
//		Mat4Multiply(backend.viewParms.projectionMatrix, model, mvp);

		dist = glState.viewData.zFar / 1.75;		// div sqrt(3)

//		VectorScale( rg.sunDirection, dist, pos );
	}

	// project sun point
	//Mat4Multiply(backend.viewParms.projectionMatrix, backend.viewParms.world.modelMatrix, mvp);
//	Mat4Transform(mvp, pos, hpos);

	// transform to UV coords
	hpos[3] = 0.5f / hpos[3];

	pos[0] = 0.5f + hpos[0] * hpos[3];
	pos[1] = 0.5f + hpos[1] * hpos[3];

	// initialize quarter buffers
	{
		float mul = 1.f;
		ivec4_t rayBox, quarterBox;
		int srcWidth  = srcFbo ? srcFbo->width  : glConfig.vidWidth;
		int srcHeight = srcFbo ? srcFbo->height : glConfig.vidHeight;

		VectorSet4(color, mul, mul, mul, 1);

		rayBox[0] = srcBox[0] * rg.sunRaysFbo->width  / srcWidth;
		rayBox[1] = srcBox[1] * rg.sunRaysFbo->height / srcHeight;
		rayBox[2] = srcBox[2] * rg.sunRaysFbo->width  / srcWidth;
		rayBox[3] = srcBox[3] * rg.sunRaysFbo->height / srcHeight;

		quarterBox[0] = 0;
		quarterBox[1] = rg.quarterFbo[0]->height;
		quarterBox[2] = rg.quarterFbo[0]->width;
		quarterBox[3] = -rg.quarterFbo[0]->height;

		// first, downsample the framebuffer
		if (colorize)
		{
			FBO_FastBlit(srcFbo, srcBox, rg.quarterFbo[0], quarterBox, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			FBO_Blit(rg.sunRaysFbo, rayBox, NULL, rg.quarterFbo[0], quarterBox, NULL, color, GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);
		}
		else
		{
			FBO_FastBlit(rg.sunRaysFbo, rayBox, rg.quarterFbo[0], quarterBox, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}
	}

	// radial blur passes, ping-ponging between the two quarter-size buffers
	{
		const float stretch_add = 2.f/3.f;
		float stretch = 1.f + stretch_add;
		int i;
		for (i=0; i<2; ++i)
		{
			RB_RadialBlur(rg.quarterFbo[i&1], rg.quarterFbo[(~i) & 1], 5, stretch, 0.f, 0.f, rg.quarterFbo[0]->width, rg.quarterFbo[0]->height, pos[0], pos[1], 1.125f);
			stretch += stretch_add;
		}
	}
	
	// add result back on top of the main buffer
	{
		float mul = 1.f;

		VectorSet4(color, mul, mul, mul, 1);

		FBO_Blit(rg.quarterFbo[0], NULL, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	}
}

static void RB_BlurAxis(fbo_t *srcFbo, fbo_t *dstFbo, float strength, qboolean horizontal)
{
	float dx, dy;
	float xmul, ymul;
	float weights[3] = {
		0.227027027f,
		0.316216216f,
		0.070270270f,
	};
	float offsets[3] = {
		0.f,
		1.3846153846f,
		3.2307692308f,
	};

	xmul = horizontal;
	ymul = 1.f - xmul;

	xmul *= strength;
	ymul *= strength;

	{
		ivec4_t srcBox, dstBox;
		vec4_t color;

		VectorSet4(color, weights[0], weights[0], weights[0], 1.0f);
		VectorSet4(srcBox, 0, 0, srcFbo->width, srcFbo->height);
		VectorSet4(dstBox, 0, 0, dstFbo->width, dstFbo->height);
		FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, 0);

		VectorSet4(color, weights[1], weights[1], weights[1], 1.0f);
		dx = offsets[1] * xmul;
		dy = offsets[1] * ymul;
		VectorSet4(srcBox, dx, dy, srcFbo->width, srcFbo->height);
		FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
		VectorSet4(srcBox, -dx, -dy, srcFbo->width, srcFbo->height);
		FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);

		VectorSet4(color, weights[2], weights[2], weights[2], 1.0f);
		dx = offsets[2] * xmul;
		dy = offsets[2] * ymul;
		VectorSet4(srcBox, dx, dy, srcFbo->width, srcFbo->height);
		FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
		VectorSet4(srcBox, -dx, -dy, srcFbo->width, srcFbo->height);
		FBO_Blit(srcFbo, srcBox, NULL, dstFbo, dstBox, NULL, color, GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE);
	}
}

static void RB_HBlur(fbo_t *srcFbo, fbo_t *dstFbo, float strength)
{
	RB_BlurAxis(srcFbo, dstFbo, strength, qtrue);
}

static void RB_VBlur(fbo_t *srcFbo, fbo_t *dstFbo, float strength)
{
	RB_BlurAxis(srcFbo, dstFbo, strength, qfalse);
}

void RB_GaussianBlur( float blur )
{
	//float mul = 1.f;
	float factor = Com_Clamp( 0.0f, 1.0f, blur );

	if ( factor <= 0.0f ) {
		return;
	}

	{
		ivec4_t srcBox, dstBox;
		vec4_t color;

		VectorSet4(color, 1, 1, 1, 1);

		// first, downsample the framebuffer
		FBO_FastBlit(NULL, NULL, rg.quarterFbo[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		FBO_FastBlit(rg.quarterFbo[0], NULL, rg.textureScratchFbo[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		// set the alpha channel
		nglColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE );
		FBO_BlitFromTexture(rg.textureScratchFbo[0], rg.whiteImage, NULL, NULL, rg.textureScratchFbo[0], NULL, NULL, color, GLS_DEPTHTEST_DISABLE);
		nglColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

		// blur the tiny buffer horizontally and vertically
		RB_HBlur(rg.textureScratchFbo[0], rg.textureScratchFbo[1], factor);
		RB_VBlur(rg.textureScratchFbo[1], rg.textureScratchFbo[0], factor);

		// finally, merge back to framebuffer
		VectorSet4(srcBox, 0, 0, rg.textureScratchFbo[0]->width, rg.textureScratchFbo[0]->height);
		VectorSet4(dstBox, 0, 0, glConfig.vidWidth,              glConfig.vidHeight);
		color[3] = factor;
		FBO_Blit(rg.textureScratchFbo[0], srcBox, NULL, NULL, dstBox, NULL, color, GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
	}
}

void RB_PostProcessSMAA( fbo_t *srcFbo )
{
	//
	// edges pass
	//
	FBO_BlitFromTexture( srcFbo, rg.renderImage, NULL, NULL, rg.smaaEdgesFbo, NULL, &rg.smaaEdgesShader, colorWhite, 0 );

	//
	// weights pass
	//
	GLSL_UseProgram( &rg.smaaWeightsShader );
	GL_BindTexture( 0, rg.smaaEdgesImage );
	GL_BindTexture( 1, rg.smaaAreaImage );
	GL_BindTexture( 2, rg.smaaSearchImage );

	GLSL_SetUniformInt( &rg.smaaWeightsShader, UNIFORM_EDGES_TEXTURE, 0 );
	GLSL_SetUniformInt( &rg.smaaWeightsShader, UNIFORM_AREA_TEXTURE, 1 );
	GLSL_SetUniformInt( &rg.smaaWeightsShader, UNIFORM_SEARCH_TEXTURE, 2 );

	FBO_BlitFromTexture( rg.smaaWeightsFbo, rg.smaaWeightsImage, NULL, NULL, rg.smaaBlendFbo, NULL, &rg.smaaWeightsShader, colorWhite, 0 );

	//
	// blending pass
	//
	GLSL_UseProgram( &rg.smaaBlendShader );
	GL_BindTexture( 0, rg.smaaBlendImage );
	GL_BindTexture( 1, rg.smaaWeightsImage );

	GLSL_SetUniformInt( &rg.smaaBlendShader, UNIFORM_BLEND_TEXTURE, 0 );
	GLSL_SetUniformInt( &rg.smaaBlendShader, UNIFORM_DIFFUSE_MAP, 0 );

	FBO_BlitFromTexture( rg.smaaBlendFbo, rg.smaaBlendImage, NULL, NULL, srcFbo, NULL, &rg.smaaBlendShader, colorWhite, 0 );
}
