#include "rgl_local.h"
#include "../rendercommon/imgui_impl_opengl3.h"

#define NGL( ret, name, ... ) PFN ## name n ## name = NULL;
NGL_Core_Procs
NGL_Debug_Procs
NGL_Buffer_Procs
NGL_FBO_Procs
NGL_Shader_Procs
NGL_GLSL_SPIRV_Procs
NGL_Texture_Procs
NGL_VertexArray_Procs
NGL_BufferARB_Procs
NGL_VertexArrayARB_Procs
NGL_VertexShaderARB_Procs

NGL_ARB_buffer_storage
NGL_ARB_map_buffer_range
#undef NGL

// because they're edgy...
void R_GLDebug_Callback_AMD(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar *message, GLvoid *userParam);
// the normal stuff
void R_GLDebug_Callback_ARB(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const GLvoid *userParam);

char gl_extensions[32768];

cvar_t *r_useExtensions;
cvar_t *r_allowLegacy;
cvar_t *r_allowShaders;
cvar_t *r_multisample;
cvar_t *r_overBrightBits;
cvar_t *r_ignorehwgamma;
cvar_t *r_normalMapping;
cvar_t *r_specularMapping;
cvar_t *r_genNormalMaps;
cvar_t *r_drawMode;
cvar_t *r_hdr;
cvar_t *r_ssao;
cvar_t *r_greyscale;
cvar_t *r_externalGLSL;
cvar_t *r_ignoreDstAlpha;
cvar_t *r_fullbright;
cvar_t *r_intensity;
cvar_t *r_singleShader;
cvar_t *r_glDebug;
cvar_t *r_textureBits;
cvar_t *r_stencilBits;
cvar_t *r_finish;
cvar_t *r_picmip;
cvar_t *r_roundImagesDown;
cvar_t *r_gammaAmount;
cvar_t *r_printShaders;
cvar_t *r_textureDetail;
cvar_t *r_textureFiltering;
cvar_t *r_speeds;
cvar_t *r_showImages;
cvar_t *r_skipBackEnd;
cvar_t *r_znear;
cvar_t *r_measureOverdraw;
cvar_t *r_ignoreGLErrors;
cvar_t *r_clear;
cvar_t *r_drawBuffer;
cvar_t *r_customWidth;
cvar_t *r_customHeight;

cvar_t *r_maxQuads;
cvar_t *r_maxPolys;
cvar_t *r_maxDLights;
cvar_t *r_maxEntities;

cvar_t *r_imageUpsampleType;
cvar_t *r_imageUpsample;
cvar_t *r_imageUpsampleMaxSize;
cvar_t *r_colorMipLevels;

cvar_t *r_arb_texture_compression;
cvar_t *r_arb_framebuffer_object;
cvar_t *r_arb_vertex_array_object;
cvar_t *r_arb_vertex_buffer_object;
cvar_t *r_arb_texture_filter_anisotropic;
cvar_t *r_arb_texture_float;

renderGlobals_t rg;
glstate_t glState;
gpuConfig_t glConfig;
glContext_t glContext;
renderBackend_t backend;

refimport_t ri;

qboolean R_HasExtension(const char *ext)
{
    const char *ptr = N_stristr( gl_extensions, ext );
	if (ptr == NULL)
		return qfalse;
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));  // verify its complete string.
}

/*
================
R_PrintLongString

Workaround for ri.Printf's 8192 characters buffer limit.
================
*/
static void R_PrintLongString(const char *string)
{
	char buffer[8192];
	const char *p;
	size_t size = strlen(string);

	p = string;
	while(size > 0) {
		N_strncpyz(buffer, p, sizeof (buffer) );
		ri.Printf( PRINT_DEVELOPER, "%s", buffer );
		p += 1023;
		size -= 1023;
	}
}

static void GpuMemInfo_f(void)
{
    switch (glContext.memInfo) {
    case MI_NONE:
        ri.Printf(PRINT_INFO, "No extension found for GPU memory info.\n");
        break;
    case MI_NVX:
        {
            int value;
            int currentVidMem;
            int totalVidMem;

			nglGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &value);
			ri.Printf(PRINT_DEVELOPER, "GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX: %i KiB\n", value);

			nglGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &value);
			ri.Printf(PRINT_DEVELOPER, "GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX: %i KiB\n", value);

			nglGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &value);
			ri.Printf(PRINT_DEVELOPER, "GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX: %i KiB\n", value);

			nglGetIntegerv(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX, &value);
			ri.Printf(PRINT_DEVELOPER, "GPU_MEMORY_INFO_EVICTION_COUNT_NVX: %i\n", value);

			nglGetIntegerv(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, &value);
			ri.Printf(PRINT_DEVELOPER, "GPU_MEMORY_INFO_EVICTED_MEMORY_NVX: %i KiB\n", value);

            ri.Printf(PRINT_DEVELOPER, "------------------------------------------------\n");

            // for those who don't want to read the eyeball barf
            nglGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &totalVidMem);
			ri.Printf(PRINT_INFO, "Total Dedicated Video Memory: %i KiB\n", totalVidMem);

			nglGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &value);
			ri.Printf(PRINT_INFO, "Total Available GPU Memory: %i KiB\n", value);

			nglGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &currentVidMem);
			ri.Printf(PRINT_INFO, "Total Current Dedicated Video Memory: %i KiB\n", currentVidMem);

            ri.Printf(PRINT_INFO, "Total Used Dedicated Video Memory: %i KiB\n", totalVidMem - currentVidMem);

			nglGetIntegerv(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX, &value);
			ri.Printf(PRINT_INFO, "Total Evictions: %i\n", value);

			nglGetIntegerv(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, &value);
			ri.Printf(PRINT_INFO, "Total Evicted Memory: %i KiB\n", value);
        }
        break;
    case MI_ATI:
        {
			int value[4];

			nglGetIntegerv(GL_VBO_FREE_MEMORY_ATI, &value[0]);
			ri.Printf(PRINT_INFO, "VBO_FREE_MEMORY_ATI: %i KiB total %i KiB largest aux: %i KiB total %i KiB largest\n", value[0], value[1], value[2], value[3]);

			nglGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, &value[0]);
			ri.Printf(PRINT_INFO, "TEXTURE_FREE_MEMORY_ATI: %i KiB total %i KiB largest aux: %i KiB total %i KiB largest\n", value[0], value[1], value[2], value[3]);

			nglGetIntegerv(GL_RENDERBUFFER_FREE_MEMORY_ATI, &value[0]);
			ri.Printf(PRINT_INFO, "RENDERBUFFER_FREE_MEMORY_ATI: %i KiB total %i KiB largest aux: %i KiB total %i KiB largest\n", value[0], value[1], value[2], value[3]);
        }
        break;
    };
}


/* 
================== 
RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.

Reads a minimum offset for where the RGB data starts in the image from
integer stored at pointer offset. When the function has returned the actual
offset was written back to address offset. This address will always have an
alignment of packAlign to ensure efficient copying.

Stores the length of padding after a line of pixels to address padlen

Return value must be freed with ri.Hunk_FreeTempMemory()
================== 
*/  

static byte *RB_ReadPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height, size_t *offset, uint32_t *padlen)
{
	byte *buffer, *bufstart;
	int padwidth, linelen;
	GLint packAlign;
	
	nglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);
	
	linelen = width * 3;
	padwidth = PAD(linelen, packAlign);
	
	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = ri.Hunk_AllocateTempMemory(padwidth * height + *offset + packAlign - 1);
	
	bufstart = PADP((intptr_t) buffer + *offset, packAlign);

	nglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);
	
	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;
	
	return buffer;
}


/* 
================== 
R_ScreenshotFilename
================== 
*/  
static void R_ScreenshotFilename( int lastNumber, char *fileName )
{
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		Com_snprintf( fileName, MAX_OSPATH, "screenshots/shot9999.tga" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Com_snprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga"
		, a, b, c, d );
}


/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
static void R_LevelShot( void )
{
	char		checkname[MAX_OSPATH];
	byte		*buffer;
	byte		*source, *allsource;
	byte		*src, *dst;
	size_t			offset = 0;
	uint32_t    padlen;
	uint32_t	x, y;
	uint32_t	r, g, b;
	float		xScale, yScale;
	uint32_t	xx, yy;

	Com_snprintf(checkname, sizeof(checkname), "levelshots/%s.tga", rg.world->baseName);

	allsource = RB_ReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, &offset, &padlen);
	source = allsource + offset;

	buffer = ri.Hunk_AllocateTempMemory(128 * 128*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = 128;
	buffer[14] = 128;
	buffer[16] = 24;	// pixel size

	// resample from source
	xScale = glConfig.vidWidth / 512.0f;
	yScale = glConfig.vidHeight / 384.0f;
	for ( y = 0 ; y < 128 ; y++ ) {
		for ( x = 0 ; x < 128 ; x++ ) {
			r = g = b = 0;
			for ( yy = 0 ; yy < 3 ; yy++ ) {
				for ( xx = 0 ; xx < 4 ; xx++ ) {
					src = source + (3 * glConfig.vidWidth + padlen) * (int)((y*3 + yy) * yScale) +
						3 * (int) ((x*4 + xx) * xScale);
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst = buffer + 18 + 3 * ( y * 128 + x );
			dst[0] = b / 12;
			dst[1] = g / 12;
			dst[2] = r / 12;
		}
	}

	// gamma correction
	if ( glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, 128 * 128 * 3 );
	}

	ri.FS_WriteFile( checkname, buffer, 128 * 128*3 + 18 );

	ri.Hunk_FreeTempMemory(buffer);
	ri.Hunk_FreeTempMemory(allsource);

	ri.Printf( PRINT_INFO, "Wrote %s\n", checkname );
}

/*
==================
R_TakeScreenshot
==================
*/
static void R_TakeScreenshot( int x, int y, int width, int height, char *name, qboolean jpeg ) {
	static char	fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
#if 0
	screenshotCommand_t	*cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	N_strncpyz( fileName, name, sizeof(fileName) );
	cmd->fileName = fileName;
	cmd->jpeg = jpeg;
#endif
}

/* 
================== 
R_ScreenShot_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
================== 
*/
static void R_ScreenShot_f(void)
{
    char checkname[MAX_OSPATH];
    static int lastNumber = -1;
    qboolean silent;

    if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_snprintf( checkname, MAX_OSPATH, "screenshots/%s.tga", ri.Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilename( lastNumber, checkname );

            if (!ri.FS_FileExists( checkname )) {
                break; // file doesn't exist
            }
	    }

		if ( lastNumber >= 9999 ) {
			ri.Printf(PRINT_INFO, "ScreenShot: Couldn't create a file\n"); 
			return;
 		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qfalse );

	if ( !silent ) {
		ri.Printf(PRINT_INFO, "Wrote %s\n", checkname);
	}
}

static void GpuInfo_f( void ) 
{
	const char *enablestrings[] =
	{
		"disabled",
		"enabled"
	};
	const char *fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	ri.Printf( PRINT_INFO, "\nGL_VENDOR: %s\n", glContext.vendor );
	ri.Printf( PRINT_INFO, "GL_RENDERER: %s\n", glContext.renderer );
	ri.Printf( PRINT_INFO, "GL_VERSION: %s\n", glContext.version_str );
	ri.Printf( PRINT_INFO, "GL_EXTENSIONS: " );
	if ( nglGetStringi ) {
		GLint numExtensions;
		int i;

		nglGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
		for ( i = 0; i < numExtensions; i++ ) {
            if (i > 64) {
                break;
            }
			ri.Printf( PRINT_INFO, "%s ", nglGetStringi( GL_EXTENSIONS, i ) );
		}
	}
	else {
		R_PrintLongString( gl_extensions );
	}
	ri.Printf( PRINT_INFO, "\n" );
	ri.Printf( PRINT_INFO, "GL_MAX_TEXTURE_SIZE: %d\n", glContext.maxTextureSize );
	ri.Printf( PRINT_INFO, "GL_MAX_TEXTURE_IMAGE_UNITS: %d\n", glContext.maxTextureUnits );
    ri.Printf( PRINT_INFO, "GL_MAX_TEXTURE_MAX_ANISOTROPY: %f\n", glContext.maxAnisotropy );
	ri.Printf( PRINT_INFO, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	ri.Printf( PRINT_INFO, "MODE: %d, %d x %d %s hz:", ri.Cvar_VariableInteger( "r_mode" ), glConfig.vidWidth, glConfig.vidHeight, fsstrings[ glConfig.isFullscreen != 0 ] );
	if ( glConfig.displayFrequency ) {
		ri.Printf( PRINT_INFO, "%d\n", glConfig.displayFrequency );
	}
	else {
		ri.Printf( PRINT_INFO, "N/A\n" );
	}
#if 0
	if ( glConfig.deviceSupportsGamma )
	{
		ri.Printf( PRINT_INFO, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	}
	else
	{
		ri.Printf( PRINT_INFO, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}
#endif

	ri.Printf( PRINT_INFO, "texture filtering: %s\n", r_textureFiltering->s );
    ri.Printf( PRINT_INFO, "Extensions:\n" );
    ri.Printf( PRINT_INFO, "GL_ARB_framebuffer_blit: %s\n", enablestrings[glContext.ARB_framebuffer_blit]);
    ri.Printf( PRINT_INFO, "GL_ARB_framebuffer_object: %s\n", enablestrings[glContext.ARB_framebuffer_object]);
    ri.Printf( PRINT_INFO, "GL_ARB_framebuffer_multisample: %s\n", enablestrings[glContext.ARB_framebuffer_multisample]);
    ri.Printf( PRINT_INFO, "GL_ARB_framebuffer_sRGB: %s\n", enablestrings[glContext.ARB_framebuffer_sRGB]);
    ri.Printf( PRINT_INFO, "GL_ARB_gl_spirv: %s\n", enablestrings[glContext.ARB_gl_spirv]);
    ri.Printf( PRINT_INFO, "GL_ARB_texture_compression: %s\n", enablestrings[glContext.ARB_texture_compression]);
    ri.Printf( PRINT_INFO, "GL_ARB_texture_filter_anisotropic: %s\n", enablestrings[glContext.ARB_texture_filter_anisotropic]);
    ri.Printf( PRINT_INFO, "GL_ARB_texture_float: %s\n", enablestrings[glContext.ARB_texture_float]);
    ri.Printf( PRINT_INFO, "GL_ARB_vertex_array_object: %s\n", enablestrings[glContext.ARB_vertex_array_object]);
    ri.Printf( PRINT_INFO, "GL_ARB_vertex_buffer_object: %s\n", enablestrings[glContext.ARB_vertex_buffer_object]);
    ri.Printf( PRINT_INFO, "GL_ARB_vertex_shader: %s\n", enablestrings[glContext.ARB_vertex_shader]);

	if ( r_finish->i ) {
		ri.Printf( PRINT_INFO, "Forcing glFinish\n" );
	}
}

static void R_Register(void)
{
    //
    // latched and archived variables
    //
    r_useExtensions = ri.Cvar_Get("r_useExtensions", "1", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_SetDescription(r_useExtensions, "Use all of the OpenGL extensions your card is capable of.");

    r_arb_texture_compression = ri.Cvar_Get("r_arb_texture_compression", "0", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_SetDescription(r_arb_texture_compression, "Enables texture compression.");
    r_arb_framebuffer_object = ri.Cvar_Get("r_arb_framebuffer_object", "1", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_SetDescription(r_arb_framebuffer_object, "Enables post-processing via multiple framebuffers.\n");
    r_arb_vertex_array_object = ri.Cvar_Get("r_arb_vertex_array_object", "0", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_SetDescription(r_arb_vertex_array_object, "Enables use of vertex array object extensions.\nNOTE: only really matters if OpenGL version < 3.3");
    r_arb_vertex_buffer_object = ri.Cvar_Get("r_arb_vertex_buffer_object", "1", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_SetDescription(r_arb_vertex_buffer_object, "Enables use of hardware accelerated vertex and index rendering.");

    r_arb_texture_filter_anisotropic = ri.Cvar_Get("r_arb_texture_filter_anisotropic", "1", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_SetDescription(r_arb_texture_filter_anisotropic, "Enabled anisotropic filtering.");
    r_arb_texture_float = ri.Cvar_Get("r_arb_texture_float", "1", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_SetDescription(r_arb_texture_float, "Enables HDR framebuffer.");

    r_allowLegacy = ri.Cvar_Get("r_allowLegacy", "0", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_SetDescription(r_allowLegacy, "Allow the use of old OpenGL API versions, requires \\r_drawMode 0 or 1 and \\r_allowShaders 0");
    r_allowShaders = ri.Cvar_Get("r_allowShaders", "1", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_SetDescription(r_allowShaders, "Allow the use of GLSL shaders, requires \\r_allowLegacy 0.");

    r_multisample = ri.Cvar_Get("r_multisample", "0", CVAR_SAVE | CVAR_LATCH);

    r_overBrightBits = ri.Cvar_Get("r_overBrightBits", "1", CVAR_SAVE | CVAR_LATCH );
	ri.Cvar_SetDescription( r_overBrightBits, "Sets the intensity of overall brightness of texture pixels." );
	r_ignorehwgamma = ri.Cvar_Get( "r_ignorehwgamma", "0", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_SetDescription( r_ignorehwgamma, "Overrides hardware gamma capabilities." );

    r_normalMapping = ri.Cvar_Get( "r_normalMapping", "1", CVAR_SAVE | CVAR_LATCH );
	ri.Cvar_SetDescription( r_normalMapping, "Enable normal maps for materials that support it." );
    r_specularMapping = ri.Cvar_Get( "r_specularMapping", "1", CVAR_SAVE | CVAR_LATCH );
	ri.Cvar_SetDescription( r_specularMapping, "Enable specular maps for materials that support it." );

    r_imageUpsample = ri.Cvar_Get( "r_imageUpsample", "0", CVAR_SAVE | CVAR_LATCH );
	r_imageUpsampleMaxSize = ri.Cvar_Get( "r_imageUpsampleMaxSize", "1024", CVAR_SAVE | CVAR_LATCH );
	r_imageUpsampleType = ri.Cvar_Get( "r_imageUpsampleType", "1", CVAR_SAVE | CVAR_LATCH );
	r_genNormalMaps = ri.Cvar_Get( "r_genNormalMaps", "0", CVAR_SAVE | CVAR_LATCH );

    r_drawMode = ri.Cvar_Get("r_drawMode", "2", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_SetDescription(r_drawMode,
                            "Sets the rendering mode (see OpenGL docs if you want more info):\n"
                            "   0 - immediate mode, deprecated in modern GPUs\n"
                            "   1 - client buffered, cpu buffers, but uses glDrawElements\n"
                            "   2 - gpu buffered, gpu and cpu buffers, most supported and (probably) the fastest");

    r_hdr = ri.Cvar_Get("r_hdr", "1", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_SetDescription( r_hdr, "Do scene rendering in a framebuffer with high dynamic range." );
    r_ssao = ri.Cvar_Get( "r_ssao", "0", CVAR_SAVE | CVAR_LATCH );
	ri.Cvar_SetDescription( r_ssao, "Enable screen-space ambient occlusion." );

    r_greyscale = ri.Cvar_Get("r_greyscale", "0", CVAR_SAVE | CVAR_LATCH);
	ri.Cvar_CheckRange( r_greyscale, "0", "1", CVT_FLOAT );
	ri.Cvar_SetDescription( r_greyscale, "Desaturates rendered frame." );

	r_externalGLSL = ri.Cvar_Get( "r_externalGLSL", "0", CVAR_LATCH );
    ri.Cvar_SetDescription(r_externalGLSL, "Enables loading glsl from external files instead of just built-in shaders.");

    r_colorMipLevels = ri.Cvar_Get("r_colorMipLevels", "0", CVAR_LATCH );
	ri.Cvar_SetDescription( r_colorMipLevels, "Debugging tool to artificially color different mipmap levels so that they are more apparent." );

    r_ignoreDstAlpha = ri.Cvar_Get( "r_ignoreDstAlpha", "1", CVAR_SAVE | CVAR_LATCH );

    //
    // temporary latched variables that can only change over a restart
    //
    r_fullbright = ri.Cvar_Get("r_fullbright", "0", CVAR_LATCH | CVAR_CHEAT );
	ri.Cvar_SetDescription( r_fullbright, "Debugging tool to render the entire level without lighting." );
	r_intensity = ri.Cvar_Get("r_intensity", "1", CVAR_LATCH );
	ri.Cvar_SetDescription( r_intensity, "Global texture lighting scale." );
	r_singleShader = ri.Cvar_Get("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH );
	ri.Cvar_SetDescription( r_singleShader, "Debugging tool that only uses the default shader for all rendering." );


    //
    // archived variables that can change any time
    //
    r_customWidth = ri.Cvar_Get( "r_customWidth", "1980", CVAR_SAVE | CVAR_LATCH );
	r_customHeight = ri.Cvar_Get( "r_customHeight", "1080", CVAR_SAVE | CVAR_LATCH );

    r_glDebug = ri.Cvar_Get( "r_glDebug", "1", CVAR_SAVE | CVAR_LATCH );
    ri.Cvar_SetDescription( r_glDebug, "Toggles OpenGL driver debug logging." );
    r_textureBits = ri.Cvar_Get( "r_textureBits", "0", CVAR_SAVE | CVAR_LATCH );
	ri.Cvar_SetDescription( r_textureBits, "Number of texture bits per texture." );
	r_stencilBits = ri.Cvar_Get( "r_stencilBits", "8", CVAR_SAVE | CVAR_LATCH );
	ri.Cvar_SetDescription( r_stencilBits, "Stencil buffer size, value decreases Z-buffer depth." );

    r_finish = ri.Cvar_Get("r_finish", "0", CVAR_SAVE | CVAR_LATCH);
	ri.Cvar_SetDescription( r_finish, "Force a glFinish call after rendering a scene." );

    r_picmip = ri.Cvar_Get("r_picmip", "0", CVAR_SAVE | CVAR_LATCH );
	ri.Cvar_CheckRange( r_picmip, "0", "16", CVT_INT );
	ri.Cvar_SetDescription( r_picmip, "Set texture quality, lower is better." );
	r_roundImagesDown = ri.Cvar_Get("r_roundImagesDown", "1", CVAR_SAVE | CVAR_LATCH );
	ri.Cvar_SetDescription( r_roundImagesDown, "When images are scaled, round images down instead of up." );

    r_gammaAmount = ri.Cvar_Get("r_gammaAmount", "1", CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_CheckRange(r_gammaAmount, "0.5", "3", CVT_FLOAT);
    ri.Cvar_SetDescription(r_gammaAmount, "Gamma correction factor.");

    r_printShaders = ri.Cvar_Get( "r_printShaders", "0", 0 );
	ri.Cvar_SetDescription( r_printShaders, "Debugging tool to print on console of the number of shaders used." );

    r_textureDetail = ri.Cvar_Get("r_textureDetail", va("%i", TexDetail_Normie), CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_CheckRange(r_textureDetail, va("%i", TexDetail_MSDOS), va("%i", TexDetail_GPUvsGod), CVT_INT);
    r_textureFiltering = ri.Cvar_Get("r_textureFiltering", va("%i", TexFilter_Trilinear), CVAR_SAVE | CVAR_LATCH);
    ri.Cvar_CheckRange(r_textureFiltering, va("%i", TexFilter_Linear), va("%i", TexFilter_Trilinear), CVT_INT);

    r_speeds = ri.Cvar_Get("r_speeds", "0", CVAR_SAVE | CVAR_LATCH);
	ri.Cvar_SetDescription( r_speeds,
                            "Prints out various debugging stats from renderer:\n"
                            "0: Disabled\n"
                            "1: Backend drawing\n"
                            "2: Lighting\n"
                            "3: OpenGL state changes" );

    //
    // temporary variables that can change at any time
    //
    r_showImages = ri.Cvar_Get( "r_showImages", "0", CVAR_TEMP );
	ri.Cvar_SetDescription( r_showImages,
                                        "Draw all images currently loaded into memory:\n"
                                        " 0: Disabled\n"
                                        " 1: Show images set to uniform size\n"
                                        " 2: Show images with scaled relative to largest image" );

    r_skipBackEnd = ri.Cvar_Get("r_skipBackEnd", "0", CVAR_CHEAT);
	ri.Cvar_SetDescription( r_skipBackEnd, "Skips loading rendering backend." );

    r_znear = ri.Cvar_Get( "r_znear", "4", CVAR_CHEAT );
	ri.Cvar_CheckRange( r_znear, "0.001", "200", CVT_FLOAT );
	ri.Cvar_SetDescription( r_znear, "Viewport distance from view origin (how close objects can be to the player before they're clipped out of the scene)." );

    r_measureOverdraw = ri.Cvar_Get("r_measureOverdraw", "0", CVAR_DEV);
    r_ignoreGLErrors = ri.Cvar_Get("r_ignoreGLErrors", "1", CVAR_LATCH);
    r_clear = ri.Cvar_Get("r_clear", "0", CVAR_DEV);
    r_drawBuffer = ri.Cvar_Get("r_drawBuffer", "GL_BACK", CVAR_DEV);
    ri.Cvar_SetDescription( r_drawBuffer, "Sets which frame buffer to draw into." );

    r_maxQuads = ri.Cvar_Get( "r_maxQuads", va("%lu", (uint64_t)MAX_BATCH_QUADS), CVAR_LATCH | CVAR_PROTECTED );
    ri.Cvar_SetDescription( r_maxQuads, "Sets the maximum amount of quad polygons that can be processed per scene.\n"
                                        "NOTE: tehree can be multiple scenes rendered in a single frame." );
    r_maxPolys = ri.Cvar_Get("r_maxPolys", va("%lu", (uint64_t)MAX_BATCH_QUADS), CVAR_LATCH | CVAR_PROTECTED);
    ri.Cvar_SetDescription(r_maxPolys, "Sets the maximum amount of polygons that can be processed per scene.\n"
                                        "NOTE: there can be multiple scenes rendered in a single frame.");
    r_maxDLights = ri.Cvar_Get( "r_maxDLights", va("%lu", (uint64_t)MAX_DLIGHTS), CVAR_LATCH | CVAR_PROTECTED );
    ri.Cvar_SetDescription( r_maxDLights, "Sets the maximum amount of dynamic lights that can be processed per scene.\n"
                                            "NOTE: there can be multiple scenes rendered in a single frame." );
    r_maxEntities = ri.Cvar_Get( "r_maxEntities", va("%lu", (uint64_t)MAX_RENDER_ENTITIES), CVAR_LATCH | CVAR_PROTECTED );
    ri.Cvar_SetDescription( r_maxEntities, "Sets the maximum amount of dynamic entities that can be processed per scene.\n"
                                            "NOTE: there can be multiple scenes rendered in a single frame." );

    // make sure all commands added here are also
    // removed in R_Shutdown
    ri.Cmd_AddCommand("texturelist", R_ImageList_f);
//    ri.Cmd_AddCommand("shaderlist", R_ShaderList_f);
    ri.Cmd_AddCommand("screenshot", R_ScreenShot_f);
    ri.Cmd_AddCommand("gpuinfo", GpuInfo_f);
    ri.Cmd_AddCommand("gpumeminfo", GpuMemInfo_f);
}

static void R_InitGLContext(void)
{
    if (glConfig.vidWidth == 0) {
        GLint temp;

        if (!ri.GLimp_Init) {
            ri.Error(ERR_FATAL, "OpenGL interface is not initialized");
        }

        ri.GLimp_Init(&glConfig);
        ri.G_SetScaling(1.0, glConfig.vidWidth, glConfig.vidHeight);
    }

    // GL function loader, based on https://gist.github.com/rygorous/16796a0c876cf8a5f542caddb55bce8a
#ifdef _NOMAD_DEBUG
#define NGL( ret, name, ... ) n ## name = (PFN ## name) ri.GL_GetProcAddress(#name); \
                            if (!n ## name) ri.Error(ERR_FATAL, "Failed to load OpenGL proc '" #name "'");
#elif defined(_NOMAD_EXPERIMENTAL)
#define NGL( ret, name, ... ) n ## name = (PFN ## name) ri.GL_GetProcAddress(#name); \
                            if (!n ## name) ri.Printf(PRINT_INFO, COLOR_YELLOW "Failed to load OpenGL proc '" #name "'\n");
#else
#define NGL( ret, name, ... ) n ## name = (PFN ## name) ri.GL_GetProcAddress(#name);
#endif
    NGL_Core_Procs
    NGL_Shader_Procs
    NGL_Texture_Procs
#undef NGL

    if (!nglGetString)
        ri.Error(ERR_FATAL, "glGetString is NULL");

    //
    // get the OpenGL config vars
    //

    N_strncpyz(glContext.vendor, (const char *)nglGetString(GL_VENDOR), sizeof(glContext.vendor));
    N_strncpyz(glContext.renderer, (const char *)nglGetString(GL_RENDERER), sizeof(glContext.renderer));
    N_strncpyz(glContext.version_str, (const char *)nglGetString(GL_VERSION), sizeof(glContext.version_str));
    N_strncpyz(gl_extensions, (const char *)nglGetString(GL_EXTENSIONS), sizeof(gl_extensions));

    N_strncpyz( glConfig.renderer, glContext.renderer, sizeof(glConfig.renderer) );
    N_strncpyz( glConfig.version_str, glContext.version_str, sizeof(glConfig.version_str) );
    N_strncpyz( glConfig.vendor, glContext.vendor, sizeof(glConfig.vendor) );
    N_strncpyz( glConfig.glsl_version_str, (const char *)nglGetString(GL_SHADING_LANGUAGE_VERSION), sizeof(glConfig.glsl_version_str) );
    N_strncpyz( glConfig.extensions, gl_extensions, sizeof(glConfig.extensions) );

    nglGetIntegerv(GL_NUM_EXTENSIONS, &glContext.numExtensions);
    nglGetIntegerv(GL_STEREO, (GLint *)&glContext.stereo);
    nglGetIntegerv(GL_MAX_IMAGE_UNITS, &glContext.maxTextureUnits);
    nglGetIntegerv(GL_MAX_TEXTURE_SIZE, &glContext.maxTextureSize);
    nglGetIntegerv(GL_MAX_SAMPLES, &glContext.maxSamples);

    sscanf(glContext.version_str, "%i.%i", &glContext.versionMajor, &glContext.versionMinor);

    ri.Printf(PRINT_INFO, "Getting OpenGL version...\n");
    // check OpenGL version
    if (!NGL_VERSION_ATLEAST(3, 3)) {
        if (!r_allowLegacy->i) {
            ri.Error(ERR_FATAL, "OpenGL version must be at least 3.3, please install more recent drivers");
        }
        else {
            ri.Printf(PRINT_DEVELOPER, "r_allowLegacy enabled and we have a legacy api version\n");
            ri.Printf(PRINT_INFO, "...using Legacy OpenGL API:");
        }
    }
    else {
        ri.Printf(PRINT_INFO, "...using OpenGL API:");
    }
    ri.Printf(PRINT_INFO, " v%s\n", glContext.version_str);

    //
    // check for broken driver
    //

    if (!glContext.numExtensions) {
        ri.Error(ERR_FATAL, "Broken OpenGL installation, GL_NUM_EXTENSIONS <= 0");
    }
    if (!glContext.maxTextureUnits) {
        ri.Error(ERR_FATAL, "Broken OpenGL installation, GL_MAX_TEXTURE_UNITS <= 0");
    }

    if (glContext.maxTextureUnits < 16) {
        ri.Printf(PRINT_INFO, COLOR_YELLOW "WARNING: GL_MAX_TEXTURE_UNITS < 16 (%i)\n", glContext.maxTextureUnits);
    }

    if (NGL_VERSION_ATLEAST(3, 0)) { // force the loading of specific procs
#ifdef _NOMAD_DEBUG
#define NGL( ret, name, ... ) n ## name = (PFN ## name) ri.GL_GetProcAddress(#name); \
                            if (!n ## name) ri.Error(ERR_FATAL, "Failed to load OpenGL proc '" #name "'");
#elif defined(_NOMAD_EXPERIMENTAL)
#define NGL( ret, name, ... ) n ## name = (PFN ## name) ri.GL_GetProcAddress(#name); \
                            if (!n ## name) ri.Printf(PRINT_INFO, COLOR_YELLOW "Failed to load OpenGL proc '" #name "'\n");
#else
#define NGL( ret, name, ... ) n ## name = (PFN ## name) ri.GL_GetProcAddress(#name);
#endif
        NGL_VertexArray_Procs
        NGL_Buffer_Procs
        NGL_FBO_Procs
#undef NGL
    }

    // check if we need Intel graphics specific fixes
    glContext.intelGraphics = qfalse;
    if (strstr((const char *)nglGetString(GL_RENDERER), "Intel"))
        glContext.intelGraphics = qtrue;
    
    R_InitExtensions();
}

static void R_InitImGui(void)
{
    imguiGL3Import_t import;

    import.glGetString = nglGetString;
    import.glGetStringi = nglGetStringi;
    import.glBindSampler = nglBindSampler;
    import.glScissor = nglScissor;
    import.glViewport = nglViewport;
    import.glUniform1i = nglUniform1i;
    import.glUniform1f = nglUniform1f;
    import.glGetAttribLocation = nglGetAttribLocation;
    import.glGetUniformLocation = nglGetUniformLocation;
    import.glUniformMatrix4fv = nglUniformMatrix4fv;
    import.glEnable = nglEnable;
    import.glDisable = nglDisable;
    import.glGetIntegerv = nglGetIntegerv;
    import.glBlendEquation = nglBlendEquation;
    import.glBlendFuncSeparate = nglBlendFuncSeparate;
    import.glBlendEquationSeparate = nglBlendEquationSeparate;
    import.glPolygonMode = nglPolygonMode;
    import.glPixelStorei = nglPixelStorei;
    import.glIsEnabled = nglIsEnabled;
    import.glIsDisabled = nglIsDisabled;
    import.glIsProgram = nglIsProgram;
    import.glIsShader = nglIsShader;
    import.glDrawElementsBaseVertex = nglDrawElementsBaseVertex;
    import.glDrawElements = nglDrawElements;
    import.glGetVertexAttribPointerv = nglGetVertexAttribPointerv;
    import.glVertexAttribPointer = nglVertexAttribPointer;
    import.glEnableVertexAttribArray = nglEnableVertexAttribArray;
    import.glDisableVertexAttribArray = nglDisableVertexAttribArray;
    import.glBufferData = nglBufferData;
    import.glBufferSubData = nglBufferSubData;
    import.glGetVertexAttribiv = nglGetVertexAttribiv;
    import.glBindTexture = nglBindTexture;
    import.glUseProgram = nglUseProgram;
    import.glBindBuffer = nglBindBuffer;
    import.glBindVertexArray = nglBindVertexArray;
    import.glGenBuffers = nglGenBuffers;
    import.glGenVertexArrays = nglGenVertexArrays;
    import.glDeleteBuffers = nglDeleteBuffers;
    import.glDeleteVertexArrays = nglDeleteVertexArrays;
    import.glGenTextures = nglGenTextures;
    import.glDeleteTextures = nglDeleteTextures;
    import.glTexParameteri = nglTexParameteri;
    import.glTexImage2D = nglTexImage2D;
    import.glActiveTexture = nglActiveTexture;
    import.glAlphaFunc = nglAlphaFunc;
    import.glClear = nglClear;

    ri.ImGui_Init((void *)(uintptr_t)rg.imguiShader.programId, &import);
}

static void R_AllocBackend( void ) {
    uint64_t size;

    size = 0;
    size += PAD( sizeof(*backendData), sizeof(uintptr_t) );
    size += PAD( sizeof(srfPoly_t) * r_maxPolys->i, sizeof(uintptr_t) );
    size += PAD( sizeof(polyVert_t) * r_maxPolys->i * 4, sizeof(uintptr_t) );
    size += PAD( sizeof(glIndex_t) * r_maxPolys->i * 6, sizeof(uintptr_t) );
    size += PAD( sizeof(renderEntityDef_t) * r_maxEntities->i, sizeof(uintptr_t) );
    size += PAD( sizeof(dlight_t) * r_maxEntities->i, sizeof(uintptr_t) );
    size += PAD( sizeof(srfQuad_t) * r_maxQuads->i, sizeof(uintptr_t) );
    
    backendData = ri.Malloc( size );
    backendData->polys = (srfPoly_t *)(backendData + 1);
    backendData->polyVerts = (polyVert_t *)(backendData->polys + r_maxPolys->i);
    backendData->indices = (glIndex_t *)(backendData->polyVerts + (r_maxPolys->i * 4));
    backendData->dlights = (dlight_t *)(backendData->indices + (r_maxPolys->i * 6));
    backendData->entities = (renderEntityDef_t *)(backendData->entities + r_maxDLights->i);
    backendData->quads = (srfQuad_t *)( backendData->entities + r_maxEntities->i );
}

static void R_CameraInfo_f( void ) {
    ri.Printf( PRINT_INFO, "\n---------- Camera Info ----------\n" );
    ri.Printf( PRINT_INFO, "** Matrix Dump: Projection Matrix **\n" );
    Mat4Dump( glState.viewData.camera.projectionMatrix );
    ri.Printf( PRINT_INFO, "** Matrix Dump: View Matrix **\n" );
    Mat4Dump( glState.viewData.camera.viewMatrix );
    ri.Printf( PRINT_INFO, "** Matrix Dump: View Projection Matrix **\n" );
    Mat4Dump( glState.viewData.camera.viewProjectionMatrix );

    ri.Printf( PRINT_INFO, "\n" );
    ri.Printf( PRINT_INFO, "Origin: %f, %f\n", glState.viewData.camera.origin[0], glState.viewData.camera.origin[1] );
    ri.Printf( PRINT_INFO, "Zoom: %f\n", glState.viewData.camera.origin[2] );
    ri.Printf( PRINT_INFO, "Aspect: %f\n", glState.viewData.camera.aspect );
}

static void R_UnloadWorld_f( void ) {
    ri.Printf( PRINT_INFO, "Unloading world...\n" );
    R_ShutdownBuffer( rg.world->buffer );

    rg.world = NULL;
    rg.worldLoaded = qfalse;
}

void R_Init(void)
{
    GLenum error;

    ri.Printf(PRINT_INFO, "---------- RE_Init ----------\n");

    // clear all globals
    memset( &rg, 0, sizeof(rg) );
    memset( &glState, 0, sizeof(glState) );
    memset( &backend, 0, sizeof(backend) );
    memset( &glConfig, 0, sizeof(glConfig) );
    memset( &glContext, 0, sizeof(glContext) );

    glState.viewData.camera.zoom = 1.0f;

    R_Register();

    // allocate backend memory buffers
    R_AllocBackend();

    R_InitNextFrame();

    // initialize OpenGL
    R_InitGLContext();

    // init texture manager
    R_InitTextures();
    
    // init vao manager
    R_InitGPUBuffers();

    // init glsl manager
    GLSL_InitGPUShaders();

    // init shader manager
    R_InitShaders();

    error = nglGetError();
    if (error != GL_NO_ERROR)
        ri.Printf(PRINT_INFO, COLOR_RED "glGetError() = 0x%x\n", error);

    // init imgui
    R_InitImGui();

    ri.Cmd_AddCommand( "camerainfo", R_CameraInfo_f );
    ri.Cmd_AddCommand( "unloadworld", R_UnloadWorld_f );

    // print info
    GpuInfo_f();
    GpuMemInfo_f();
    ri.Printf(PRINT_INFO, "---------- finished RE_Init ----------\n");
}

void RE_BeginRegistration(gpuConfig_t *config)
{
    R_Init();
    rg.registered = qtrue;
    *config = glConfig;
}

void RE_Shutdown(refShutdownCode_t code)
{
    ri.Printf(PRINT_INFO, "RE_Shutdown( %i )\n", code);

    ri.Cmd_RemoveCommand( "texturelist" );
    ri.Cmd_RemoveCommand( "screenshot" );
    ri.Cmd_RemoveCommand( "gpuinfo" );
    ri.Cmd_RemoveCommand( "gpumeminfo" );
    ri.Cmd_RemoveCommand( "camerainfo" );
    ri.Cmd_RemoveCommand( "unloadworld" );

    if (rg.registered) {
        R_IssuePendingRenderCommands();

        R_DeleteTextures();
        R_ShutdownGPUBuffers();
        GLSL_ShutdownGPUShaders();
        ri.ImGui_Shutdown();
    }

    // shutdown platform specific OpenGL thingies
    if (code != REF_KEEP_CONTEXT) {
        ri.GLimp_Shutdown(code == REF_UNLOAD_DLL ? qtrue : qfalse);

        memset( &glConfig, 0, sizeof(glConfig) );
        memset( &glState, 0, sizeof(glState) );
        memset( &backend, 0, sizeof(backend) );
        memset( &glContext, 0, sizeof(glContext) );
    }

    // free everything
    ri.FreeAll();
    memset( &rg, 0, sizeof(rg) );
    backendData = NULL;
}

/*
=============
RE_EndRegistration

Touch all images to make sure they are resident (probably obsolete on modern systems)
=============
*/
void RE_EndRegistration(void) {
    R_IssuePendingRenderCommands();
    RB_ShowImages();
}

void RE_GetConfig(gpuConfig_t *config) {
    *config = glConfig;
}

void *RE_GetTextureImGuiData( nhandle_t hShader ) {
    shader_t *sh;

    sh = R_GetShaderByHandle( hShader );
    if (!sh) {
        ri.Printf( PRINT_WARNING, "no shader found when getting imgui shader texture data.\n" );
        return NULL;
    }

    return (void *)(intptr_t)sh->stages[0]->image->id;
}

GDR_EXPORT renderExport_t *GDR_DECL GetRenderAPI(uint32_t version, refimport_t *import)
{
    static renderExport_t re;

    ri = *import;
    memset(&re, 0, sizeof(re));

    if (version != NOMAD_VERSION_FULL) {
        ri.Error(ERR_FATAL, "GetRenderAPI: rendergl version (%i) != glnomad engine version (%i)", NOMAD_VERSION_FULL, version);
    }

    re.Shutdown = RE_Shutdown;
    re.BeginRegistration = RE_BeginRegistration;
    re.RegisterShader = RE_RegisterShader;
    re.GetConfig = RE_GetConfig;
    re.ImGui_TextureData = RE_GetTextureImGuiData;

    re.ClearScene = RE_ClearScene;
    re.BeginScene = RE_BeginScene;
    re.EndScene = RE_EndScene;
    re.RenderScene = RE_RenderScene;

    re.BeginFrame = RE_BeginFrame;
    re.EndFrame = RE_EndFrame;

    re.RegisterSprite = RE_RegisterSprite;
    re.RegisterSpriteSheet = RE_RegisterSpriteSheet;
    re.RegisterShader = RE_RegisterShader;

    re.LoadWorld = RE_LoadWorldMap;
    re.EndRegistration = RE_EndRegistration;
    
    re.AddPolyListToScene = RE_AddPolyListToScene;
    re.AddSpriteToScene = RE_AddSpriteToScene;
    re.AddPolyToScene = RE_AddPolyToScene;
    re.AddEntityToScene = RE_AddEntityToScene;
    re.SetColor = RE_SetColor;
    re.DrawImage = RE_DrawImage;

    re.CanMinimize = NULL;
    re.ThrottleBackend = NULL;
    re.FinishBloom = NULL;

    return &re;
}

void R_GLDebug_Callback_AMD(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar *message, GLvoid *userParam)
{

}

#define MAX_CACHED_GL_MESSAGES 8192
static char *cachedGLMessages[MAX_CACHED_GL_MESSAGES];
static int numCachedGLMessages = 0;

void R_GLDebug_Callback_ARB(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const GLvoid *userParam)
{
    static const char *color;
    int i;
    uint64_t len;

    if (!r_glDebug->i)
        return; // we don't want to confuse non-developers...

    // save the messages so OpenGL can't spam us with useless shit
    for (i = 0; i < numCachedGLMessages; i++) {
        if (!N_stricmp(cachedGLMessages[i], message)) {
            return;
        }
    }

    // allocate it on the hunk so we don't need to clean it up
    len = strlen(message) + 1;
    cachedGLMessages[numCachedGLMessages] = (char *)ri.Hunk_Alloc(len, h_low);
    cachedGLMessages[numCachedGLMessages][len - 1] = '\0';
    strcpy(cachedGLMessages[numCachedGLMessages], message);
    numCachedGLMessages++;

    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        ri.Printf(PRINT_INFO, COLOR_MAGENTA "[GLDebug Log]:" COLOR_WHITE " %s\n", message);
        return;
    }

    if (!color) {
        if (type == GL_DEBUG_TYPE_ERROR_ARB || type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB) {
            color = COLOR_RED;
        }
        else if (type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB) {
            color = COLOR_YELLOW;
        }
        else if (type == GL_DEBUG_TYPE_PERFORMANCE_ARB || type == GL_DEBUG_TYPE_PORTABILITY_ARB) {
            color = COLOR_CYAN;
        }
        else {
            color = COLOR_WHITE;
        }
    }

    ri.Printf(PRINT_INFO, "%s[GLDebug Log]:" COLOR_WHITE" %s\n", color, message);

    switch (source) {
    case GL_DEBUG_SOURCE_API_ARB:
        ri.Printf(PRINT_INFO, "\tSource: GL_DEBUG_SOURCE_API_ARB\n");
        break;
    case GL_DEBUG_SOURCE_APPLICATION_ARB:
        ri.Printf(PRINT_INFO, "\tSource: GL_DEBUG_SOURCE_APPLICATION_ARB\n");
        break;
    case GL_DEBUG_SOURCE_OTHER_ARB:
        ri.Printf(PRINT_INFO, "\tSource: GL_DEBUG_SOURCE_OTHER_ARB\n");
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
        ri.Printf(PRINT_INFO, "\tSource: GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB\n");
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
        ri.Printf(PRINT_INFO, "\tSource: GL_DEBUG_SOURCE_THIRD_PARTY_ARB\n");
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
        ri.Printf(PRINT_INFO, "\tSource: GL_DEBUG_SOURCE_SHADER_COMPILER_ARB\n");
        break;
    };

    switch (type) {
    case GL_DEBUG_TYPE_ERROR_ARB:
        ri.Printf(PRINT_INFO, "\tType: GL_DEBUG_TYPE_ERROR_ARB\n");
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        ri.Printf(PRINT_INFO, "\tType: GL_DEBUG_TYPE_DEPRECATED_BEHAVIOUR_ARB\n");
        break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
        ri.Printf(PRINT_INFO, "\tType: GL_DEBUG_TYPE_PERFORMANCE_ARB\n");
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        ri.Printf(PRINT_INFO, "\tType: GL_DEBUG_TYPE_UNDEFINED_BEHAVIOUR_ARB\n");
        break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
        ri.Printf(PRINT_INFO, "\tType: GL_DEBUG_TYPE_PORTABILITY_ARB\n");
        break;
    case GL_DEBUG_TYPE_OTHER_ARB:
        ri.Printf(PRINT_INFO, "\tType: GL_DEBUG_TYPE_OTHER_ARB\n");
        break;
    };

    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH_ARB:
        ri.Printf(PRINT_INFO, "\tSeverity: GL_DEBUG_SEVERITY_HIGH_ARB\n");
        break;
    case GL_DEBUG_SEVERITY_MEDIUM_ARB:
        ri.Printf(PRINT_INFO, "\tSeverity: GL_DEBUG_SEVERITY_MEDIUM_ARB\n");
        break;
    case GL_DEBUG_SEVERITY_LOW_ARB:
        ri.Printf(PRINT_INFO, "\tSeverity: GL_DEBUG_SEVERITY_LOW_ARB\n");
        break;
    };
    ri.Printf(PRINT_INFO, "\n");
}
