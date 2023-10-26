#ifndef _RGL_LOCAL_
#define _RGL_LOCAL_

#pragma once

#include "../engine/n_shared.h"
#include "../engine/gln_files.h"
#include "ngl.h"
#include "../rendercommon/r_public.h"
#include "../rendercommon/r_types.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define GLN_INDEX_TYPE GL_UNSIGNED_INT
typedef uint32_t glIndex_t;

#define MAX_UNIFORM_BUFFERS 512
#define MAX_VERTEX_BUFFERS 1024
#define MAX_FRAMEBUFFERS 64
#define MAX_GLSL_OBJECTS 128
#define MAX_SHADERS 1024
#define MAX_TEXTURES 1024

#define PRINT_INFO 0
#define PRINT_DEVELOPER 1

#define PSHADOW_MAP_SIZE 512

#define NGL_VERSION_ATLEAST(major,minor) (glContext->versionMajor > major || (glContext->versionMajor == major && glContext->versionMinor >= minor))

#define MAX_DRAW_VERTICES (MAX_INT/sizeof(drawVert_t))
#define MAX_DRAW_INDICES (MAX_INT/sizeof(glIndex_t))

// per drawcall batch
#define MAX_BATCH_QUADS 4096
#define MAX_BATCH_VERTICES (MAX_BATCH_QUADS*4)
#define MAX_BATCH_INDICES (MAX_BATCH_QUADS*6)

// any change in the LIGHTMAP_* defines here MUST be reflected in
// R_FindShader() in tr_bsp.c
#define LIGHTMAP_2D         -4	// shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX  -3	// pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE -2
#define LIGHTMAP_NONE       -1

typedef enum {
    MI_NONE,
    MI_NVX,
    MI_ATI
} gpuMemInfo_t;

typedef enum {
	TCR_NONE = 0x0000,
	TCR_RGTC = 0x0001,
	TCR_BPTC = 0x0002,
} textureCompressionRef_t;

typedef enum {
    GL_DBG_KHR,
    GL_DBG_AMD,
    GL_DBG_ARB,
} glDebugType_t;

typedef struct
{
    char vendor[1024];
    char renderer[1024];
    char version_str[1024];
    char glsl_version_str[1024];
    char extensions[8192];
    float version_f;
    int versionMajor, versionMinor;
    int glslVersionMajor, glslVersionMinor;
    int numExtensions;
    glTextureCompression_t textureCompression;
    textureCompressionRef_t textureCompressionRef;
    gpuMemInfo_t memInfo;
    glDebugType_t debugType;
    qboolean nonPowerOfTwoTextures;
    qboolean stereo;
    qboolean intelGraphics;
    qboolean swizzleNormalmap;

    int maxTextureUnits;
    int maxTextureSize;
    int vboTarget;
    int iboTarget;
    int maxSamples;
    int maxColorAttachments;
    int maxRenderBufferSize;

    float maxAnisotropy;

    qboolean ARB_gl_spirv;
    qboolean ARB_texture_filter_anisotropic;
    qboolean ARB_vertex_buffer_object;
    qboolean ARB_buffer_storage;
    qboolean ARB_map_buffer_range;
    qboolean ARB_texture_float;
    qboolean ARB_vertex_array_object;
    qboolean ARB_framebuffer_object;
    qboolean ARB_framebuffer_sRGB;
    qboolean ARB_framebuffer_blit;
    qboolean ARB_framebuffer_multisample;
    qboolean ARB_vertex_shader;
    qboolean ARB_texture_compression;
} glContext_t;

typedef struct {
    refEntity_t e;
    vec3_t ambientLight;
    vec3_t lightDir;
    vec3_t directedLight;
} renderEntityDef_t;

typedef enum
{
	IMGTYPE_COLORALPHA, // for color, lightmap, diffuse, and specular
	IMGTYPE_NORMAL,
	IMGTYPE_NORMALHEIGHT,
	IMGTYPE_DELUXE, // normals are swizzled, deluxe are not
} imgType_t;

typedef enum
{
	IMGFLAG_NONE           = 0x0000,
	IMGFLAG_MIPMAP         = 0x0001,
	IMGFLAG_PICMIP         = 0x0002,
	IMGFLAG_CUBEMAP        = 0x0004,
	IMGFLAG_NO_COMPRESSION = 0x0010,
	IMGFLAG_NOLIGHTSCALE   = 0x0020,
	IMGFLAG_CLAMPTOEDGE    = 0x0040,
	IMGFLAG_GENNORMALMAP   = 0x0080,
	IMGFLAG_LIGHTMAP       = 0x0100,
	IMGFLAG_NOSCALE        = 0x0200,
	IMGFLAG_CLAMPTOBORDER  = 0x0400,
} imgFlags_t;

typedef struct texture_s
{
    char *imgName;              // image path, including extension
    struct texture_s *next;     // for hash search
    struct texture_s *list;     // for listing

    uint32_t width, height;     // source image
    uint32_t uploadWidth;       // adter power of two but not including clamp to MAX_TEXTURE_SIZE
    uint32_t uploadHeight;
    uint32_t id;                // GL texture binding

    uint64_t frameUsed;

    uint32_t internalFormat;
    imgType_t type;
    imgFlags_t flags;
} texture_t;

enum {
	ATTRIB_INDEX_POSITION       = 0,
	ATTRIB_INDEX_TEXCOORD       = 1,
    ATTRIB_INDEX_COLOR          = 2,
    ATTRIB_INDEX_NORMAL         = 3,
	
	ATTRIB_INDEX_COUNT
};

enum
{
    ATTRIB_POSITION             = BIT(ATTRIB_INDEX_POSITION),
    ATTRIB_NORMAL               = BIT(ATTRIB_INDEX_NORMAL),
	ATTRIB_TEXCOORD             = BIT(ATTRIB_INDEX_TEXCOORD),
    ATTRIB_COLOR                = BIT(ATTRIB_INDEX_COLOR),

	ATTRIB_BITS =
        ATTRIB_POSITION |
        ATTRIB_TEXCOORD | 
        ATTRIB_COLOR |
        ATTRIB_NORMAL
};

typedef enum {
    GLSL_INT = 0,
    GLSL_FLOAT,
    GLSL_VEC2,
    GLSL_VEC3,
    GLSL_VEC4,
    GLSL_MAT16,
    GLSL_BUFFER, // uniform buffer -- special case
} glslType_t;

typedef enum {
    UNIFORM_DIFFUSE_MAP = 0,
    UNIFORM_LIGHT_MAP,
    UNIFORM_NORMAL_MAP,
    UNIFORM_SPECULAR_MAP,

    UNIFORM_NUM_LIGHTS,
    UNIFORM_LIGHT_INFO,
    UNIFORM_AMBIENT_LIGHT,

    UNIFORM_MODELVIEWPROJECTION,
    UNIFORM_MODELMATRIX,

    UNIFORM_SPECULAR_SCALE,
    UNIFORM_NORMAL_SCALE,

    UNIFORM_COLOR_GEN,
    UNIFORM_ALPHA_GEN,
    UNIFORM_COLOR,
    UNIFORM_BASE_COLOR,
    UNIFORM_VERT_COLOR,

    // random stuff
    UNIFORM_ALPHA_TEST,
    UNIFORM_TCGEN,
	UNIFORM_VIEWINFO, // znear, zfar, width/2, height/2

    UNIFORM_COUNT
} uniform_t;

typedef struct
{
    char name[MAX_GDR_PATH];

    char *compressedVSCode;
    char *compressedFSCode;

    uint32_t vertexBufLen;
    uint32_t fragmentBufLen;

    uint32_t programId;
    uint32_t vertexId;
    uint32_t fragmentId;
    uint32_t attribBits; // vertex array attribute flags

    // uniforms
    GLint uniforms[UNIFORM_COUNT];
    int16_t uniformBufferOffsets[UNIFORM_COUNT]; // max 32767/64=511 uniforms
    char *uniformBuffer;
} shaderProgram_t;

typedef struct
{
    shaderProgram_t *shader;
    uint32_t id;
    uint32_t binding;
    uint64_t size;
} uniformBuffer_t;

typedef enum
{
    DL_POINT,       // point light
    DL_SPOT,        // spot light
    DL_SKY,         // sky light
    DL_DIR,         // directional light
} dlightType_t; 

typedef struct dlight_s
{
    vec4_t color;
    vec3_t pos;

    float brightness;
    float diffuse;
    float specular;
    float ambient;

    dlightType_t ltype;

    struct dlight_s *next;
    struct dlight_s *prev;
} dlight_t;

typedef struct
{
    char name[MAX_GDR_PATH];

    uint32_t fboId;

    uint32_t colorBuffers[16];
    uint32_t colorFormat;
    texture_t *colorImage[16];

    uint32_t depthBuffer;
    uint32_t depthFormat;

    uint32_t stencilBuffer;
    uint32_t stencilFormat;

    uint32_t packedDepthStencilBuffer;
    uint32_t packedDepthStencilFormat;

    uint32_t width;
    uint32_t height;
    
    // if we're only rendering to a chunk of the screen
    uint32_t x;
    uint32_t y;
} fbo_t;

// normal and light are unused
typedef struct
{
    vec3_t xyz;
    vec2_t uv;
    vec2_t lightmap;
    int16_t normal[4];
    int16_t tangent[4];
    int16_t lightdir[4];
    uint16_t color[4];
} drawVert_t;

/*
==============================================================================

SURFACES

==============================================================================
*/

// any changes in surfaceType must be mirrored in rb_surfaceTable[]
typedef enum {
    SF_BAD,
    SF_SKIP,    // ignore
    SF_POLY,
    SF_TILE,

    SF_NUM_SURFACE_TYPES,
    SF_MAX = 0x7fffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

typedef struct drawSurf_s {
    uint32_t sort;
    surfaceType_t *surface; // any of surface*_t
} drawSurf_t;

typedef struct {
    surfaceType_t surface;

    // a single quad
    drawVert_t *verts;
    glIndex_t *indices;
} srfTile_t;

// when sgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
typedef struct {
    surfaceType_t   surfaceType;
    nhandle_t       hShader;
    uint32_t        numVerts;
    polyVert_t      *verts; // later becomes a drawVert_t
} srfPoly_t;

extern void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void *);

typedef struct {
    uint32_t index;
    uint32_t count;
    uint32_t type;
    uint32_t enabled;
    uint32_t normalized;
    uintptr_t stride;
    uintptr_t offset;
} vertexAttrib_t;

// not meant to be used by anything other than the vbo backend

typedef enum {
    BUFFER_STATIC,      // data is constant throughout buffer lifetime
    BUFFER_DYNAMIC,     // expected to be updated once in a while, but not every frame
    BUFFER_FRAME,       // expected to be update on a per-frame basis
} bufferType_t;

typedef enum {
    BUF_GL_MAPPED,
    BUF_GL_BUFFER,
    BUF_GL_CLIENT
} bufMemType_t;

typedef struct {
    void *data;
    uint32_t target;
    uint32_t id;
    uint32_t size;
    bufMemType_t usage;
    uintptr_t offset;
} buffer_t;

typedef struct vertexBuffer_s
{
    char name[MAX_GDR_PATH];
    
    uint32_t vaoId;
    bufferType_t type;

    buffer_t vertex;
    buffer_t index;

    vertexAttrib_t attribs[ATTRIB_INDEX_COUNT];

    struct vertexBuffer_s *next;
} vertexBuffer_t;

//===============================================================================

typedef enum {
	SS_BAD,
	SS_PORTAL,			// mirrors, portals, viewscreens
	SS_ENVIRONMENT,		// sky box
	SS_OPAQUE,			// opaque

	SS_DECAL,			// scorch marks, etc.
	SS_SEE_THROUGH,		// ladders, grates, grills that may have small blended edges
						// in addition to alpha test
	SS_BANNER,

	SS_FOG,

	SS_UNDERWATER,		// for items that should be drawn in front of the water plane

	SS_BLEND0,			// regular transparency and filters
	SS_BLEND1,			// generally only used for additive type effects
	SS_BLEND2,
	SS_BLEND3,

	SS_BLEND6,
	SS_STENCIL_SHADOW,
	SS_ALMOST_NEAREST,	// gun smoke puffs

	SS_NEAREST			// blood blobs
} shaderSort_t;


#define MAX_SHADER_STAGES 8

typedef enum {
	GF_NONE,

	GF_SIN,
	GF_SQUARE,
	GF_TRIANGLE,
	GF_SAWTOOTH, 
	GF_INVERSE_SAWTOOTH, 

	GF_NOISE

} genFunc_t;


typedef enum {
	DEFORM_NONE,
	DEFORM_WAVE,
	DEFORM_NORMALS,
	DEFORM_BULGE,
	DEFORM_MOVE,
	DEFORM_PROJECTION_SHADOW,
	DEFORM_AUTOSPRITE,
	DEFORM_AUTOSPRITE2,
	DEFORM_TEXT0,
	DEFORM_TEXT1,
	DEFORM_TEXT2,
	DEFORM_TEXT3,
	DEFORM_TEXT4,
	DEFORM_TEXT5,
	DEFORM_TEXT6,
	DEFORM_TEXT7
} deform_t;

// deformVertexes types that can be handled by the GPU
typedef enum
{
	// do not edit: same as genFunc_t

	DGEN_NONE,
	DGEN_WAVE_SIN,
	DGEN_WAVE_SQUARE,
	DGEN_WAVE_TRIANGLE,
	DGEN_WAVE_SAWTOOTH,
	DGEN_WAVE_INVERSE_SAWTOOTH,
	DGEN_WAVE_NOISE,

	// do not edit until this line

	DGEN_BULGE,
	DGEN_MOVE
} deformGen_t;

typedef enum {
	AGEN_IDENTITY,
	AGEN_SKIP,
	AGEN_ENTITY,
	AGEN_ONE_MINUS_ENTITY,
	AGEN_VERTEX,
	AGEN_ONE_MINUS_VERTEX,
	AGEN_LIGHTING_SPECULAR,
	AGEN_WAVEFORM,
	AGEN_PORTAL,
	AGEN_CONST,
} alphaGen_t;

typedef enum {
	CGEN_BAD,
	CGEN_IDENTITY_LIGHTING,	// tr.identityLight
	CGEN_IDENTITY,			// always (1,1,1,1)
	CGEN_ENTITY,			// grabbed from entity's modulate field
	CGEN_ONE_MINUS_ENTITY,	// grabbed from 1 - entity.modulate
	CGEN_EXACT_VERTEX,		// tess.vertexColors
	CGEN_VERTEX,			// tess.vertexColors * tr.identityLight
	CGEN_EXACT_VERTEX_LIT,	// like CGEN_EXACT_VERTEX but takes a light direction from the lightgrid
	CGEN_VERTEX_LIT,		// like CGEN_VERTEX but takes a light direction from the lightgrid
	CGEN_ONE_MINUS_VERTEX,
//	CGEN_WAVEFORM,			// programmatically generated
	CGEN_LIGHTING_DIFFUSE,
	CGEN_CONST				// fixed color
} colorGen_t;

typedef enum {
	TCGEN_BAD,
	TCGEN_IDENTITY,			// clear to 0,0
	TCGEN_LIGHTMAP,
	TCGEN_TEXTURE,
	TCGEN_ENVIRONMENT_MAPPED,
	TCGEN_ENVIRONMENT_MAPPED_FP, // with correct first-person mapping
	TCGEN_FOG,
	TCGEN_VECTOR,			// S and T from world coordinates
} texCoordGen_t;

typedef enum {
	ACFF_NONE,
	ACFF_MODULATE_RGB,
	ACFF_MODULATE_RGBA,
	ACFF_MODULATE_ALPHA
} acff_t;

#define TR_MAX_TEXMODS 4

typedef enum {
	TMOD_NONE,
	TMOD_TRANSFORM,
	TMOD_TURBULENT,
	TMOD_SCROLL,
	TMOD_SCALE,
	TMOD_STRETCH,
	TMOD_ROTATE,
	TMOD_ENTITY_TRANSLATE
} texMod_t;

typedef struct {
	texMod_t		type;

	// used for TMOD_TURBULENT and TMOD_STRETCH
//	waveForm_t		wave;

	// used for TMOD_TRANSFORM
	float			matrix[2][2];		// s' = s * m[0][0] + t * m[1][0] + trans[0]
	float			translate[2];		// t' = s * m[0][1] + t * m[0][1] + trans[1]

	// used for TMOD_SCALE
	float			scale[2];			// s *= scale[0]
	                                    // t *= scale[1]

	// used for TMOD_SCROLL
	float			scroll[2];			// s' = s + scroll[0] * time
										// t' = t + scroll[1] * time

	// + = clockwise
	// - = counterclockwise
	float			rotateSpeed;

} texModInfo_t;

#define MAX_IMAGE_ANIMATIONS 24

typedef struct {
	texture_t		*image[MAX_IMAGE_ANIMATIONS];
	uint32_t		numImageAnimations;
	double			imageAnimationSpeed;

	texCoordGen_t	tcGen;
	vec3_t			tcGenVectors[2];

	uint32_t		numTexMods;
	texModInfo_t	*texMods;

	uint32_t		videoMapHandle;
	qboolean		isLightmap;
	qboolean		isVideoMap;
} textureBundle_t;

enum
{
	TB_COLORMAP    = 0,
	TB_DIFFUSEMAP  = 0,
	TB_LIGHTMAP    = 1,
	TB_NORMALMAP   = 2,
	TB_SPECULARMAP = 4,
	NUM_TEXTURE_BUNDLES = 7
};

typedef enum
{
	// material shader stage types
	ST_COLORMAP = 0,			// vanilla Q3A style shader treatening
	ST_DIFFUSEMAP = 0,          // treat color and diffusemap the same
	ST_NORMALMAP,
	ST_NORMALPARALLAXMAP,
	ST_SPECULARMAP,
	ST_GLSL
} stageType_t;

typedef struct {
    qboolean active;

    textureBundle_t bundle[NUM_TEXTURE_BUNDLES];

    colorGen_t rgbGen;
    alphaGen_t alphaGen;

    byte constantColor[4]; // for CGEN_CONST and AGEN_CONST

    uint32_t stateBits; // GLSL_xxxx mask

    qboolean isLightmap;

    shaderProgram_t *glslShaderGroup;
    uint32_t glslShaderIndex;

    stageType_t type;

    vec4_t normalScale;
    vec4_t specularScale;
} shaderStage_t;

typedef struct shader_s {
	char		name[MAX_GDR_PATH];		// game path, including extension

	uint32_t	index;					// this shader == rg.shaders[index]
    shaderSort_t sort;
	uint32_t	sortedIndex;			// based on texture hash
    uint32_t    stateBits;              // depthMask, srcFactor, dstFactor (GLstate essentially)

	qboolean	defaultShader;			// we want to return index 0 if the shader failed to
										// load for some reason, but R_FindShader should
										// still keep a name allocated for it, so if
										// something calls RE_RegisterShader again with
										// the same name, we don't try looking for it again

	qboolean	explicitlyDefined;		// found in a .shader file

	uint32_t	surfaceFlags;			// if explicitlyDefined, this will have SURF_* flags
	uint32_t	contentFlags;

    uint32_t vertexAttribs;

    shaderStage_t *stages[MAX_SHADER_STAGES];

    qboolean noPicMips;
    qboolean noMipMaps;
    qboolean polygonOffset;
    int32_t lightmapIndex;
    int32_t lightmapSearchIndex;

	struct shader_s	*next;
} shader_t;

typedef struct {
    vec3_t origin;
    float angle;
    float zoom;
    float aspect;
    mat4_t modelMatrix;
} cameraData_t;

typedef struct {
    cameraData_t camera;

    fbo_t *targetFbo;

    uint32_t viewportX, viewportY, viewportWidth, viewportHeight;
    mat4_t projectionMatrix;

    float zFar;
    float zNear;

    stereoFrame_t stereoFrame;
} viewData_t;

typedef struct {
    char name[MAX_GDR_PATH]; // ie: maps/example.bmf
    char baseName[MAX_GDR_PATH]; // ie: example

    uint64_t dataSize;

    texture_t *tileset;
    vertexBuffer_t *buf;

    uint32_t width;
    uint32_t height;

    maplight_t *lights;
    uint32_t numLights;

    maptile_t *tiles;
    uint32_t numTiles;

    mapcheckpoint_t *checkpoints;
    uint32_t numCheckpoints;

    mapspawn_t *spawns;
    uint32_t numSpawns;

    glIndex_t *indices;
    uint32_t numIndices;

    drawVert_t *vertices;
    uint32_t numVertices;

    srfTile_t *tileSurfs;
    uint32_t numSurfs;

    tile2d_sprite_t *sprites;
    uint32_t numTilesetSprites;

    shader_t *shader;
} world_t;

typedef struct stageVars
{
	color4ub_t	colors[MAX_BATCH_VERTICES];
	vec2_t		texcoords[NUM_TEXTURE_BUNDLES][MAX_BATCH_VERTICES];
} stageVars_t;

typedef struct
{
    // silence the compiler warning
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif
    glIndex_t indices[MAX_BATCH_INDICES] GDR_ALIGN(16);
    vec3_t xyz[MAX_BATCH_VERTICES] GDR_ALIGN(16);
    int16_t normal[MAX_BATCH_VERTICES] GDR_ALIGN(16);
    vec2_t texCoords[MAX_BATCH_VERTICES] GDR_ALIGN(16);
    uint16_t color[MAX_BATCH_VERTICES][4] GDR_ALIGN(16);

    stageVars_t svars GDR_ALIGN(16);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    void *attribPointers[ATTRIB_INDEX_COUNT];

    uint64_t numVertices;
    uint64_t numIndices;
    uint64_t firstIndex;

    shader_t *shader;
    shaderStage_t **stages;
    vertexBuffer_t *buf;

    double shaderTime;

    qboolean useInternalVao;
    qboolean useCacheVao;
    qboolean updated;
} drawBuffer_t;

/*
** performanceCounters_t
*/
typedef struct {
    float c_overDraw;

    uint32_t c_bufferBinds;
    uint32_t c_bufferVertices;
    uint32_t c_bufferIndices;

    uint32_t c_staticBufferDraws;
    uint32_t c_dynamicBufferDraws;

    uint32_t c_texturesUsed;

    uint32_t c_glslShaderBinds;
    uint32_t c_vaoBinds;
    uint32_t c_vboBinds;
    uint32_t c_iboBinds;

    uint64_t msec; // total msec for backend run
} performanceCounters_t;

typedef struct
{
    drawBuffer_t dbuf;

    performanceCounters_t pc;

    qboolean framePostProcessed;
    qboolean depthFill;
    qboolean colorMask[4];

    byte color2D[4];

    shader_t *frameShader;
    vertexBuffer_t *frameCache;

    nhandle_t cmdThread;
} renderBackend_t;

extern renderBackend_t *backend;

#define MAX_TEXTURE_UNITS 16

// the renderer front end should never modify glstate_t
typedef struct {
	GLuint		currenttextures[ MAX_TEXTURE_UNITS ];
	int			currenttmu;
	qboolean	finishCalled;
	GLint		texEnv[2];
	unsigned	glStateBits;
    int         currentArray;
	unsigned	glClientStateBits[ MAX_TEXTURE_UNITS ];

    GLuint      textureStack[MAX_TEXTURE_UNITS];
    GLuint      *textureStackPtr;

    mat4_t modelViewProjectionMatrix;
    mat4_t projection;
    mat4_t modelView;

    uint32_t vertexAttribsEnabled;

    uint32_t    vaoId;
    uint32_t    vboId;
    uint32_t    iboId;
    uint32_t    defFboId;
    uint32_t    readFboId;
    uint32_t    writeFboId;
    uint32_t    rboId;
    uint32_t    shaderId;

    fbo_t *currentFbo;
    vertexBuffer_t *currentVao;
    texture_t *currentTexture;
    shaderProgram_t *currentShader;
} glstate_t;

#define MAX_RENDER_TEXTURES 1024
#define MAX_RENDER_PROGRAMS 128
#define MAX_RENDER_SHADERS 1024
#define MAX_RENDER_BUFFERS 1024
#define MAX_RENDER_FBOS 64

typedef struct renderGlobals_s
{
    qboolean registered;

    performanceCounters_t pc;

    shader_t *shaders[MAX_RENDER_SHADERS];
    shader_t *sortedShaders[MAX_RENDER_SHADERS];
    uint64_t numShaders;

    texture_t *textures[MAX_RENDER_TEXTURES];
    uint64_t numTextures;

    vertexBuffer_t *buffers[MAX_RENDER_BUFFERS];
    uint64_t numBuffers;

    fbo_t *fbos[MAX_RENDER_FBOS];
    uint64_t numFBOs;

    shaderProgram_t *programs[MAX_RENDER_PROGRAMS];
    uint64_t numPrograms;

    uint32_t numLightmaps;
    texture_t **lightmaps;

    world_t *world;
    qboolean worldLoaded;

    viewData_t viewData;

    fbo_t *renderFbo;
    fbo_t *msaaResolveFbo;

    fbo_t *currentFBO;
    vertexBuffer_t *currentBuffer;

    shader_t *defaultShader;

    shaderProgram_t basicShader;
    shaderProgram_t textureColorShader;

    texture_t *defaultImage;
    texture_t *whiteImage;
    texture_t *identityLightImage;
    texture_t *scratchImage[MAX_VIDEO_HANDLES];
    texture_t *renderImage;
    texture_t *screenScratchImage;
    texture_t *hdrDepthImage;
    texture_t *renderDepthImage;
    texture_t *textureDepthImage;
    texture_t *screenSsaoImage;

    uint64_t frameCount;

    file_t logFile;

    float identityLight;

    uint32_t overbrightBits;
    uint32_t identityLightByte;
} renderGlobals_t;

typedef enum {
    TexDetail_MSDOS,
    TexDetail_IntegratedGPU,
    TexDetail_Normie,
    TexDetail_ExpensiveShitWeveGotHere,
    TexDetail_GPUvsGod
} textureDetail_t;

typedef enum {              // [min, mag]
    TexFilter_Linear,       // GL_LINEAR GL_LINEAR
    TexFilter_Nearest,      // GL_NEAREST GL_NEAREST
    TexFilter_Bilinear,     // GL_NEAREST GL_LINEAR
    TexFilter_Trilinear     // GL_LINEAR GL_NEAREST
};

extern renderGlobals_t rg;
extern glContext_t *glContext;
extern glstate_t glState;
extern gpuConfig_t glConfig;

#define OffsetByteToFloat(a)    ((float)(a) * 1.0f/127.5f - 1.0f)
#define FloatToOffsetByte(a)    (byte)((a) * 127.5f + 128.0f)
#define ByteToFloat(a)          ((float)(a) * 1.0f/255.0f)
#define FloatToByte(a)          (byte)((a) * 255.0f)

#define GLS_SRCBLEND_ZERO						0x00000001
#define GLS_SRCBLEND_ONE						0x00000002
#define GLS_SRCBLEND_DST_COLOR					0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR		0x00000004
#define GLS_SRCBLEND_SRC_ALPHA					0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		0x00000006
#define GLS_SRCBLEND_DST_ALPHA					0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE				0x00000009
#define GLS_SRCBLEND_BITS						0x0000000f

#define GLS_DSTBLEND_ZERO						0x00000010
#define GLS_DSTBLEND_ONE						0x00000020
#define GLS_DSTBLEND_SRC_COLOR					0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		0x00000040
#define GLS_DSTBLEND_SRC_ALPHA					0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		0x00000060
#define GLS_DSTBLEND_DST_ALPHA					0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		0x00000080
#define GLS_DSTBLEND_BITS						0x000000f0

#define GLS_BLEND_BITS							(GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)

#define GLS_DEPTHMASK_TRUE						0x00000100

#define GLS_POLYMODE_LINE						0x00001000

#define GLS_DEPTHTEST_DISABLE					0x00010000
#define GLS_DEPTHFUNC_EQUAL						0x00020000
#define GLS_DEPTHFUNC_GREATER                   0x00040000
#define GLS_DEPTHFUNC_BITS                      0x00060000

#define GLS_ATEST_GT_0							0x10000000
#define GLS_ATEST_LT_80							0x20000000
#define GLS_ATEST_GE_80							0x40000000
#define GLS_ATEST_BITS							0x70000000

#define GLS_DEFAULT                             (GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA)

// vertex array states

#define CLS_NONE								0x00000000
#define CLS_COLOR_ARRAY							0x00000001
#define CLS_TEXCOORD_ARRAY						0x00000002
#define CLS_NORMAL_ARRAY						0x00000004

#define DRAW_IMMEDIATE 0
#define DRAW_CLIENT_BUFFERED 1
#define DRAW_GPU_BUFFERED 2

#define TEXUNIT_COLORMAP    (GL_TEXTURE0+TB_COLORMAP)
#define TEXUNIT_DIFFUSEMAP  (GL_TEXTURE0+TB_DIFFUSEMAP)
#define TEXUNIT_LIGHTMAP    (GL_TEXTURE0+TB_LIGHTMAP)
#define TEXUNIT_NORMALMAP   (GL_TEXTURE0+TB_NORMALMAP)
#define TEXUNIT_SPECULARMAP (GL_TEXTURE0+TB_SPECULARMAP)
#define TEXUNIT_TB(x) (GL_TEXTURE0+(x))

extern cvar_t *r_experimental; // do we want experimental features?
extern cvar_t *r_colorMipLevels;
extern cvar_t *r_glDebug;
extern cvar_t *r_genNormalMaps;
extern cvar_t *r_printShaders;
extern cvar_t *r_baseNormalX;
extern cvar_t *r_baseNormalY;
extern cvar_t *r_baseParallax;
extern cvar_t *r_baseSpecular;
extern cvar_t *r_baseGloss;
extern cvar_t *vid_xpos;
extern cvar_t *vid_ypos;
extern cvar_t *r_allowSoftwareGL;
extern cvar_t *r_displayRefresh;
extern cvar_t *r_fullscreen;
extern cvar_t *r_customWidth;
extern cvar_t *r_customHeight;
extern cvar_t *r_aspectRatio;
extern cvar_t *r_driver;
extern cvar_t *r_drawFPS;
extern cvar_t *r_swapInterval;
extern cvar_t *r_mode;
extern cvar_t *r_customPixelAspect;
extern cvar_t *r_colorBits;
extern cvar_t *r_stencilBits;
extern cvar_t *r_depthBits;
extern cvar_t *r_stereoEnabled;
extern cvar_t *r_allowShaders;
extern cvar_t *r_clear;
extern cvar_t *r_ignoreGLErrors;
extern cvar_t *r_ignorehwgamma;
extern cvar_t *r_gammaAmount;
extern cvar_t *r_drawMode;
extern cvar_t *r_allowLegacy;
extern cvar_t *r_useExtensions;
extern cvar_t *r_crashOnFailedProc;
extern cvar_t *r_measureOverdraw;
extern cvar_t *r_finish;
extern cvar_t *r_textureFiltering;
extern cvar_t *r_textureDetail;
extern cvar_t *r_multisampleType;
extern cvar_t *r_multisampleAmount;
extern cvar_t *r_hdr;
extern cvar_t *r_ssao; // screen space ambient occlusion
extern cvar_t *r_drawBuffer;
extern cvar_t *r_speeds;
extern cvar_t *r_maxPolyVerts;
extern cvar_t *r_maxPolys;
extern cvar_t *r_drawWorld;
extern cvar_t *r_externalGLSL;
extern cvar_t *r_skipBackEnd;
extern cvar_t *r_ignoreDstAlpha;
extern cvar_t *r_znear;
extern cvar_t *r_depthPrepass;
extern cvar_t *r_textureBits;
extern cvar_t *r_greyscale;
extern cvar_t *r_roundImagesDown;
extern cvar_t *r_imageUpsampleMaxSize;
extern cvar_t *r_imageUpsample;
extern cvar_t *r_imageUpsampleType;
extern cvar_t *r_picmip;
extern cvar_t *r_overBrightBits;
extern cvar_t *r_intensity;
extern cvar_t *r_normalMapping;
extern cvar_t *r_specularMapping;
extern cvar_t *r_showImages;
extern cvar_t *r_fullbright;
extern cvar_t *r_singleShader;

// GL extensions
extern cvar_t *r_arb_texture_filter_anisotropic;
extern cvar_t *r_arb_vertex_buffer_object;
extern cvar_t *r_arb_vertex_array_object;
extern cvar_t *r_arb_texture_float;
extern cvar_t *r_arb_framebuffer_object;
extern cvar_t *r_arb_vertex_shader;
extern cvar_t *r_arb_texture_compression;

#ifndef SGN
#define SGN(x) (((x) >= 0) ? !!(x) : -1)
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef CLAMP
#define CLAMP(a,b,c) MIN(MAX((a),(b)),(c))
#endif

static GDR_INLINE int VectorCompare4(const vec4_t v1, const vec4_t v2)
{
	if(v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] || v1[3] != v2[3])
	{
		return 0;
	}
	return 1;
}

//
// rgl_shader.c
//
void R_ShaderList_f(void);
nhandle_t RE_RegisterShaderFromTexture(const char *name, texture_t *image, int32_t lightmapIndex, qboolean mipRawImage);
nhandle_t RE_RegisterShader(const char *name);
shader_t *R_GetShaderByHandle(nhandle_t hShader);
shader_t *R_FindShaderByName( const char *name );
shader_t *R_FindShader(const char *name, int32_t lightmapIndex, qboolean mipRawImage);
void R_InitShaders( void );

//
// rgl_shade.c
//
void RB_BeginSurface(shader_t *shader);
void RB_EndSurface(void);
void RB_StageIterator(void);

//
// rgl_program.c
//
void GLSL_UseProgram(shaderProgram_t *program);
void GLSL_InitGPUShaders(void);
void GLSL_ShutdownGPUShaders(void);
void GLSL_SetUniformInt(shaderProgram_t *program, uint32_t uniformNum, GLint value);
void GLSL_SetUniformFloat(shaderProgram_t *program, uint32_t uniformNum, GLfloat value);
void GLSL_SetUniformVec2(shaderProgram_t *program, uint32_t uniformNum, const vec2_t v);
void GLSL_SetUniformVec3(shaderProgram_t *program, uint32_t uniformNum, const vec3_t v);
void GLSL_SetUniformVec4(shaderProgram_t *program, uint32_t uniformNum, const vec4_t v);
void GLSL_SetUniformMatrix4(shaderProgram_t *program, uint32_t uniformNum, const mat4_t m);

//
// rgl_math.c
//
void Mat4Scale( float scale, const mat4_t in, mat4_t out );
void Mat4Rotate( const vec3_t v, float angle, const mat4_t in, mat4_t out );
void Mat4Zero( mat4_t out );
void Mat4Identity( mat4_t out );
void Mat4Copy( const mat4_t in, mat4_t out );
void Mat4Multiply( const mat4_t in1, const mat4_t in2, mat4_t out );
void Mat4Transform( const mat4_t in1, const vec4_t in2, vec4_t out );
qboolean Mat4Compare( const mat4_t a, const mat4_t b );
void Mat4Dump( const mat4_t in );
void Mat4Translation( vec3_t vec, mat4_t out );
void Mat4Ortho( float left, float right, float bottom, float top, float znear, float zfar, mat4_t out );
void Mat4View(vec3_t axes[3], vec3_t origin, mat4_t out);
void Mat4SimpleInverse( const mat4_t in, mat4_t out);
int NextPowerOfTwo(int in);
unsigned short FloatToHalf(float in);
float HalfToFloat(unsigned short in);


//
// rgl_texture.c
//
void R_ImageList_f(void);
nhandle_t RE_RegisterSpriteSheet(const char *name, uint32_t spriteX, uint32_t spriteY);
void R_DeleteTextures(void);
texture_t *R_FindImageFile( const char *name, imgType_t type, imgFlags_t flags );
void R_GammaCorrect( byte *buffer, uint64_t bufSize );

//
// rgl_cmd.c
//
void *R_GetCommandBufferReserved( uint32_t bytes, uint32_t reservedBytes );
void *R_GetCommandBuffer( uint32_t bytes );
void R_IssueRenderCommands( qboolean runPerformanceCounters );
void R_IssuePendingRenderCommands( void );

//
// rgl_backend.c
//
void GDR_ATTRIBUTE((format(printf, 1, 2))) GL_LogComment(const char *fmt, ...);
void GDR_ATTRIBUTE((format(printf, 1, 2))) GL_LogError(const char *fmt, ...);
void GL_SetObjectDebugName(GLenum target, GLuint id, const char *name, const char *add);
void RB_ShowImages(void);
void GL_SetModelViewMatrix(const mat4_t m);
void GL_SetProjectionMatrix(const mat4_t m);
void GL_CheckErrors(void);
void GL_BindNullTextures(void);
void GL_PushTexture(texture_t *texture);
void GL_PopTexture(void);
void GL_BindTexture(GLenum unit, texture_t *texture);
void GL_BindNullRenderbuffer(void);
int GL_UseProgram(GLuint program);
void GL_BindNullProgram(void);
void GL_BindFramebuffer(GLenum target, GLuint fbo);
void GL_BindNullFramebuffer(GLenum target);
void GL_BIndNullFramebuffers(void);
void GL_BindRenderbuffer(GLuint rbo);
void GL_BindNullRenderbuffer(void);
void GL_State(unsigned stateBits);
void GL_ClientState( int unit, unsigned stateBits );
void GL_SetDefaultState(void);
void RE_BeginFrame(stereoFrame_t stereoFrame);
void RE_EndFrame(uint64_t *frontEndMsec, uint64_t *backEndMsec);
void RB_ExecuteRenderCommands(const void *data);

//
// rgl_init.c
//
qboolean R_HasExtension(const char *ext);
void R_Init(void);

//
// rgl_main.c
//
uint32_t R_GenDrawSurfSort(const shader_t *sh);
void RB_MakeModelViewProjection(void);
void GL_CameraResize(void);
void R_SortDrawSurfs(drawSurf_t *drawSurfs, uint32_t numDrawSurfs);
void R_AddDrawSurf(surfaceType_t *surface, shader_t *shader);
void RE_BeginRegistration(gpuConfig_t *config);

//
// rgl_scene.c
//
extern uint64_t r_firstSceneDrawSurf;
extern uint64_t r_numEntities;
extern uint64_t r_firstSceneEntity;
extern uint64_t r_numPolys;
extern uint64_t r_firstScenePoly;
extern uint64_t r_numPolyVerts;
void R_ConvertCoords(vec3_t verts[4], vec3_t pos);
qboolean RB_SurfaceVaoCached(uint32_t numVerts, drawVert_t *verts, uint32_t numIndices, glIndex_t *indices);
void GDR_EXPORT RE_AddPolyToScene( nhandle_t hShader, const polyVert_t *verts, uint32_t numVerts );
void GDR_EXPORT RE_AddPolyListToScene( const poly_t *polys, uint32_t numPolys );
void GDR_EXPORT RE_ClearScene(void);
void RB_CheckOverflow(uint32_t verts, uint32_t indexes);
void R_InitNextFrame(void);
void R_FinishBatch(const drawBuffer_t *input);
void R_DrawElements(uint32_t numIndices, uintptr_t offset);
void RB_InstantQuad2(vec4_t quadVerts[4], vec2_t texCoords[4]);
void RB_InstantQuad(vec4_t quadVerts[4]);

//
// rgl_texture.c
//
void R_UpdateTextures(void);
void R_InitTextures(void);
void R_ShutdownTextures(void);

//
// rgl_fbo.c
//
void FBO_Bind(fbo_t *fbo);
void FBO_Shutdown(void);
void FBO_Init(void);
void FBO_FastBlit(fbo_t *src, ivec4_t srcBox, fbo_t *dst, ivec4_t dstBox, uint32_t buffers, uint32_t filter);

//
// rgl_cache.c
//
void VBO_Bind(vertexBuffer_t *vbo);
void VBO_BindNull(void);
vertexBuffer_t *R_AllocateBuffer(const char *name, void *vertices, uint32_t verticesSize, void *indices, uint32_t indicesSize, bufferType_t type);
void VBO_Flush(vertexBuffer_t *buf);
void R_ShutdownBuffers(void);
void R_VaoPackColor(uint16_t *out, const vec4_t c);
void R_VaoPackNormal(int16_t *out, vec3_t v);
void R_VaoUnpackNormal(vec3_t v, int16_t *pack);

void R_InitGPUBuffers(void);
void R_ShutdownGPUBuffers(void);

#if 0
// per frame functions
void VaoCache_Init(void);
void VaoCache_InitQueue(void);
void VaoCache_AddSurface(drawVert_t *verts, uint32_t numVerts, glIndex_t *indexes, uint32_t numIndexes);
void VaoCache_BindVao(void);
void VaoCache_Commit(void);
void VaoCache_RecycleVertexBuffer(void);
void VaoCache_RecycleIndexBuffer(void);
void VaoCache_CheckAdd(qboolean *endSurface, qboolean *recycleVertexBuffer, qboolean *recycleIndexBuffer, uint32_t numVerts, uint32_t numIndexes);
#endif
void RB_UpdateCache(uint32_t attribBits);


/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

#define MAX_RC_BUFFER 0x80000

typedef struct
{
    byte buffer[MAX_RC_BUFFER];
    uint32_t usedBytes;
} renderCommandList_t;

typedef enum
{
    RC_POSTPROCESS,
    RC_SWAP_BUFFERS,
    RC_DRAW_BUFFER,
    RC_DRAW_SURFS,
    RC_COLORMASK,

    // mainly called from the vm
    RC_DRAW_IMAGE,
    RC_SET_COLOR,

    RC_END_LIST
} renderCmdType_t;

typedef struct {
    renderCmdType_t commandId;
    drawSurf_t *drawSurfs;
    uint32_t numDrawSurfs;
    viewData_t viewData;
} drawSurfCmd_t;

typedef struct {
    renderCmdType_t commandId;
    float color[4];
} setColorCmd_t;

typedef struct {
    renderCmdType_t commandId;
    uint32_t rgba[4];
} colorMaskCmd_t;

typedef struct {
    renderCmdType_t commandId;
} swapBuffersCmd_t;

typedef struct {
    renderCmdType_t commandId;
    uint32_t buffer;
} drawBufferCmd_t;

typedef struct {
    renderCmdType_t commandId;
    shader_t *shader;
    float x, y;
    float w, h;
    float u1, v1;
    float u2, v2;
} drawImageCmd_t;

typedef struct {
    renderCmdType_t commandId;
    viewData_t viewData;
} postProcessCmd_t;

#define MAX_DRAWSURFS 0x10000
#define DRAWSURF_MASK (0x20000-1)

typedef struct {
    drawSurf_t drawSurfs[MAX_DRAWSURFS];
    polyVert_t *polyVerts;
    srfPoly_t *polys;

    uint64_t numPolys;
    uint64_t numDrawSurfs;

    renderCommandList_t commandList;
} renderBackendData_t;

extern renderBackendData_t *backendData;

GDR_EXPORT void RE_DrawImage( float x, float y, float w, float h, float u1, float v1, float u2, float v2, nhandle_t hShader );
GDR_EXPORT void RE_LoadWorldMap(const char *filename);
GDR_EXPORT void RE_SetColor(const float *rgba);

#endif