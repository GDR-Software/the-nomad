#include "rgl_local.h"

extern const char *fallbackShader_generic_vp;
extern const char *fallbackShader_generic_fp;
extern const char *fallbackShader_imgui_vp;
extern const char *fallbackShader_imgui_fp;
extern const char *fallbackShader_ssao_vp;
extern const char *fallbackShader_ssao_fp;
extern const char *fallbackShader_tile_vp;
extern const char *fallbackShader_tile_fp;
extern const char *fallbackShader_down4x_vp;
extern const char *fallbackShader_down4x_fp;
extern const char *fallbackShader_bokeh_vp;
extern const char *fallbackShader_bokeh_fp;
extern const char *fallbackShader_depthblur_vp;
extern const char *fallbackShader_depthblur_fp;
extern const char *fallbackShader_calclevels4x_vp;
extern const char *fallbackShader_calclevels4x_fp;
extern const char *fallbackShader_tonemap_vp;
extern const char *fallbackShader_tonemap_fp;
extern const char *fallbackShader_lightall_vp;
extern const char *fallbackShader_lightall_fp;
extern const char *fallbackShader_texturecolor_vp;
extern const char *fallbackShader_texturecolor_fp;

#define GLSL_VERSION_ATLEAST(major,minor) (glContext.glslVersionMajor > (major) || (glContext.versionMajor == (major) && glContext.glslVersionMinor >= minor))

typedef struct {
    const char *name;
    uint64_t type;
} uniformInfo_t;

// these must in the same order as in uniform_t in rgl_local.h
static uniformInfo_t uniformsInfo[UNIFORM_COUNT] = {
    { "u_DiffuseMap",           GLSL_INT },
	{ "u_LightMap",             GLSL_INT },
	{ "u_NormalMap",            GLSL_INT },
	{ "u_DeluxeMap",            GLSL_INT },
	{ "u_SpecularMap",          GLSL_INT },

	{ "u_TextureMap",           GLSL_INT },
	{ "u_LevelsMap",            GLSL_INT },

	{ "u_ScreenImageMap",       GLSL_INT },
	{ "u_ScreenDepthMap",       GLSL_INT },

	{ "u_ShadowMap",            GLSL_INT },
	{ "u_ShadowMap2",           GLSL_INT },
	{ "u_ShadowMap3",           GLSL_INT },
	{ "u_ShadowMap4",           GLSL_INT },

	{ "u_ShadowMvp",            GLSL_MAT16 },
	{ "u_ShadowMvp2",           GLSL_MAT16 },
	{ "u_ShadowMvp3",           GLSL_MAT16 },
	{ "u_ShadowMvp4",           GLSL_MAT16 },

	{ "u_EnableTextures",       GLSL_VEC4 },

	{ "u_DiffuseTexMatrix",     GLSL_VEC4 },
	{ "u_DiffuseTexOffTurb",    GLSL_VEC4 },

	{ "u_TCGen0",               GLSL_INT },
	{ "u_TCGen0Vector0",        GLSL_VEC3 },
	{ "u_TCGen0Vector1",        GLSL_VEC3 },

	{ "u_DeformGen",            GLSL_INT },
	{ "u_DeformParams",         GLSL_VEC5 },

	{ "u_ColorGen",             GLSL_INT },
	{ "u_AlphaGen",             GLSL_INT },
	{ "u_Color",                GLSL_VEC4 },
	{ "u_BaseColor",            GLSL_VEC4 },
	{ "u_VertColor",            GLSL_VEC4 },

	{ "u_DlightInfo",           GLSL_VEC4 },
	{ "u_LightForward",         GLSL_VEC3 },
	{ "u_LightUp",              GLSL_VEC3 },
	{ "u_LightRight",           GLSL_VEC3 },
	{ "u_LightOrigin",          GLSL_VEC4 },
	{ "u_ModelLightDir",        GLSL_VEC3 },
	{ "u_LightRadius",          GLSL_FLOAT },
	{ "u_AmbientColor",         GLSL_VEC3 },
	{ "u_DirectedLight",        GLSL_VEC3 },

	{ "u_ModelViewProjection",  GLSL_MAT16 },

	{ "u_Time",                 GLSL_FLOAT },
	{ "u_VertexLerp" ,          GLSL_FLOAT },
	{ "u_NormalScale",          GLSL_VEC4 },
	{ "u_SpecularScale",        GLSL_VEC4 },

	{ "u_ViewInfo",             GLSL_VEC4 },
	{ "u_ViewOrigin",           GLSL_VEC3 },
	{ "u_LocalViewOrigin",      GLSL_VEC3 },
	{ "u_ViewForward",          GLSL_VEC3 },
	{ "u_ViewLeft",             GLSL_VEC3 },
	{ "u_ViewUp",               GLSL_VEC3 },

	{ "u_InvTexRes",            GLSL_VEC2 },
	{ "u_AutoExposureMinMax",   GLSL_VEC2 },
	{ "u_ToneMinAvgMaxLinear",  GLSL_VEC3 },

	{ "u_PrimaryLightOrigin",   GLSL_VEC4  },
	{ "u_PrimaryLightColor",    GLSL_VEC3  },
	{ "u_PrimaryLightAmbient",  GLSL_VEC3  },
	{ "u_PrimaryLightRadius",   GLSL_FLOAT },

	{ "u_AlphaTest",            GLSL_INT },

    { "u_GammaAmount",          GLSL_FLOAT },

    { "u_NumLights",            GLSL_INT },

    { "u_Exposure",             GLSL_FLOAT },
    { "u_ScreenSize",           GLSL_VEC2 },
    { "u_SharpenAmount",        GLSL_FLOAT },
    { "u_LightBuffer",          GLSL_BUFFER },
};

static shaderProgram_t *hashTable[MAX_RENDER_SHADERS];

#define SHADER_CACHE_FILE_NAME "shadercache.dat"

typedef struct {
    char name[MAX_NPATH];
    uint32_t fmt;
    void *data;
    uint64_t size;
} shaderCacheEntry_t;

static shaderCacheEntry_t *cacheHashTable;

//
// R_InitShaderCache
//
static void R_InitShaderCache( void )
{
    cacheHashTable = (shaderCacheEntry_t *)ri.Hunk_Alloc( sizeof(*cacheHashTable) * rg.numPrograms, h_low );
    memset(cacheHashTable, 0, sizeof(cacheHashTable));

    for (uint64_t i = 0; i < rg.numPrograms; i++) {

    }
}

static void R_LoadShaderCache(void)
{
    fileHandle_t f;
    uint64_t numEntries, i;
    uint64_t hash;
    char name[MAX_GDR_PATH];
    shaderCacheEntry_t *entry;

    f = ri.FS_FOpenRead(SHADER_CACHE_FILE_NAME);
    if (f == FS_INVALID_HANDLE) {
        if (ri.FS_FileExists(SHADER_CACHE_FILE_NAME)) {
            ri.Error(ERR_DROP, "Failed to load shader cache file even though it exists");
        }
        else {
            ri.Printf(PRINT_INFO, "Failed to load shader cache file, probably doesn't exist yet\n");
        }
    }

    if (!ri.FS_Read(&numEntries, sizeof(uint64_t), f)) {
        ri.FS_FClose(f);
        ri.Printf(PRINT_DEVELOPER, "Error reading shader cache numEntries\n");
    }
    if (!numEntries) {
        ri.Error(ERR_DROP, "R_LoadShaderCache: numEntries is a funny number");
    }

    cacheHashTable = ri.Hunk_Alloc( sizeof(*cacheHashTable) * numEntries, h_low );

    for (i = 0; i < numEntries; i++) {
        if (!ri.FS_Read(name, sizeof(name), f)) {
            ri.FS_FClose(f);
            ri.Printf(PRINT_DEVELOPER, "Error reading shader cache entry name at %lu\n", i);
        }
        
        hash = Com_GenerateHashValue(name, MAX_RENDER_SHADERS);
        entry = &cacheHashTable[hash];
        memcpy(entry->name, name, sizeof(entry->name));

        if (!ri.FS_Read(&entry->size, sizeof(uint64_t), f)) {
            ri.FS_FClose(f);
            ri.Printf(PRINT_DEVELOPER, "Error reading shader cache entry size at %lu\n", i);
        }

        entry->data = ri.Hunk_Alloc( entry->size, h_low );
        if (!ri.FS_Read(entry->data, entry->size, f)) {
            ri.FS_FClose(f);
            ri.Printf(PRINT_DEVELOPER, "Error reading shader cache entry buffer at %lu\n", i);
        }
    }

    ri.FS_FClose(f);
}

static void R_SaveShaderToCache(const shaderProgram_t *program, fileHandle_t cacheFile)
{
    shaderCacheEntry_t entry;
    GLint length;
    GLenum fmt;

    if (glContext.ARB_gl_spirv) {
        return;
    }

    memset(&entry, 0, sizeof(entry));

    nglGetProgramiv(program->programId, GL_PROGRAM_BINARY_LENGTH, &length);

    entry.data = ri.Hunk_AllocateTempMemory(length);
    entry.size = length;

    nglGetProgramBinary(program->programId, length, NULL, (GLenum *)&entry.fmt, (char *)entry.data + sizeof(GLenum));
    
    N_strncpyz(entry.name, program->name, sizeof(entry.name));
    ri.FS_Write(entry.name, sizeof(entry.name), cacheFile);
    ri.FS_Write(&entry.size, sizeof(uint64_t), cacheFile);
    ri.FS_Write(&entry.fmt, sizeof(uint32_t), cacheFile);
    ri.FS_Write(entry.data, entry.size, cacheFile);

    ri.Hunk_FreeTempMemory(entry.data);
}

static int R_ShaderSavedToCache(const char *name)
{

}

typedef enum {
	GLSL_PRINTLOG_PROGRAM_INFO,
	GLSL_PRINTLOG_SHADER_INFO,
	GLSL_PRINTLOG_SHADER_SOURCE
} glslPrintLog_t;

static void GLSL_PrintLog(GLuint programOrShader, glslPrintLog_t type, qboolean developerOnly)
{
    static char     *msg;
	static char     msgPart[8192];
	GLsizei         maxLength = 0;
	uint32_t        i;
	const int       printLevel = developerOnly ? PRINT_DEVELOPER : PRINT_INFO;

	switch (type) {
	case GLSL_PRINTLOG_PROGRAM_INFO:
		ri.Printf(printLevel, "Program info log:\n");
		nglGetProgramiv(programOrShader, GL_INFO_LOG_LENGTH, &maxLength);
		break;
	case GLSL_PRINTLOG_SHADER_INFO:
		ri.Printf(printLevel, "Shader info log:\n");
		nglGetShaderiv(programOrShader, GL_INFO_LOG_LENGTH, &maxLength);
		break;
	case GLSL_PRINTLOG_SHADER_SOURCE:
		ri.Printf(printLevel, "Shader source:\n");
		nglGetShaderiv(programOrShader, GL_SHADER_SOURCE_LENGTH, &maxLength);
		break;
	};

	if (maxLength <= 0) {
		ri.Printf(printLevel, "None.\n");
		return;
	}

	if ( maxLength < 1023 ) {
		msg = msgPart;
    } else {
		msg = alloca( maxLength );
    }

	switch (type) {
	case GLSL_PRINTLOG_PROGRAM_INFO:
		nglGetProgramInfoLog(programOrShader, maxLength, &maxLength, msg);
		break;
	case GLSL_PRINTLOG_SHADER_INFO:
		nglGetShaderInfoLog(programOrShader, maxLength, &maxLength, msg);
		break;
	case GLSL_PRINTLOG_SHADER_SOURCE:
		nglGetShaderSource(programOrShader, maxLength, &maxLength, msg);
		break;
	};

    ri.Printf(printLevel, "%s", msg);

	if (maxLength < 1023) {
		msgPart[maxLength + 1] = '\0';

		ri.Printf(printLevel, "%s\n", msgPart);
	}
	else {
		for(i = 0; i < maxLength; i += 1023) {
			N_strncpyz(msgPart, msg + i, sizeof(msgPart));

			ri.Printf(printLevel, "%s", msgPart);
		}

		ri.Printf(printLevel, "\n");
    }
}

static int GLSL_CompileGPUShader( GLuint program, GLuint *prevShader, const GLchar *buffer, uint64_t size, GLenum shaderType, const char *programName )
{
    GLuint compiled;
    GLuint shader;

    // create shader
    shader = nglCreateShader( shaderType );

    switch ( shaderType ) {
    case GL_VERTEX_SHADER:
        GL_SetObjectDebugName( GL_SHADER, shader, programName, "_vertexShader" );
        break;
    case GL_FRAGMENT_SHADER:
        GL_SetObjectDebugName( GL_SHADER, shader, programName, "_fragmentShader" );
        break;
    case GL_GEOMETRY_SHADER:
        GL_SetObjectDebugName( GL_SHADER, shader, programName, "_geometryShader" );
        break;
    case GL_TESS_CONTROL_SHADER:
        GL_SetObjectDebugName( GL_SHADER, shader, programName, "_tessControlShader" );
        break;
    case GL_TESS_EVALUATION_SHADER:
        GL_SetObjectDebugName( GL_SHADER, shader, programName, "_tessEvalShader" );
        break;
    default:
        ri.Error( ERR_FATAL, "GLSL_CompileGPUShader: invalid shader type %u", shaderType );
    };

    // give it the source
    nglShaderSource( shader, 1, (const GLchar **)&buffer, (const GLint *)&size );

    // compile
    nglCompileShader( shader );

    // check if shader compiled
    nglGetShaderiv( shader, GL_COMPILE_STATUS, &compiled );
    if ( !compiled ) {
        GLSL_PrintLog( shader, GLSL_PRINTLOG_SHADER_INFO, qfalse );
        ri.Error( ERR_DROP, "Failed to compiled shader" );
        return qfalse;
    }

    if (*prevShader) {
        nglDetachShader(program, *prevShader);
        nglDeleteShader(*prevShader);
    }

    // attach shader to program
    nglAttachShader(program, shader);

    *prevShader = shader;

    return qtrue;
}

static void GLSL_LinkProgram( GLuint program )
{
	GLint linked;

	nglLinkProgram( program );

	nglGetProgramiv( program, GL_LINK_STATUS, &linked );
	if( !linked ) {
		GLSL_PrintLog( program, GLSL_PRINTLOG_PROGRAM_INFO, qfalse );
		ri.Error( ERR_DROP, "shaders failed to link" );
	}
}

static int GLSL_LoadGPUShaderText(const char *name, const char *fallback, GLenum shaderType, char *dest, uint64_t destSize)
{
    char filename[MAX_GDR_PATH];
    GLchar *buffer = NULL;
    const GLchar *shaderText = NULL;
    uint64_t size;

    if (shaderType == GL_VERTEX_SHADER) {
        Com_snprintf(filename, sizeof(filename), "shaders/%s_vp.glsl", name);
    }
    else {
        Com_snprintf(filename, sizeof(filename), "shaders/%s_fp.glsl", name);
    }

    if (r_externalGLSL->i) {
        size = ri.FS_LoadFile(filename, (void **)&buffer);
    }
    else {
        size = 0;
        buffer = NULL;
    }

    if (!buffer) {
        if (fallback) {
            ri.Printf(PRINT_DEVELOPER, "...loading built-in '%s'\n", filename);
            shaderText = fallback;
            size = strlen(shaderText);
        }
        else {
            ri.Printf(PRINT_DEVELOPER, "couldn't load '%s'\n", filename);
            return qfalse;
        }
    }
    else {
        ri.Printf(PRINT_DEVELOPER, "...loaded '%s'\n", filename);
        shaderText = buffer;
    }

    if (size > destSize) {
        return qfalse;
    }

    N_strncpyz(dest, shaderText, size + 1);
    if (buffer)
        ri.FS_FreeFile(buffer);

    return qtrue;
}

static void GLSL_ShowProgramUniforms(GLuint program)
{
	uint32_t        i, count, size;
	GLenum			type;
	char            uniformName[1000];

	// query the number of active uniforms
	nglGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);

	// Loop over each of the active uniforms, and set their value
	for (i = 0; i < count; i++) {
		nglGetActiveUniform(program, i, sizeof(uniformName), NULL, &size, &type, uniformName);

//		ri.Printf(PRINT_DEVELOPER, "active uniform: '%s'\n", uniformName);
	}
}


static void GLSL_PrepareHeader(GLenum shaderType, const GLchar *extra, char *dest, uint64_t size)
{
    float fbufWidthScale, fbufHeightScale;

    dest[0] = '\0';

    // OpenGL version from 3.3 and up have corresponding glsl versions
    if (NGL_VERSION_ATLEAST(3, 30)) {
        N_strcat(dest, size, "#version 450 core\n" );
    }
    // otherwise, do the Quake3e method
    else if (GLSL_VERSION_ATLEAST(1, 30)) {
        if (GLSL_VERSION_ATLEAST(1, 50)) {
            N_strcat(dest, size, "#version 150\n");
        }
        else if (GLSL_VERSION_ATLEAST(1, 30)) {
            N_strcat(dest, size, "#verrsion 130\n");
        }
    }
    else {
        N_strcat(dest, size, "#define GLSL_LEGACY\n");
        N_strcat(dest, size, "#version 120\n");
    }

    if (!(NGL_VERSION_ATLEAST(1, 30))) {
        if (shaderType == GL_VERTEX_SHADER) {
            N_strcat(dest, size, "#define in attribute\n");
            N_strcat(dest, size, "#define out varying\n");
        }
        else {
            N_strcat(dest, size, "#define a_Color gl_FragColor\n");
            N_strcat(dest, size, "#define in varying\n");
            N_strcat(dest, size, "#define texture texture2D\n"); // texture2D is deprecated in modern GLSL
        }
    }

    N_strcat(dest, size, "#ifndef M_PI\n#define M_PI 3.14159265358979323846\n#endif\n");

    N_strcat(dest, size,
					 va("#ifndef deformGen_t\n"
						"#define deformGen_t\n"
						"#define DGEN_WAVE_SIN %i\n"
						"#define DGEN_WAVE_SQUARE %i\n"
						"#define DGEN_WAVE_TRIANGLE %i\n"
						"#define DGEN_WAVE_SAWTOOTH %i\n"
						"#define DGEN_WAVE_INVERSE_SAWTOOTH %i\n"
						"#define DGEN_BULGE %i\n"
						"#define DGEN_MOVE %i\n"
						"#endif\n",
						DGEN_WAVE_SIN,
						DGEN_WAVE_SQUARE,
						DGEN_WAVE_TRIANGLE,
						DGEN_WAVE_SAWTOOTH,
						DGEN_WAVE_INVERSE_SAWTOOTH,
						DGEN_BULGE,
						DGEN_MOVE));

	N_strcat(dest, size,
					 va("#ifndef tcGen_t\n"
						"#define tcGen_t\n"
						"#define TCGEN_LIGHTMAP %i\n"
						"#define TCGEN_TEXTURE %i\n"
						"#define TCGEN_ENVIRONMENT_MAPPED %i\n"
						"#define TCGEN_FOG %i\n"
						"#define TCGEN_VECTOR %i\n"
						"#endif\n",
						TCGEN_LIGHTMAP,
						TCGEN_TEXTURE,
						TCGEN_ENVIRONMENT_MAPPED,
						TCGEN_FOG,
						TCGEN_VECTOR));

	N_strcat(dest, size,
					 va("#ifndef colorGen_t\n"
						"#define colorGen_t\n"
						"#define CGEN_LIGHTING_DIFFUSE %i\n"
                        "#define CGEN_VERTEX %i\n"
						"#endif\n",
						CGEN_LIGHTING_DIFFUSE, CGEN_VERTEX ) );

	N_strcat(dest, size,
							 va("#ifndef alphaGen_t\n"
								"#define alphaGen_t\n"
								"#define AGEN_LIGHTING_SPECULAR %i\n"
								"#define AGEN_PORTAL %i\n"
								"#endif\n",
								AGEN_LIGHTING_SPECULAR,
								AGEN_PORTAL));

    fbufWidthScale = 1.0f / ((float)glConfig.vidWidth);
	fbufHeightScale = 1.0f / ((float)glConfig.vidHeight);
	N_strcat(dest, size,
			 va("#ifndef r_FBufScale\n#define r_FBufScale vec2(%f, %f)\n#endif\n", fbufWidthScale, fbufHeightScale));

    if ( r_pbr->i ) {
        N_strcat( dest, size, "#define USE_PBR\n" );
    }

    if ( extra ) {
        N_strcat( dest, size, extra );
    }

    // OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
	// so we have to reset the line counting
	N_strcat(dest, size, "#line 0\n");
}

static void GLSL_CheckAttribLocation(GLuint id, const char *name, const char *attribName, int index)
{
    GLint location;

    location = nglGetAttribLocation(id, name);
    if (location != index) {
        ri.Error(ERR_FATAL, "GetAttribLocation(%s):%i != %s (%i)", name, location, attribName, index);
    }
}

static int GLSL_InitGPUShader2(shaderProgram_t *program, const char *name, uint32_t attribs, const char *vsCode, const char *fsCode)
{
    ri.Printf(PRINT_DEVELOPER, "---------- GPU Shader ----------\n");

    if (strlen(name) >= MAX_GDR_PATH) {
        ri.Error(ERR_DROP, "GLSL_InitGPUShader2: \"%s\" is too long", name);
    }

    N_strncpyz(program->name, name, sizeof(program->name));

    program->programId = nglCreateProgram();
    program->attribBits = attribs;

    GL_SetObjectDebugName(GL_PROGRAM, program->programId, name, "_program");

    if (!(GLSL_CompileGPUShader(program->programId, &program->vertexId, vsCode, strlen(vsCode), GL_VERTEX_SHADER, name))) {
        ri.Printf(PRINT_INFO, "GLSL_InitGPUShader2: Unable to load \"%s\" as GL_VERTEX_SHADER\n", name);
        nglDeleteProgram(program->programId);
        return qfalse;
    }

    if (fsCode) {
        if (!(GLSL_CompileGPUShader(program->programId, &program->fragmentId, fsCode, strlen(fsCode), GL_FRAGMENT_SHADER, name))) {
            ri.Printf(PRINT_INFO, "GLSL_InitGPUShader2: Unable to load \"%s\" as GL_FRAGMENT_SHADER\n", name);
            nglDeleteProgram(program->programId);
            return qfalse;
        }
    }

    if ( attribs & ATTRIB_NORMAL ) {
        nglBindAttribLocation( program->programId, ATTRIB_INDEX_NORMAL, "a_Normal" );
    }
    if ( attribs & ATTRIB_POSITION ) {
        nglBindAttribLocation( program->programId, ATTRIB_INDEX_POSITION, "a_Position" );
    }
    if ( attribs & ATTRIB_TANGENT ) {
        nglBindAttribLocation( program->programId, ATTRIB_INDEX_TANGENT, "a_Tangent" );
    }
    if ( attribs & ATTRIB_LIGHTCOORD ) {
        nglBindAttribLocation( program->programId, ATTRIB_INDEX_LIGHTCOORD, "a_LightCoords" );
    }
    if ( attribs & ATTRIB_TEXCOORD ) {
        nglBindAttribLocation( program->programId, ATTRIB_INDEX_TEXCOORD, "a_TexCoords" );
    }
    if ( attribs & ATTRIB_COLOR ) {
        nglBindAttribLocation( program->programId, ATTRIB_INDEX_COLOR, "a_Color" );
    }
    if ( attribs & ATTRIB_WORLDPOS ) {
        nglBindAttribLocation( program->programId, ATTRIB_INDEX_WORLDPOS, "a_WorldPos" );
    }
    
    GLSL_LinkProgram(program->programId);

    return qtrue;
}

static int GLSL_InitGPUShader(shaderProgram_t *program, const char *name, uint32_t attribs, qboolean fragmentShader,
    const GLchar *extra, qboolean addHeader, const char *fallback_vs, const char *fallback_fs)
{
    char vsCode[32000];
    char fsCode[32000];
    char *postHeader;
    uint64_t size;

    rg.programs[rg.numPrograms] = program;
    rg.numPrograms++;

    size = sizeof(vsCode);

    if (addHeader) {
        GLSL_PrepareHeader(GL_VERTEX_SHADER, extra, vsCode, size);
        postHeader = &vsCode[strlen(vsCode)];
        size -= strlen(vsCode);
    }
    else {
        postHeader = &vsCode[0];
    }

    if (!GLSL_LoadGPUShaderText(name, fallback_vs, GL_VERTEX_SHADER, postHeader, size)) {
        return qfalse;
    }

    if (fragmentShader) {
		size = sizeof(fsCode);

		if (addHeader) {
			GLSL_PrepareHeader(GL_FRAGMENT_SHADER, extra, fsCode, size);
			postHeader = &fsCode[strlen(fsCode)];
			size -= strlen(fsCode);
		}
		else {
			postHeader = &fsCode[0];
		}

		if (!GLSL_LoadGPUShaderText(name, fallback_fs, GL_FRAGMENT_SHADER, postHeader, size)) {
			return qfalse;
		}
	}

    return GLSL_InitGPUShader2(program, name, attribs, vsCode, fsCode);
}

static void GLSL_InitUniforms( shaderProgram_t *program )
{
	uint32_t i, uniformBufferSize;

	GLint *uniforms = program->uniforms;
	uniformBufferSize = 0;

	for ( i = 0; i < UNIFORM_COUNT; i++ ) {
        uniforms[i] = nglGetUniformLocation( program->programId, uniformsInfo[i].name );

    	if ( uniforms[i] == -1 ) {
    		continue;
        }

		program->uniformBufferOffsets[i] = uniformBufferSize;
 
		switch ( uniformsInfo[i].type ) {
		case GLSL_INT:
			uniformBufferSize += sizeof( GLint );
			break;
		case GLSL_FLOAT:
			uniformBufferSize += sizeof( GLfloat );
			break;
		case GLSL_VEC2:
			uniformBufferSize += sizeof( vec2_t );
			break;
		case GLSL_VEC3:
			uniformBufferSize += sizeof( vec3_t );
			break;
		case GLSL_VEC4:
			uniformBufferSize += sizeof( vec4_t );
			break;
        case GLSL_VEC5:
            uniformBufferSize += sizeof( vec_t ) * 5;
            break;
		case GLSL_MAT16:
			uniformBufferSize += sizeof( mat4_t );
			break;
        case GLSL_BUFFER:
            // we store the block index instead of the data
            uniformBufferSize += sizeof( GLuint );
            break;
		default:
			break;
		};
	}

	program->uniformBuffer = (char *)ri.Malloc( uniformBufferSize );

    for ( i = 0; i < UNIFORM_COUNT; i++ ) {
        if ( program->uniforms[i] != -1 && uniformsInfo[i].type == GLSL_BUFFER ) {
            *( (GLuint *)( program->uniformBuffer + program->uniformBufferOffsets[ i ] ) ) =
                nglGetUniformBlockIndex( program->programId, uniformsInfo[i].name );
        }
    }
}

static void GLSL_FinishGPUShader(shaderProgram_t *program)
{
	GLSL_ShowProgramUniforms(program->programId);
	GL_CheckErrors();
}

static void GLSL_DeleteGPUShader(shaderProgram_t *program)
{
    if ( program->programId ) {
        if ( program->vertexId ) {
            nglDetachShader( program->programId, program->vertexId );
            nglDeleteShader( program->vertexId );
        }
        if ( program->fragmentId ) {
            nglDetachShader( program->programId, program->fragmentId );
            nglDeleteShader( program->fragmentId );
        }

        nglDeleteProgram( program->programId );
    }
}

void GLSL_SetUniformInt(shaderProgram_t *program, uint32_t uniformNum, GLint value)
{
    GLint *uniforms = program->uniforms;
    GLint *compare = (GLint *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

    if (uniforms[uniformNum] == -1)
        return;
    
    if (uniformsInfo[uniformNum].type != GLSL_INT) {
        ri.Printf(PRINT_INFO, COLOR_YELLOW "WARNING: GLSL_SetUniformInt: wrong type for uniform %i in program %s\n", uniformNum, program->name);
        return;
    }
    if (value == *compare)
        return;
    
    *compare = value;
    nglUniform1i(uniforms[uniformNum], value);
}

void GLSL_SetUniformFloat(shaderProgram_t *program, uint32_t uniformNum, GLfloat value)
{
    GLint *uniforms = program->uniforms;
    GLfloat *compare = (GLfloat *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

    if (uniforms[uniformNum] == -1)
        return;
    
    if (uniformsInfo[uniformNum].type != GLSL_FLOAT) {
        ri.Printf(PRINT_INFO, COLOR_YELLOW "WARNING: GLSL_SetUniformFloat: wrong type for uniform %i in program %s\n", uniformNum, program->name);
        return;
    }
    if (value == *compare)
        return;
    
    *compare = value;
    nglUniform1f(uniforms[uniformNum], value);
}

void GLSL_SetUniformVec2(shaderProgram_t *program, uint32_t uniformNum, const vec2_t v)
{
    GLint *uniforms = program->uniforms;
    vec_t *compare = (vec_t *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

    if (uniforms[uniformNum] == -1)
        return;
    
    if (uniformsInfo[uniformNum].type != GLSL_VEC2) {
        ri.Printf(PRINT_INFO, COLOR_YELLOW "WARNING: GLSL_SetUniformVec2: wrong type for uniform %i in program %s\n", uniformNum, program->name);
        return;
    }
    if (v[0] == compare[0] && v[1] == compare[1])
        return;
    
    compare[0] = v[0];
    compare[1] = v[1];
    nglUniform2f(uniforms[uniformNum], v[0], v[1]);
}

void GLSL_SetUniformVec3(shaderProgram_t *program, uint32_t uniformNum, const vec3_t v)
{
    GLint *uniforms = program->uniforms;
    vec_t *compare = (vec_t *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

    if (uniforms[uniformNum] == -1)
        return;
    
    if (uniformsInfo[uniformNum].type != GLSL_VEC3) {
        ri.Printf(PRINT_INFO, COLOR_YELLOW "WARNING: GLSL_SetUniformVec3: wrong type for uniform %i in program %s\n", uniformNum, program->name);
        return;
    }
    if (VectorCompare(v, compare))
        return;
    
    VectorCopy(compare, v);
    nglUniform3f(uniforms[uniformNum], v[0], v[1], v[2]);
}

void GLSL_SetUniformVec4(shaderProgram_t *program, uint32_t uniformNum, const vec4_t v)
{
    GLint *uniforms = program->uniforms;
    vec_t *compare = (vec_t *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

    if (uniforms[uniformNum] == -1)
        return;
    
    if (uniformsInfo[uniformNum].type != GLSL_VEC4) {
        ri.Printf(PRINT_INFO, COLOR_YELLOW "WARNING: GLSL_SetUniformVec4: wrong type for uniform %i in program %s\n", uniformNum, program->name);
        return;
    }
    if ( VectorCompare4( v, compare ) )
        return;
    
    VectorCopy4( compare, v );
    nglUniform4f(uniforms[uniformNum], v[0], v[1], v[2], v[3]);
}

void GLSL_SetUniformVec5( shaderProgram_t *program, uint32_t uniformNum, const float *v )
{
    GLint *uniforms = program->uniforms;
    vec_t *compare = (vec_t *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

    if (uniforms[uniformNum] == -1)
        return;
    
    if ( uniformsInfo[uniformNum].type != GLSL_VEC5 ) {
        ri.Printf(PRINT_INFO, COLOR_YELLOW "WARNING: GLSL_SetUniformVec5: wrong type for uniform %i in program %s\n", uniformNum, program->name);
        return;
    }
    if ( !memcmp( compare, v, sizeof(vec_t) * 5 ) )
        return;
    
    memcpy( compare, v, sizeof(vec_t) * 5 );
    nglUniform1fv(uniforms[uniformNum], 5, v);
}

void GLSL_SetUniformMatrix4(shaderProgram_t *program, uint32_t uniformNum, const mat4_t m)
{
    GLint *uniforms = program->uniforms;
    mat4_t *compare = (mat4_t *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

    if (uniforms[uniformNum] == -1)
        return;
    
    if (uniformsInfo[uniformNum].type != GLSL_MAT16) {
        ri.Printf(PRINT_INFO, COLOR_YELLOW "WARNING: GLSL_SetUniformMatrix4: wrong type for uniform %i in program %s\n", uniformNum, program->name);
        return;
    }
    if (Mat4Compare(m, *compare))
        return;
    
    memcpy( *compare, m, sizeof(mat4_t) );
    nglUniformMatrix4fv(uniforms[uniformNum], 1, GL_FALSE, &m[0][0]);
}

void GLSL_ShaderBufferData( shaderProgram_t *shader, uint32_t uniformNum, uniformBuffer_t *buffer )
{
    GLint *uniforms;
    GLuint bufferObject;
    GLenum target;

    GLSL_UseProgram( shader );

    uniforms = shader->uniforms;
    bufferObject = buffer ? buffer->id : 0;
    target = r_arb_shader_storage_buffer_object->i ? GL_SHADER_STORAGE_BUFFER : GL_UNIFORM_BUFFER;

    if ( uniforms[ uniformNum ] == -1 ) {
        return;
    }

    if ( uniformsInfo[ uniformNum ].type != GLSL_BUFFER ) {
        ri.Printf( PRINT_INFO, COLOR_YELLOW "WARNING: GLSL_BindShaderBuffer: wrong type for uniform %i in program %s\n", uniformNum, shader->name );
        return;
    }

    if ( glState.uboId != bufferObject ) {
        nglBindBufferARB( target, bufferObject );
        glState.uboId = bufferObject;
        glState.currentUbo = buffer;
    }

    if ( buffer ) {
        nglBufferSubDataARB( target, 0, buffer->size, buffer->data );
    }
}

static void GLSL_LinkUniformToShader( shaderProgram_t *program, uint32_t uniformNum, uniformBuffer_t *buffer )
{
    GLint *uniforms = program->uniforms;
    GLuint *compare = (GLuint *)( program->uniformBuffer + program->uniformBufferOffsets[ uniformNum ] );
    GLenum target;

    if ( r_arb_shader_storage_buffer_object->i ) {
        target = GL_SHADER_STORAGE_BUFFER;
    } else {
        target = GL_UNIFORM_BUFFER;
    }

    if ( uniforms[ uniformNum ] == -1 ) {
        ri.Printf( PRINT_WARNING, "GLSL_LinkUniformToShader: uniformBuffer %s not in program '%s'\n", buffer->name, program->name );
        return;
    }

    GLSL_UseProgram( program );
    nglBindBufferARB( target, buffer->id );

    buffer->binding = *compare;
    nglUniformBlockBinding( program->programId, buffer->binding, program->numBuffers );
    nglBindBufferRangeARB( target, 0, buffer->id, 0, buffer->size );
    nglBindBufferBaseARB( target, 0, buffer->id );

    nglBindBufferARB( target, 0 );
    GL_CheckErrors();
    
    GLSL_UseProgram( NULL );

    program->numBuffers++;

    ri.Printf( PRINT_DEVELOPER, "GLSL_LinkUniformToShader: linked uniform buffer object '%s' to GLSL program '%s'\n",
        buffer->name, program->name );
}

uniformBuffer_t *GLSL_InitUniformBuffer( const char *name, byte *buffer, uint64_t bufSize )
{
    uint64_t size;
    uint64_t nameLen;
    GLenum target;
    uniformBuffer_t *buf;

    nameLen = strlen( name );
    if ( nameLen >= MAX_NPATH ) {
        ri.Error( ERR_FATAL, "GLSL_InitUniformBuffer: name '%s' too long", name );
    }

    // this should happen
    if ( rg.numUniformBuffers == MAX_UNIFORM_BUFFERS ) {
        ri.Error( ERR_FATAL, "GLSL_InitUniformBuffer: MAX_UNIFORM_BUFFERS hit" );
    }

    size = 0;
    size += PAD( nameLen + 1, sizeof( uintptr_t ) );
    size += PAD( sizeof( *buf ), sizeof( uintptr_t ) );

    buf = rg.uniformBuffers[ rg.numUniformBuffers ] = (uniformBuffer_t *)ri.Hunk_Alloc( size, h_low );
    memset( buf, 0, size );

    buf->name = (char *)( buf + 1 );
    if ( !buffer ) {
        buf->data = (byte *)ri.Malloc( bufSize );
    } else {
        buf->data = buffer;
    }
    buf->size = bufSize;
    strcpy( buf->name, name );

    buf->externalBuffer = buffer != NULL;

    if ( r_arb_shader_storage_buffer_object->i ) {
        target = GL_SHADER_STORAGE_BUFFER;
    } else {
        target = GL_UNIFORM_BUFFER;
    }

    // generate buffer
    nglGenBuffersARB( 1, &buf->id );
    nglBindBufferARB( target, buf->id );

    nglBufferDataARB( target, bufSize, buffer, GL_STATIC_DRAW );

    nglBindBufferARB( target, 0 );

    GLSL_UseProgram( NULL );

    rg.numUniformBuffers++;

    return buf;
}

void GLSL_InitGPUShaders( void )
{
    uint64_t start, end;
    uint64_t i;
    uint32_t attribs;
    uint32_t numEtcShaders = 0, numGenShaders = 0, numLightShaders = 0;
    char extradefines[MAX_STRING_CHARS];

    rg.numPrograms = 0;

    ri.Printf( PRINT_INFO, "---- GLSL_InitGPUShaders ----\n" );

    rg.lightData = GLSL_InitUniformBuffer( "u_LightBuffer", NULL, sizeof( shaderLight_t ) * MAX_MAP_LIGHTS );

    R_IssuePendingRenderCommands();

    start = ri.Milliseconds();

    for ( i = 0; i < GENERICDEF_COUNT; i++ ) {
        attribs = ATTRIB_POSITION | ATTRIB_TEXCOORD | ATTRIB_COLOR;

        extradefines[0] = '\0';

        N_strcat( extradefines, sizeof( extradefines ) - 1, va( "#define MAX_MAP_LIGHTS %i\n", MAX_MAP_LIGHTS ) );
        N_strcat( extradefines, sizeof( extradefines ) - 1, va( "#define POINT_LIGHT %i\n", LIGHT_POINT ) );
        N_strcat( extradefines, sizeof( extradefines ) - 1, va( "#define DIRECTION_LIGHT %i\n", LIGHT_DIRECTIONAL ) );

        if ( i & GENERICDEF_USE_DEFORM_VERTEXES ) {
            N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_DEFORM_VERTEXES\n" );
        }
        if ( i & GENERICDEF_USE_TCGEN_AND_TCMOD ) {
            N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_TCMOD\n" );
            N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_TCGEN\n" );
        }

//        if ( i & GENERICDEF_USE_RGBAGEN ) {
            N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_RGBAGEN\n" );
//        }
        if ( r_bloom->i ) {
            N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_BLOOM\n" );
        }
        if ( r_toneMap->i && r_toneMapType->i == 1 ) {
            N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_EXPOSURE_TONE_MAPPING\n" );
        }
        if ( r_hdr->i ) {
            N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_HDR\n" );
        }

        if ( !GLSL_InitGPUShader( &rg.genericShader[i], "generic", attribs, qtrue, extradefines, qtrue, fallbackShader_generic_vp, fallbackShader_generic_fp ) ) {
            ri.Error( ERR_FATAL, "Could not load generic shader!" );
        }

        GLSL_InitUniforms( &rg.genericShader[i] );

        GLSL_LinkUniformToShader( &rg.genericShader[i], UNIFORM_LIGHTDATA, rg.lightData );

        GLSL_FinishGPUShader( &rg.genericShader[i] );

        numGenShaders++;
    }

    attribs = ATTRIB_POSITION | ATTRIB_TEXCOORD;
    if ( !GLSL_InitGPUShader( &rg.textureColorShader, "texturecolor", attribs, qtrue, extradefines, qtrue, fallbackShader_texturecolor_vp, fallbackShader_texturecolor_fp ) ) {
        ri.Error( ERR_FATAL, "Could not load texturecolor shader!" );
    }
    GLSL_InitUniforms( &rg.textureColorShader );
    GLSL_FinishGPUShader( &rg.textureColorShader );

    for ( i = 0; i < LIGHTDEF_COUNT; i++ ) {
        int lightType = i & LIGHTDEF_LIGHTTYPE_MASK;
        qboolean fastLight = !( r_normalMapping->i || r_specularMapping->i || r_bloom->i );

        // skip impossible combos
		if ( ( i & LIGHTDEF_USE_PARALLAXMAP ) && !r_parallaxMapping->i ) {
			continue;
        }

		if ( ( i & LIGHTDEF_USE_SHADOWMAP ) && ( !lightType || !r_sunlightMode->i ) ) {
			continue;
        }

        attribs = ATTRIB_POSITION | ATTRIB_TEXCOORD | ATTRIB_COLOR;
        extradefines[0] = '\0';

        N_strcat( extradefines, sizeof( extradefines ) - 1, va( "#define MAX_MAP_LIGHTS %i\n", MAX_MAP_LIGHTS ) );
        N_strcat( extradefines, sizeof( extradefines ) - 1, va( "#define POINT_LIGHT %i\n", LIGHT_POINT ) );
        N_strcat( extradefines, sizeof( extradefines ) - 1, va( "#define DIRECTION_LIGHT %i\n", LIGHT_DIRECTIONAL ) );

        if ( r_dlightMode->i >= 2 ) {
            N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_SHADOWMAP\n" );
        }
        if ( glContext.swizzleNormalmap ) {
            N_strcat( extradefines, sizeof( extradefines ) - 1, "#define SWIZZLE_NORMALMAP\n" );
        }

        if ( lightType ) {
            N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_LIGHT\n" );
            if ( fastLight ) {
                N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_FAST_LIGHT\n" );
            }

            if ( r_bloom->i && r_postProcess->i && r_arb_framebuffer_object->i ) {
                N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_BLOOM\n" );
            }
            if ( r_toneMap->i && r_toneMapType->i == 1 ) {
                N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_EXPOSURE_TONE_MAPPING\n" );
            }
            if ( r_hdr->i && r_postProcess->i && r_arb_framebuffer_object->i ) {
                N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_HDR\n" );
            }

            switch ( lightType ) {
            case LIGHTDEF_USE_LIGHTMAP: {
                N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_LIGHTMAP\n" );
                if ( r_deluxeMapping->i && !fastLight ) {
                    N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_DELUXEMAP\n" );
                }
                break; }
            case LIGHTDEF_USE_LIGHT_VECTOR:
                N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_LIGHT_VECTOR\n" );
                break;
            case LIGHTDEF_USE_LIGHT_VERTEX:
                N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_LIGHT_VERTEX\n" );
                break;
            default:
                break;
            };

            if ( r_multisampleType->i == AntiAlias_FXAA ) {
                N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_FXAA\n" );
            }

            if ( r_normalMapping->i ) {
    			N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_NORMALMAP\n" );

    			attribs |= ATTRIB_TANGENT;
    			
                if ( ( i & LIGHTDEF_USE_PARALLAXMAP ) && r_parallaxMapping->i ) {
    				N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_PARALLAXMAP\n" );
    				if ( r_parallaxMapping->i > 1 ) {
    					N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_RELIEFMAP\n" );
                    }
    			    if ( r_parallaxMapShadows->i ) {
    					N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_PARALLAXMAP_SHADOWS\n" );
                    }
    				N_strcat( extradefines, sizeof( extradefines ) - 1, va( "#define r_parallaxMapOffset %f\n", r_parallaxMapOffset->f ) );
    			}
    		}
    		if ( r_specularMapping->i ) {
    			N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_SPECULARMAP\n" );
            }
            if ( r_deluxeSpecular->f > 0.000001f ) {
                N_strcat( extradefines, sizeof( extradefines ) - 1, va( "#define r_deluxeSpecular %f\n", r_deluxeSpecular->f ) );
            }

            switch ( r_glossType->i ) {
			case 0:
			default:
				N_strcat( extradefines, sizeof( extradefines ) - 1, "#define GLOSS_IS_GLOSS\n" );
				break;
			case 1:
				N_strcat( extradefines, sizeof( extradefines ) - 1, "#define GLOSS_IS_SMOOTHNESS\n" );
				break;
			case 2:
				N_strcat( extradefines, sizeof( extradefines ) - 1, "#define GLOSS_IS_ROUGHNESS\n" );
				break;
			case 3:
			    N_strcat( extradefines, sizeof( extradefines ) - 1, "#define GLOSS_IS_SHININESS\n" );
				break;
			};
        }

        if ( i & LIGHTDEF_USE_SHADOWMAP ) {
			N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_SHADOWMAP\n" );

			if ( r_sunlightMode->i == 1 ) {
				N_strcat( extradefines, sizeof( extradefines ) - 1, "#define SHADOWMAP_MODULATE\n" );
            } else if ( r_sunlightMode->i == 2 ) {
				N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_PRIMARY_LIGHT\n" );
            }
		}

		if ( i & LIGHTDEF_USE_TCGEN_AND_TCMOD ) {
			N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_TCGEN\n" );
			N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_TCMOD\n" );
		}

        if ( !GLSL_InitGPUShader( &rg.lightallShader[i], "lightall", attribs, qtrue, extradefines, qtrue, fallbackShader_lightall_vp, fallbackShader_lightall_fp ) ) {
            ri.Error( ERR_FATAL, "Could not load lightall shader!" );
        }

        GLSL_InitUniforms( &rg.lightallShader[i] );

        GLSL_LinkUniformToShader( &rg.lightallShader[i], UNIFORM_LIGHTDATA, rg.lightData );

        GLSL_FinishGPUShader( &rg.lightallShader[i] );

        numLightShaders++;
    }

    attribs = ATTRIB_POSITION | ATTRIB_TEXCOORD | ATTRIB_COLOR;

    extradefines[0] = '\0';
    if ( r_multisampleType->i == AntiAlias_FXAA ) {
        N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_FXAA\n" );
    }
    N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_TCGEN\n" );
    N_strcat( extradefines, sizeof( extradefines ) - 1, "#define USE_TCMOD\n" );
    if ( !GLSL_InitGPUShader( &rg.imguiShader, "imgui", attribs, qtrue, extradefines, qtrue, fallbackShader_imgui_vp, fallbackShader_imgui_fp ) ) {
        ri.Error( ERR_FATAL, "Could not load imgui shader!" );
    }
    GLSL_InitUniforms( &rg.imguiShader );
    GLSL_FinishGPUShader( &rg.imguiShader );
    numGenShaders++;

    attribs = ATTRIB_POSITION | ATTRIB_NORMAL | ATTRIB_TANGENT | ATTRIB_TEXCOORD | ATTRIB_COLOR | ATTRIB_WORLDPOS;

    extradefines[0] = '\0';
    N_strcat( extradefines, sizeof( extradefines ) - 1, va( "#define MAX_MAP_LIGHTS %i\n", MAX_MAP_LIGHTS ) );
    N_strcat( extradefines, sizeof( extradefines ) - 1, va( "#define POINT_LIGHT %i\n", LIGHT_POINT ) );
    N_strcat( extradefines, sizeof( extradefines ) - 1, va( "#define DIRECTION_LIGHT %i\n", LIGHT_DIRECTIONAL ) );
    if ( !GLSL_InitGPUShader( &rg.tileShader, "tile", attribs, qtrue, NULL, qtrue, fallbackShader_tile_vp, fallbackShader_tile_fp ) ) {
        ri.Error( ERR_FATAL, "Could not load tile shader!" );
    }
    GLSL_InitUniforms( &rg.tileShader );

    GLSL_LinkUniformToShader( &rg.tileShader, UNIFORM_LIGHTDATA, rg.lightData );

    GLSL_FinishGPUShader( &rg.tileShader );
    numGenShaders++;

    attribs = ATTRIB_POSITION | ATTRIB_TEXCOORD;
    if ( !GLSL_InitGPUShader( &rg.down4xShader, "down4x", attribs, qtrue, NULL, qtrue, fallbackShader_down4x_vp, fallbackShader_down4x_fp ) ) {
        ri.Error( ERR_FATAL, "Could not load down4x shader!" );
    }
    GLSL_InitUniforms( &rg.down4xShader );
    GLSL_FinishGPUShader( &rg.down4xShader );
    numEtcShaders++;

    attribs = ATTRIB_POSITION | ATTRIB_TEXCOORD;
    if ( !GLSL_InitGPUShader( &rg.bokehShader, "bokeh", attribs, qtrue, NULL, qtrue, fallbackShader_bokeh_vp, fallbackShader_bokeh_fp ) ) {
        ri.Error( ERR_FATAL, "Could not load bokeh shader!" );
    }
    GLSL_InitUniforms( &rg.bokehShader );
    GLSL_FinishGPUShader( &rg.bokehShader );

    numEtcShaders++;

    attribs = ATTRIB_POSITION | ATTRIB_TEXCOORD;
    if ( !GLSL_InitGPUShader( &rg.tonemapShader, "tonemap", attribs, qtrue, NULL, qtrue, fallbackShader_tonemap_vp, fallbackShader_tonemap_fp ) ) {
        ri.Error( ERR_FATAL, "Could not load tonemap shader!" );
    }
    GLSL_InitUniforms( &rg.tonemapShader );
    GLSL_FinishGPUShader( &rg.tonemapShader );

    numEtcShaders++;

    for ( i = 0; i < 2; i++ ) {
        attribs = ATTRIB_POSITION | ATTRIB_TEXCOORD;

        extradefines[0] = '\0';
        if ( !i ) {
            N_strcat( extradefines, sizeof( extradefines ), "#define FIRST_PASS\n" );
        }

        if ( !GLSL_InitGPUShader( &rg.calclevels4xShader[i], "calclevels4x", attribs, qtrue, extradefines, qtrue, fallbackShader_calclevels4x_vp, fallbackShader_calclevels4x_fp ) ) {
            ri.Error( ERR_FATAL, "Could not load calclevels4x shader!" );
        }

        GLSL_InitUniforms( &rg.calclevels4xShader[i] );
        GLSL_FinishGPUShader( &rg.calclevels4xShader[i] );
        numEtcShaders++;
    }

    for ( i = 0; i < 4; i++ ) {
        attribs = ATTRIB_POSITION | ATTRIB_TEXCOORD;
        extradefines[0] = '\0';

        if ( i & 1 ) {
            N_strcat( extradefines, sizeof( extradefines ), "#define USE_VERTICAL_BLUR\n" );
        } else {
            N_strcat( extradefines, sizeof( extradefines ), "#define USE_HORIZONTAL_BLUR\n" );
        }

        if ( !( i & 2 ) ) {
            N_strcat( extradefines, sizeof( extradefines ), "#define USE_DEPTH\n" );
        }

        if ( !GLSL_InitGPUShader( &rg.depthBlurShader[i], "depthblur", attribs, qtrue, extradefines, qtrue, fallbackShader_depthblur_vp, fallbackShader_depthblur_fp ) ) {
            ri.Error( ERR_FATAL, "Could not load depthBlur shader!" );
        }

        GLSL_InitUniforms( &rg.depthBlurShader[i] );
        GLSL_FinishGPUShader( &rg.depthBlurShader[i] );

        numEtcShaders++;
    }

    end = ri.Milliseconds();

    ri.Printf(PRINT_INFO, "...loaded %u GLSL shaders (%u gen %u etc %u light) in %5.2f seconds\n",
        rg.numPrograms, numGenShaders, numEtcShaders, numLightShaders, ( end - start ) / 1000.0);
}

void GLSL_ShutdownGPUShaders(void)
{
    uint32_t i;

    ri.Printf(PRINT_INFO, "---------- GLSL_ShutdownGPUShaders -----------\n");

    for ( i = 0; i < ATTRIB_INDEX_COUNT; i++ ) {
        nglDisableVertexAttribArray( i );
    }
    
    GL_BindNullProgram();
    nglBindBufferARB( r_arb_shader_storage_buffer_object->i ? GL_SHADER_STORAGE_BUFFER : GL_UNIFORM_BUFFER,  0 );

    GLSL_DeleteGPUShader( &rg.imguiShader );
    GLSL_DeleteGPUShader( &rg.tileShader );
    GLSL_DeleteGPUShader( &rg.tonemapShader );
    GLSL_DeleteGPUShader( &rg.bokehShader );
    GLSL_DeleteGPUShader( &rg.down4xShader );

    for ( i = 0; i < rg.numUniformBuffers; i++ ) {
        nglDeleteBuffersARB( 1, &rg.uniformBuffers[i]->id );
    }
    for ( i = 0; i < GENERICDEF_COUNT; i++ ) {
        GLSL_DeleteGPUShader( &rg.genericShader[i] );
    }
    for ( i = 0; i < LIGHTDEF_COUNT; i++ ) {
        GLSL_DeleteGPUShader( &rg.lightallShader[i] );
    }
    for ( i = 0; i < 4; i++ ) {
        GLSL_DeleteGPUShader( &rg.depthBlurShader[i] );
    }
    for ( i = 0; i < 2; i++ ) {
        GLSL_DeleteGPUShader( &rg.calclevels4xShader[i] );
    }
}

void GLSL_UseProgram(shaderProgram_t *program)
{
    GLuint programObject = program ? program->programId : 0;

    if (GL_UseProgram(programObject)) {
        backend.pc.c_glslShaderBinds++;
        glState.currentShader = program;
    }
}

shaderProgram_t *GLSL_GetGenericShaderProgram(int stage)
{
	const shaderStage_t *pStage = backend.drawBatch.shader->stages[stage];
	int shaderAttribs = 0;

	switch (pStage->rgbGen) {
	case CGEN_LIGHTING_DIFFUSE:
		shaderAttribs |= GENERICDEF_USE_RGBAGEN;
		break;
	default:
		break;
	};

	switch (pStage->alphaGen) {
	case AGEN_LIGHTING_SPECULAR:
	case AGEN_PORTAL:
		shaderAttribs |= GENERICDEF_USE_RGBAGEN;
		break;
	default:
		break;
	};

    if ( backend.drawBatch.shader->numDeforms == 0 && pStage->bundle[0].numTexMods == 0 ) {
        return &rg.genericShader[shaderAttribs];
    }

	if ( pStage->bundle[0].tcGen != TCGEN_TEXTURE )
	{
		shaderAttribs |= GENERICDEF_USE_TCGEN_AND_TCMOD;
	}

	if (backend.drawBatch.shader->numDeforms && !ShaderRequiresCPUDeforms(backend.drawBatch.shader))
	{
		shaderAttribs |= GENERICDEF_USE_DEFORM_VERTEXES;
	}

	if (pStage->bundle[0].numTexMods)
	{
		shaderAttribs |= GENERICDEF_USE_TCGEN_AND_TCMOD;
	}

	return &rg.genericShader[shaderAttribs];
}
