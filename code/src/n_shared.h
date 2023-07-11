#ifndef _N_SHARED_
#define _N_SHARED_

#pragma once

#ifdef __LCC__
#ifndef Q3_VM
#define Q3_VM
#endif
#endif

#include "n_platform.h"
#ifndef _NOMAD_VERSION
#   error a version must be supplied when compiling the engine or a mod
#endif

#define NOMAD_VERSION _NOMAD_VERSION
#define NOMAD_VERSION_UPDATE _NOMAD_VERSION_UPDATE
#define NOMAD_VERSION_PATCH _NOMAD_VERSION_PATCH
#define VSTR_HELPER(x) #x
#define VSTR(x) VSTR_HELPER(x)
#define NOMAD_VERSION_STRING "glnomad v" VSTR(_NOMAD_VERSION) "." VSTR(_NOMAD_VERSION_UPDATE) "." VSTR(_NOMAD_VERSION_PATCH)

#ifdef Q3_VM
#include "bg_lib.h"
#else
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#endif

#include "n_pch.h"

typedef enum { qfalse = 0, qtrue = 1 } qboolean;

int N_strcmp(const char *str1, const char *str2);
int N_strncmp(const char *str1, const char *str2, size_t count);
int N_stricmp(const char *str1, const char *str2);
int N_stricmpn(const char *str1, const char *str2, size_t n);
char *N_strlwr(char *s1);
char *N_strupr(char *s1);
void N_strcat(char *dest, size_t size, const char *src);
const char *N_stristr(const char *s, const char *find);
#ifdef _WIN32
int N_vsnprintf(char *str, size_t size, const char *format, va_list ap);
#else
#define N_vsnprintf vsnprintf
#endif
int N_atoi(const char *s);
float N_atof(const char *s);
size_t N_strlen(const char *str);
qboolean N_streq(const char *str1, const char *str2);
qboolean N_strneq(const char *str1, const char *str2, size_t n);
char* N_stradd(char *dst, const char *src);
void N_strcpy(char *dest, const char *src);
void N_strncpy(char *dest, const char *src, size_t count);
void N_strncpyz(char *dest, const char *src, size_t count);
void* N_memset(void *dest, int fill, size_t count);
void N_memcpy(void *dest, const void *src, size_t count);
void* N_memchr(void *ptr, int c, size_t count);
int N_memcmp(const void *ptr1, const void *ptr2, size_t count);
int N_replace(const char *str1, const char *str2, char *src, size_t max_len);

#include "z_heap.h"
#include "string.hpp"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t mat3_t[3][3];
typedef vec_t mat4_t[4][4];
typedef unsigned char byte;

extern const vec3_t vec3_origin;
extern const vec2_t vec2_origin;

#define arraylen(arr) (sizeof(arr)/sizeof(*arr))

// math stuff
#define VectorAdd(a,b,c) (c[0]=a[0]+b[0];c[1]=a[1]+b[1];)
#define VectorSubtract(a,b,c) {c[0]=a[0]-b[0];c[1]=a[1]-b[1];}
#define DotProduct(x,y) (x[0]*y[0]+x[1]*y[1])
#define VectorCopy(x,y) (x[0]=y[0];x[1]=y[1];)
void CrossProduct(const vec2_t v1, const vec2_t v2, vec2_t out);
float disBetweenOBJ(const vec2_t src, const vec2_t tar);
float Q_rsqrt(float number);
float Q_root(float x);

#ifndef __cplusplus
static qboolean N_strtobool(const char* s)
{
	return N_stricmp(s, "true") ? qtrue : qfalse;
}
static const char* N_booltostr(qboolean b)
{
	return b ? "true" : "false";
}
#endif

#include "n_console.h"
#include "n_common.h"

#ifdef __cplusplus

#if !defined(NDEBUG) && !defined(_NOMAD_DEBUG)
    #undef NDEBUG
#endif

using json = nlohmann::json;

#include "g_bff.h"
#include "n_map.h"

template<typename T>
struct GDRSmartPointerDeleter {
	constexpr GDRSmartPointerDeleter() = default;
	~GDRSmartPointerDeleter() = default;

	inline constexpr void operator()(T *ptr) { Z_ChangeTag((void *)ptr, TAG_PURGELEVEL); }
};

template<typename type>
using nomadunique_ptr = eastl::unique_ptr<type, GDRSmartPointerDeleter<type>>;

template<typename T, typename... Args>
inline nomadunique_ptr<T> make_nomadunique(Args&&... args)
{
	return nomadunique_ptr<T>(
		static_cast<T*>(Z_Malloc(sizeof(T), TAG_STATIC, NULL, "zalloc")(eastl::forward<Args>(args)...)));
}

// a wee little eastl thingy
namespace eastl {
	constexpr const uint32_t file_end = SEEK_END;
	constexpr const uint32_t file_cur = SEEK_CUR;
	constexpr const uint32_t file_beg = SEEK_SET;

	struct ifstream {
		inline ifstream(const ::eastl::string& filepath)
			: fp(fopen(filepath.c_str(), "rb")) { }
		inline ifstream(const char *filepath)
			: fp(fopen(filepath, "rb")) { }
		ifstream(const ifstream &) = delete;
		ifstream(ifstream &&) = delete;
		inline ~ifstream(void)
		{ ::fclose(fp); }
		inline operator bool(void) const
		{ return fp ? true : false; }
		inline size_t read(void *buffer, size_t size)
		{ return ::fread(buffer, sizeof(char), size, fp); }
		inline bool fail(void) const
		{ return fp == NULL; }
		inline bool is_open(void) const
		{ return fp != NULL; }
		inline void close(void)
		{ ::fclose(fp); }
		inline void open(const ::eastl::string& filepath)
		{ fp = ::fopen(filepath.c_str(), "wb"); }
		inline void open(const char *filepath)
		{ fp = ::fopen(filepath, "wb"); }
		inline uint64_t seekg(uint64_t offset, const uint32_t whence)
		{ return ::fseek(fp, offset, whence); }
		inline uint64_t tellg(void)
		{ return ::ftell(fp); }
		inline void rewind(void)
		{ ::rewind(fp); }
	private:
		FILE *fp = NULL;
	};

	struct ofstream {
		inline ofstream(const ::eastl::string& filepath)
			: fp(fopen(filepath.c_str(), "wb")) { }
		inline ofstream(const char *filepath)
			: fp(fopen(filepath, "wb")) { }
		ofstream(const ofstream &) = delete;
		ofstream(ofstream &&) = delete;
		inline ~ofstream(void)
		{ ::fclose(fp); }
		inline size_t write(const void *buffer, size_t size)
		{ return ::fwrite(buffer, sizeof(char), size, fp); }
		inline bool fail(void) const
		{ return fp == NULL; }
		inline bool is_open(void) const
		{ return fp != NULL; }
		inline void close(void)
		{ ::fclose(fp); }
		inline void open(const ::eastl::string& filepath)
		{ fp = ::fopen(filepath.c_str(), "wb"); }
		inline void open(const char *filepath)
		{ fp = ::fopen(filepath, "wb"); }
		inline uint64_t seekg(uint64_t offset, const uint32_t whence)
		{ return ::fseek(fp, offset, whence); }
		inline uint64_t tellg(void)
		{ return ::ftell(fp); }
		inline void rewind(void)
		{ ::rewind(fp); }
		inline void putc(int c)
		{ ::fputc(c, fp); }
	private:
		FILE *fp = NULL;
	};
	
	struct fstream {
		inline fstream(const ::eastl::string& filepath)
			: fp(fopen(filepath.c_str(), "wb+")) { }
		inline fstream(const char *filepath)
			: fp(fopen(filepath, "wb+")) { }
		fstream(const fstream &) = delete;
		fstream(fstream &&) = delete;
		inline ~fstream(void)
		{ ::fclose(fp); }
		inline size_t read(void *buffer, size_t size)
		{ return ::fread(buffer, sizeof(char), size, fp); }
		inline size_t write(const void *buffer, size_t size)
		{ return ::fwrite(buffer, sizeof(char), size, fp); }
		inline bool fail(void) const
		{ return fp == NULL; }
		inline bool is_open(void) const
		{ return fp != NULL; }
		inline void close(void)
		{ ::fclose(fp); }
		inline void open(const ::eastl::string& filepath)
		{ fp = ::fopen(filepath.c_str(), "wb+"); }
		inline void open(const char *filepath)
		{ fp = ::fopen(filepath, "wb+"); }
		inline uint64_t seekg(uint64_t offset, const uint32_t whence)
		{ return ::fseek(fp, offset, whence); }
		inline uint64_t tellg(void)
		{ return ::ftell(fp); }
		inline void rewind(void)
		{ ::rewind(fp); }
		inline void putc(int c)
		{ ::fputc(c, fp); }
		inline int getc(void)
		{ return ::fgetc(fp); }
	private:
		FILE *fp = NULL;
	};
};

#define PAD(base, alignment)	(((base)+(alignment)-1) & ~((alignment)-1))
#define PADLEN(base, alignment)	(PAD((base), (alignment)) - (base))

template<typename type, typename alignment>
inline type* PADP(type *base, alignment align)
{
	return (type *)((void *)PAD((intptr_t)base, align));
}

template<class T, typename... Args>
inline T* construct(T *ptr, Args&&... args)
{
    ::new ((void *)ptr) T(eastl::forward<Args>(args)...);
    return ptr;
}

template<class T>
inline T* construct(T *ptr)
{
    ::new ((void *)ptr) T();
    return ptr;
}

#ifdef _NOMAD_DEBUG
#undef assert
inline void __nomad_assert_fail(const char* expression, const char* file, const char* func, unsigned line)
{
	N_Error(
		"Assertion '%s' failed (Main Engine):\n"
		"  \\file: %s\n"
		"  \\function: %s\n"
		"  \\line: %u\n\nIf this is an SDL2 error, here is the message string: %s\n",
	expression, file, func, line, SDL_GetError());
}
#define assert(x) (((x)) ? void(0) : __nomad_assert_fail(#x,__FILE__,__func__,__LINE__))
#else
#define assert(x)
#endif

#define MAX_TICRATE 333
#define MIN_TICRATE 10
#define MAX_VERT_FOV 100
#define MAX_HORZ_FOV 250
#define MSAA_OFF 0
#define MSAA_4X 1
#define MSAA_8X 2
#define MSAA_16X 3

#define N_strtobool(str) (N_strncasecmp("true", (str), 4) ? (1) : (0))
template<typename type>
inline const char* N_booltostr(type b)
{
	return b ? "true" : "false";
}


// eastl overloaders
inline std::ofstream& operator<<(std::ofstream& o, const eastl::string& data)
{
	o.write(data.c_str(), data.size());
	return o;
}
inline std::ofstream& operator>>(const eastl::string& data, std::ofstream& o)
{
	return o << data;
}
inline std::ifstream& operator>>(std::ifstream& i, eastl::string& data)
{
	return eastl::getline(i, data);
}
inline std::ifstream& operator<<(eastl::string& data, std::ifstream& i)
{
	return i >> data;
}

extern int myargc;
extern char** myargv;

int I_GetParm(const char *parm);
size_t N_ReadFile(const char *filepath, void *buffer);
size_t N_LoadFile(const char *filepath, void **buffer);
size_t N_FileSize(const char *filepath);
void N_WriteFile(const char *filepath, const void *data, size_t size);
float disBetweenOBJ(const vec2_t src, const vec2_t tar);
float disBetweenOBJ(const glm::vec2& src, const glm::vec2& tar);
float disBetweenOBJ(const glm::vec3& src, const glm::vec3& tar);

typedef struct coord_s
{
	float y, x;
	inline coord_s() = default;
	inline coord_s(float _y, float _x)
		: y(_y), x(_x)
	{
	}
	inline coord_s(const coord_s &) = default;
	inline coord_s(coord_s &&) = default;
	inline ~coord_s() = default;
	
	inline bool operator==(const coord_s& c) const
	{ return (y == c.y && x == c.x); }
	inline bool operator!=(const coord_s& c) const
	{ return (y != c.y && x != c.x); }
	inline bool operator>(const coord_s& c) const
	{ return (y > c.y && x > c.x); }
	inline bool operator<(const coord_s& c) const
	{ return (y < c.y && x < c.x); }
	inline bool operator>=(const coord_s& c) const
	{ return (y >= c.y && x <= c.x); }
	inline bool operator<=(const coord_s& c) const
	{ return (y <= c.y && x <= c.x); }
	
	inline coord_s& operator++(int) {
		++y;
		++x;
		return *this;
	}
	inline coord_s& operator++(void) {
		y++;
		x++;
		return *this;
	}
	inline coord_s& operator--(int) {
		--y;
		--x;
		return *this;
	}
	inline coord_s& operator--(void) {
		y--;
		x--;
		return *this;
	}
	inline coord_s& operator+=(const coord_s& c) {
		y += c.y;
		x += c.x;
		return *this;
	}
	inline coord_s& operator-=(const coord_s& c) {
		y -= c.y;
		x -= c.x;
		return *this;
	}
	inline coord_s& operator*=(const coord_s& c) {
		y *= c.y;
		x *= c.x;
		return *this;
	}
	
	inline coord_s& operator=(const coord_s& c) {
		y = c.y;
		x = c.x;
		return *this;
	}
	inline coord_s& operator=(const float p) {
		y = p;
		x = p;
		return *this;
	}
} coord_t;

constexpr const char* about_str =
"The Nomad has been a project in the making for a long time now, and its been a hard game to,\n"
"develop. The game's idea originally came from fan-fiction after I played Modern Warfare 2's Remastered\n"
"Campaign for the second time around, it used to take place in right about 2020-2080, but then during\n"
"the development I watched and feel in love with Mad Max: Fury Road. I changed up the setting to fit\n"
"more of a desert-planet style of post-apocalypse, but when I was doing this, I thought: \"How can I\n"
"use the already developed content?\", and that right there is exactly how the notorious mercenary guilds\n"
"of Bellatum Terrae were born.\n"
"\nHave fun,\nYour Resident Fiend, Noah Van Til\n\n(IN COLLABORATION WITH GDR GAMES)\n";
constexpr const char* credits_str =
"\n"
"That one slightly aggressive victorian-london-vibey piano piece you'll hear on the ever so occassion:\n    alpecagrenade\n"
"Programming: Noah Van Til (and most of the music as well)\n"
"Concept Artists & Ideas Contributers: Cooper & Tucker Kemnitz\n"
"A Few of the Guns: Ben Pavlovic\n"
"\n";

#define UPPER_CASE(x) (char)((x) - 32)
#define UPPER_CASE_STR(x) (const char *)((x) - 32)
#define MOUSE_WHEELUP   5
#define MOUSE_WHEELDOWN 6

inline const char* N_ButtonToString(const uint32_t& code)
{
	switch (code) {
	case SDLK_a:
	case SDLK_b:
	case SDLK_c:
	case SDLK_d:
	case SDLK_e:
	case SDLK_f:
	case SDLK_g:
	case SDLK_h:
	case SDLK_i:
	case SDLK_j:
	case SDLK_k:
	case SDLK_l:
	case SDLK_m:
	case SDLK_n:
	case SDLK_o:
	case SDLK_p:
	case SDLK_q:
	case SDLK_r:
	case SDLK_s:
	case SDLK_t:
	case SDLK_u:
	case SDLK_v:
	case SDLK_w:
	case SDLK_x:
	case SDLK_y:
	case SDLK_z:
	case SDLK_0:
	case SDLK_1:
	case SDLK_2:
	case SDLK_3:
	case SDLK_4:
	case SDLK_5:
	case SDLK_6:
	case SDLK_7:
	case SDLK_8:
	case SDLK_9:
	case SDLK_BACKSLASH:
	case SDLK_SLASH:
	case SDLK_PERIOD:
	case SDLK_COMMA:
	case SDLK_SEMICOLON:
	case SDLK_QUOTE:
	case SDLK_MINUS:
	case SDLK_EQUALS:
	case SDLK_LEFTBRACKET:
	case SDLK_RIGHTBRACKET:
	case SDLK_LEFTPAREN:
	case SDLK_RIGHTPAREN:
		return (const char *)&code;
		break;
	case SDL_BUTTON_LEFT: return "Mouse Button Left"; break;
	case SDL_BUTTON_RIGHT: return "Mouse Button Right"; break;
	case SDL_BUTTON_MIDDLE: return "Mouse Button Middle"; break;
	case MOUSE_WHEELDOWN: return "Mouse Wheel Down"; break;
	case MOUSE_WHEELUP: return "Mouse Wheel Up"; break;
	case SDLK_RETURN: return "Enter"; break;
	case SDLK_LCTRL: return "Left Control"; break;
	case SDLK_RCTRL: return "Right Control"; break;
	case SDLK_SPACE: return "Space"; break;
	case SDLK_BACKSPACE: return "Backspace"; break;
	case SDLK_RIGHT: return "Right Arrow"; break;
	case SDLK_UP: return "up Arrow"; break;
	case SDLK_LEFT: return "Left Arrow"; break;
	case SDLK_DOWN: return "Down Arrow"; break;
	case SDLK_TAB: return "Tab"; break;
	case SDLK_BACKQUOTE: return "Grave"; break;
	case SDLK_ESCAPE: return "Escape"; break;
	case SDLK_F1: return "F1"; break;
	case SDLK_F2: return "F2"; break;
	case SDLK_F3: return "F3"; break;
	case SDLK_F4: return "F4"; break;
	case SDLK_F5: return "F5"; break;
	case SDLK_F6: return "F6"; break;
	case SDLK_F7: return "F7"; break;
	case SDLK_F8: return "F8"; break;
	case SDLK_F9: return "F9"; break;
	case SDLK_F10: return "F10"; break;
	case SDLK_F11: return "F11"; break;
	case SDLK_F12: return "F12"; break;
	default:
		Con_Printf("WARNING: unknown SDL_KeyCode given");
		return "Unknown";
		break;
	};
}
#endif



typedef enum
{
    DIF_NOOB,
    DIF_RECRUIT,
    DIF_MERC,
    DIF_NOMAD,
    DIF_BLACKDEATH,
    DIF_MINORINCONVENIECE,

    DIF_HARDEST = DIF_MINORINCONVENIECE
} gamedif_t;

typedef enum
{
	D_NORTH,
	D_WEST,
	D_SOUTH,
	D_EAST,

	NUMDIRS,

	D_NULL
} dirtype_t;

typedef enum
{
    R_SDL2,
    R_OPENGL,
    R_VULKAN
} renderapi_t;

#endif
