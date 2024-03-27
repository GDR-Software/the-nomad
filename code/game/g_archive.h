#ifndef __G_ARCHIVE__
#define __G_ARCHIVE__

#pragma once

typedef struct {
    char mapname[MAX_NPATH];
    gamedif_t dif;

    // mod info
    char **modList;
    uint64_t numMods;
} gamedata_t;

typedef struct {
	char *name;
	int32_t nameLength;
	int32_t type;
	uint32_t dataSize;
    uint32_t dataOffset;
	union {
		int8_t s8;
		int16_t s16;
		int32_t s32;
		int64_t s64;
		uint8_t u8;
		uint16_t u16;
		uint32_t u32;
		uint64_t u64;
		float f;
		char *str;
		vec2_t v2;
		vec3_t v3;
		vec4_t v4;
	} data;
} ngdfield_t;


#pragma pack( push, 1 )
// version, 64 bits
typedef union version_s {
	struct {
		uint16_t m_nVersionMajor;
		uint16_t m_nVersionUpdate;
		uint32_t m_nVersionPatch;
	};
	uint64_t m_nVersion;

    bool operator==( const version_s& other ) const {
        return m_nVersion == other.m_nVersion;
    }
    bool operator!=( const version_s& other ) const {
        return m_nVersion != other.m_nVersion;
    }
} version_t;

typedef struct
{
	int32_t ident;
	version_t version;
} ngdvalidation_t;
#pragma pack( pop )

typedef struct {
	ngdvalidation_t validation;
	int64_t numSections;

    gamedata_t gamedata;
} ngdheader_t;

typedef struct {
    char name[MAX_STRING_CHARS];
    int64_t size;
    int64_t numFields;
} ngdsection_write_t;

template<typename Key, typename Value>
using ArchiveCache = eastl::unordered_map<Key, Value, eastl::hash<Key>, eastl::equal_to<Key>, CZoneAllocator<TAG_SAVEFILE>, true>;

typedef struct ngdsection_read_s
{
	char name[MAX_STRING_CHARS];
	int64_t nameLength;
	int64_t size;
	int64_t numFields;
	
    ArchiveCache<const char *, ngdfield_t *> m_FieldCache;
	
	struct ngdsection_read_s *next;
} ngdsection_read_t;

typedef struct {
	char name[MAX_OSPATH];
	
	ngdheader_t header;
	
	ngdsection_read_t *m_pSectionList;
} ngd_file_t;

#ifndef COMPILE_AATC
#include "../module_lib/module_public.h"
#include "aatc/aatc.hpp"
#include "aatc/aatc_common.hpp"
#include "aatc/aatc_container_unordered_map.hpp"
#include "aatc/aatc_container_vector.hpp"

class CGameArchive
{
public:
    CGameArchive( void ) = default;
    ~CGameArchive() = default;

    void BeginSaveSection( const char *moduleName, const char *name );
    void EndSaveSection( void );

    const char **GetSaveFiles( uint64_t *nFiles ) const;

    void SaveFloat( const char *name, float data );

    void SaveByte( const char *name, uint8_t data );
    void SaveUShort( const char *name, uint16_t data );
    void SaveUInt( const char *name, uint32_t data );
    void SaveULong( const char *name, uint64_t data );

    void SaveChar( const char *name, int8_t data );
    void SaveShort( const char *name, int16_t data );
    void SaveInt( const char *name, int32_t data );
    void SaveLong( const char *name, int64_t data );

    void SaveVec2( const char *name, const vec2_t data );
    void SaveVec3( const char *name, const vec3_t data );
    void SaveVec4( const char *name, const vec4_t data );

    void SaveCString( const char *name, const char *data );
    void SaveString( const char *name, const string_t *pData );

    void SaveInt8Array( const char *name, const aatc::container::tempspec::vector<int8_t> *pData );
    void SaveInt16Array( const char *name, const aatc::container::tempspec::vector<int16_t> *pData );
    void SaveInt32Array( const char *name, const aatc::container::tempspec::vector<int32_t> *pData );
    void SaveInt64Array( const char *name, const aatc::container::tempspec::vector<int64_t> *pData );
    void SaveUInt8Array( const char *name, const aatc::container::tempspec::vector<uint8_t> *pData );
    void SaveUInt16Array( const char *name, const aatc::container::tempspec::vector<uint16_t> *pData );
    void SaveUInt32Array( const char *name, const aatc::container::tempspec::vector<uint32_t> *pData );
    void SaveUInt64Array( const char *name, const aatc::container::tempspec::vector<uint64_t> *pData );
    void SaveFloatArray( const char *name, const aatc::container::tempspec::vector<float> *pData );

    void SaveArray( const char *pszName, const CScriptArray *pData );

    float LoadFloat( const char *name, nhandle_t hSection );

    uint8_t LoadByte( const char *name, nhandle_t hSection );
    uint16_t LoadUShort( const char *name, nhandle_t hSection );
    uint32_t LoadUInt( const char *name, nhandle_t hSection );
    uint64_t LoadULong( const char *name, nhandle_t hSection );

    int8_t LoadChar( const char *name, nhandle_t hSection );
    int16_t LoadShort( const char *name, nhandle_t hSection );
    int32_t LoadInt( const char *name, nhandle_t hSection );
    int64_t LoadLong( const char *name, nhandle_t hSection );

    void LoadVec2( const char *name, vec2_t data, nhandle_t hSection );
    void LoadVec3( const char *name, vec3_t data, nhandle_t hSection );
    void LoadVec4( const char *name, vec4_t data, nhandle_t hSection );

    void LoadCString( const char *name, char *pBuffer, int32_t maxLength, nhandle_t hSection );
    void LoadString( const char *name, string_t *pString, nhandle_t hSection );

    void LoadInt8Array( const char *name, aatc::container::tempspec::vector<int8_t> *pData, nhandle_t hSection );
    void LoadInt16Array( const char *name, aatc::container::tempspec::vector<int16_t> *pData, nhandle_t hSection );
    void LoadInt32Array( const char *name, aatc::container::tempspec::vector<int32_t> *pData, nhandle_t hSection );
    void LoadInt64Array( const char *name, aatc::container::tempspec::vector<int64_t> *pData, nhandle_t hSection );
    void LoadUInt8Array( const char *name, aatc::container::tempspec::vector<uint8_t> *pData, nhandle_t hSection );
    void LoadUInt16Array( const char *name, aatc::container::tempspec::vector<uint16_t> *pData, nhandle_t hSection );
    void LoadUInt32Array( const char *name, aatc::container::tempspec::vector<uint32_t> *pData, nhandle_t hSection );
    void LoadUInt64Array( const char *name, aatc::container::tempspec::vector<uint64_t> *pData, nhandle_t hSection );
    void LoadFloatArray( const char *name, aatc::container::tempspec::vector<float> *pData, nhandle_t hSection );

    void LoadArray( const char *pszName, CScriptArray *pData, nhandle_t hSection );

    bool Load( const char *filename );
    bool Save( void );
    bool LoadPartial( const char *filename, gamedata_t *gd );

    nhandle_t GetSection( const char *name );

    friend void G_InitArchiveHandler( void );
    friend void G_ShutdownArchiveHandler( void );
private:
    void SaveArray( const char *func, const char *name, const void *pData, uint32_t nBytes );

    void AddField( const char *name, int32_t type, const void *data, uint32_t dataSize );
    bool ValidateHeader( const void *header ) const;
    qboolean LoadArchiveFile( const char *filename, uint64_t index );
    const ngdfield_t *FindField( const char *name, int32_t type, nhandle_t hSection ) const;

    fileHandle_t m_hFile;
    ngdsection_read_t *m_pSectionCache;
    
    int64_t m_nSections;
    int64_t m_nSectionDepth;
    ngdsection_write_t m_Section;

    char **m_ArchiveFileList;
    uint64_t m_nArchiveFiles;
};

void G_InitArchiveHandler( void );
void G_ShutdownArchiveHandler( void );

extern CGameArchive *g_pArchiveHandler;

#endif

#endif