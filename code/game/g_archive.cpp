// g_archive.cpp -- no game data archiving allowed within the vm to allow various mods and vanilla saves to work with each other

#include "g_game.h"
#include "g_archive.h"
#include "../ui/ui_lib.h"

#define NGD_MAGIC 0xff5ad1120
#define XOR_MAGIC 0xff

#define IDENT (('d'<<24)+('g'<<16)+('n'<<8)+'!')

/*

.ngd save file layout:

section    |  name         |  value               | type
--------------------------------------------------------
HEADER     | ident         | !ngd                 | int32
HEADER     | versionMajor  | NOMAD_VERSION_MAJOR  | uint16
HEADER     | versionUpdate | NOMAD_VERSION_UPDATE | uint16
HEADER     | versionPatch  | NOMAD_VERSION_PATCH  | uint32
HEADER     | numSections   | N/A                  | int64

*/

CGameArchive *g_pArchiveHandler;

enum {
	FT_CHAR,
	FT_SHORT,
	FT_INT,
	FT_LONG,

	FT_UCHAR,
	FT_USHORT,
	FT_UINT,
	FT_ULONG,

	FT_FLOAT,
	FT_VECTOR2,
	FT_VECTOR3,
	FT_VECTOR4,
	FT_STRING,

	FT_ARRAY
};

bool CGameArchive::ValidateHeader( const void *data ) const
{
	const ngdheader_t *h;

	h = (const ngdheader_t *)data;

    if ( h->validation.ident != IDENT ) {
        Con_Printf( COLOR_RED "LoadArchiveFile: failed to load save, header has incorrect identifier.\n" );
		return false;
    }

    if ( h->validation.version.m_nVersionMajor != NOMAD_VERSION
    || h->validation.version.m_nVersionUpdate != NOMAD_VERSION_UPDATE
    || h->validation.version.m_nVersionPatch != NOMAD_VERSION_PATCH ) {
        Con_Printf( COLOR_RED "LoadArchiveFile: failed to load save, header has incorrect version.\n" );
        return false;
    }

	return true;
}

static void G_ListMods( const char *pSaveFile, uint64_t nMods, fileHandle_t hFile ) {
	uint64_t i;
	char name[MAX_NPATH];
	int32_t versionMajor, versionUpdate, versionPatch;
	
	Con_Printf( "%lu mods located in save file '%s'\n", nMods, pSaveFile );
	for ( i = 0; i < nMods; i++ ) {
		FS_Read( name, sizeof( name ), hFile );
		FS_Read( &versionMajor, sizeof( versionMajor ), hFile );
		FS_Read( &versionUpdate, sizeof( versionUpdate ), hFile );
		FS_Read( &versionPatch, sizeof( versionPatch ), hFile );

		versionMajor = LittleInt( versionMajor );
		versionUpdate = LittleInt( versionUpdate );
		versionPatch = LittleInt( versionPatch );

		Con_Printf( "%lu: %s v%i.%i.%i\n", i, name, versionMajor, versionUpdate, versionPatch );
	}
}

static ngdfield_t *G_LoadArchiveField( const char *pSaveFile, fileHandle_t hFile )
{
	uint64_t size;
	int32_t nameLength;
	int32_t dataSize;
	int32_t type;
	char szName[MAX_SAVE_FIELD_NAME];
	ngdfield_t *field;

	memset( szName, 0, sizeof( szName ) );

	FS_Read( &nameLength, sizeof( nameLength ), hFile );
	nameLength = LittleInt( nameLength );

	FS_Read( szName, nameLength, hFile );
	if ( nameLength != strlen( szName ) + 1 ) {
		N_Error( ERR_DROP,
			"CGameArchive::LoadArchiveFile: failed to load save '%s', field '%s', nameLength != strlen( szName )"
		, pSaveFile, szName );
	}

	FS_Read( &type, sizeof( type ), hFile );
	type = LittleInt( type );

	switch ( type ) {
	case FT_ARRAY: {
		FS_Read( &dataSize, sizeof( dataSize ), hFile );
		if ( !dataSize ) {
			N_Error( ERR_DROP,
				"CGameArchive::LoadArchiveFile: failed to load save '%s', field '%s' (type = array), dataSize is corrupt"
			, pSaveFile, szName );
		}
		break; }
	case FT_CHAR:
	case FT_UCHAR:
		dataSize = sizeof( uint8_t );
		break;
	case FT_SHORT:
	case FT_USHORT:
		dataSize = sizeof( uint16_t );
		break;
	case FT_INT:
	case FT_UINT:
		dataSize = sizeof( uint32_t );
		break;
	case FT_LONG:
	case FT_ULONG:
		dataSize = sizeof( uint64_t );
		break;
	case FT_FLOAT:
		dataSize = sizeof( float );
		break;
	case FT_VECTOR2:
		dataSize = sizeof( vec2_t );
		break;
	case FT_VECTOR3:
		dataSize = sizeof( vec3_t );
		break;
	case FT_VECTOR4:
		dataSize = sizeof( vec4_t );
		break;
	case FT_STRING: {
		FS_Read( &dataSize, sizeof( dataSize ), hFile );
		if ( !dataSize ) {
			N_Error( ERR_DROP,
				"CGameArchive::LoadArchiveFile: failed to load save '%s', field '%s' (type = string), dataSize is corrupt"
			, pSaveFile, szName );
		}
		break; }
	};
	dataSize = LittleInt( dataSize );

	size = 0;
	size += PAD( sizeof( *field ), sizeof( uintptr_t ) );
	size += PAD( nameLength, sizeof( uintptr_t ) );
	size += PAD( dataSize, sizeof( uintptr_t ) );

	field = (ngdfield_t *)Z_Malloc( size, TAG_SAVEFILE );
	memset( field, 0, size );
	field->name = (char *)( field + 1 );
	field->dataSize = dataSize;
	field->type = type;
	field->nameLength = nameLength;
	strcpy( field->name, szName );

	return field;
}

qboolean CGameArchive::LoadArchiveFile( const char *filename, uint64_t index )
{
	ngdheader_t header;
    int64_t i, j;
	int32_t nameLength, numFields;
	uint64_t size;
	uint64_t bufSize;
    fileHandle_t hFile;
	ngdsection_read_t *section;
	ngdfield_t *field;
	ngd_file_t *file;
	char name[MAX_SAVE_SECTION_NAME];
	
	hFile = FS_FOpenRead( filename );
	if ( hFile == FS_INVALID_HANDLE ) {
		Con_Printf( COLOR_RED "ERROR: failed to open save file '%s'!\n", filename );
		return false;
	}

	N_strncpyz( name, COM_SkipPath( const_cast<char *>( filename ) ), sizeof( name ) );
	
	FS_Read( &header.validation, sizeof( header.validation ), hFile );
	header.validation.ident = LittleInt( header.validation.ident );
	header.validation.version.m_nVersionMajor = LittleShort( header.validation.version.m_nVersionMajor );
	header.validation.version.m_nVersionUpdate = LittleShort( header.validation.version.m_nVersionUpdate );
	header.validation.version.m_nVersionPatch = LittleInt( header.validation.version.m_nVersionPatch );

	if ( !ValidateHeader( &header ) ) {
		return false;
	}

	FS_Read( &header.numSections, sizeof( header.numSections ), hFile );
	FS_Read( header.gamedata.mapname, sizeof( header.gamedata.mapname ), hFile );
	FS_Read( &header.gamedata.dif, sizeof( header.gamedata.dif ), hFile );
	FS_Read( &header.gamedata.numMods, sizeof( header.gamedata.numMods ), hFile );

	header.numSections = LittleLong( header.numSections );
	header.gamedata.dif = (gamedif_t)LittleInt( header.gamedata.dif );
	header.gamedata.numMods = LittleLong( header.gamedata.numMods );

	// skip past the mod data
	G_ListMods( name, header.gamedata.numMods, hFile );
	//FS_FileSeek( hFile, ( MAX_NPATH + ( sizeof( int32_t ) * 3 ) ) * header.gamedata.numMods, FS_SEEK_CUR );

	size = PAD( sizeof( *file ) + ( sizeof( *file->m_pSectionList ) * header.numSections ), sizeof( uintptr_t ) );

	file = (ngd_file_t *)Z_Malloc( size, TAG_SAVEFILE );
	memset( file, 0, size );

	file->m_pSectionList = (ngdsection_read_t *)( file + 1 );
	strcpy( file->name, name );
	file->m_nSections = header.numSections;
	section = file->m_pSectionList;

	Con_DPrintf( "Adding save file '%s' to cache with %li sections...\n", name, header.numSections );
	
	for ( i = 0; i < header.numSections; i++ ) {		
		FS_Read( &nameLength, sizeof( nameLength ), hFile );
		nameLength = LittleInt( nameLength );
		if ( !nameLength || nameLength >= MAX_SAVE_SECTION_NAME ) {
			N_Error( ERR_DROP, "CGameArchive::LoadArchiveFile: failed to load save file '%s', section %li nameLength is invalid"
				, name, i );
		}
		FS_Read( name, nameLength, hFile );
		if ( nameLength != strlen( name ) + 1 ) {
			N_Error( ERR_DROP, "CGameArchive::LoadArchiveFile: failed to load save file '%s', section %li nameLength != strlen( name )"
				, name, i );
		}
		FS_Read( &numFields, sizeof( numFields ), hFile );
		numFields = LittleInt( numFields );

		memset( section, 0, sizeof( *section ) );
		section->m_FieldCache.reserve( numFields );
		section->numFields = numFields;
		strcpy( section->name, name );
		section->nameLength = nameLength;

		for ( j = 0; j < section->numFields; j++ ) {
			field = G_LoadArchiveField( name, hFile );
			section->m_FieldCache.try_emplace( field->name, field );
			FS_FileSeek( hFile, field->dataSize, FS_SEEK_CUR );
		}
		section++;
	}
	FS_FClose( hFile );

	m_pArchiveCache[ index ] = file;
	m_hFile = hFile;
	
	return qtrue;
}

static void G_SaveGame_f( void )
{
	const char *filename;
	char path[MAX_NPATH];

	filename = Cmd_Argv( 1 );

	if ( *filename ) {
		COM_StripExtension( filename, path, sizeof( path ) );
		COM_DefaultExtension( path, sizeof( path ), ".ngd" );
		filename = Cvar_VariableString( "sgame_SaveName" );
	} else {
		Com_snprintf( path, sizeof( path ) - 1, "%s", Cvar_VariableString( "sgame_SaveName" ) );
		COM_DefaultExtension( path, sizeof( path ), ".ngd" );
		filename = path;
	}
	g_pArchiveHandler->Save( path );
}

CGameArchive::CGameArchive( void )
{
	uint64_t i;
	char **fileList;

	Con_Printf( "G_InitArchiveHandler: initializing save file cache...\n" );

	fileList = FS_ListFiles( "SaveData", ".ngd", &m_nArchiveFiles );
	m_pArchiveCache = (ngd_file_t **)Z_Malloc( sizeof( *m_pArchiveCache ) * m_nArchiveFiles, TAG_SAVEFILE );

	m_pArchiveFileList = (char **)Z_Malloc( sizeof( *m_pArchiveFileList ) * m_nArchiveFiles, TAG_SAVEFILE );
	for ( i = 0; i < m_nArchiveFiles; i++ ) {
		m_pArchiveFileList[i] = (char *)Z_Malloc( strlen( fileList[i] ) + 1, TAG_SAVEFILE );
		strcpy( m_pArchiveFileList[i], fileList[i] );
		LoadArchiveFile( fileList[i], i );

		Con_Printf( "...Cached save file '%s'\n", fileList[i] );
	}

	FS_FreeFileList( fileList );
}

void G_InitArchiveHandler( void )
{
	if ( g_pArchiveHandler ) {
		return;
	}

	g_pArchiveHandler = new ( Hunk_Alloc( sizeof( *g_pArchiveHandler ), h_low ) ) CGameArchive();
	Cmd_AddCommand( "sgame.save_game", G_SaveGame_f );
}

void G_ShutdownArchiveHandler( void ) {
	g_pArchiveHandler->~CGameArchive();
	g_pArchiveHandler = NULL;
	Cmd_RemoveCommand( "sgame.save_game" );
}

const char **CGameArchive::GetSaveFiles( uint64_t *nFiles ) const {
	*nFiles = m_nArchiveFiles;
	return (const char **)m_pArchiveFileList;
}

void CGameArchive::BeginSaveSection( const char *moduleName, const char *name )
{
	const char *path;
	int64_t nameLength = strlen( name ) + 1;
	
	if ( nameLength >= MAX_SAVE_SECTION_NAME ) {
		N_Error( ERR_DROP, "CGameArchive::AddSection: section name '%s' is longer than %i characters, please shorten, like seriously",
			name, MAX_SAVE_SECTION_NAME - 1 );
	}
	if ( m_nSectionDepth >= MAX_SAVE_SECTION_DEPTH ) {
		N_Error( ERR_DROP, "CGameArchive::AddSection: section stack overflow" );
	}

	Con_DPrintf( "Adding section '%s' to archive file...\n", name );

	m_pSection = &m_szSectionStack[ m_nSectionDepth++ ];

	memset( m_pSection, 0, sizeof( *m_pSection ) );
	m_pSection->offset = FS_FileTell( m_hFile );
	m_pSection->numFields = 0;
	m_pSection->nameLength = strlen( name ) + 1;
	N_strncpyz( m_pSection->name, name, sizeof( m_pSection->name ) );

	FS_Write( &m_pSection->nameLength, sizeof( m_pSection->nameLength ), m_hFile );
	FS_Write( m_pSection->name, m_pSection->nameLength, m_hFile );
	FS_Write( &m_pSection->numFields, sizeof( m_pSection->numFields ), m_hFile );
#ifdef SAVEFILE_MOD_SAFETY
	path = va( "SaveData/%s/%s.prt", moduleName, name );
	m_pSection->hFile = FS_FOpenWrite( path );
	if ( m_pSection->hFile == FS_INVALID_HANDLE ) {
		N_Error( ERR_DROP, "CGameArchive::BeginSaveSection: failed to create file '%s' in write-only mode", path );
	}
	FS_Write( &m_pSection->nameLength, sizeof( m_pSection->nameLength ), m_pSection->hFile );
	FS_Write( m_pSection->name, m_pSection->nameLength, m_pSection->hFile );
	FS_Write( &m_pSection->numFields, sizeof( m_pSection->numFields ), m_pSection->hFile );
#endif
}

void CGameArchive::EndSaveSection( void )
{
	const uint64_t pos = FS_FileTell( m_hFile );

	FS_FileSeek( m_hFile, m_pSection->offset, FS_SEEK_SET );
	FS_Write( &m_pSection->nameLength, sizeof( m_pSection->nameLength ), m_hFile );
	FS_Write( m_pSection->name, m_pSection->nameLength, m_hFile );
	FS_Write( &m_pSection->numFields, sizeof( m_pSection->numFields ), m_hFile );
	FS_FileSeek( m_hFile, pos, FS_SEEK_SET );

#ifdef SAVEFILE_MOD_SAFETY
	FS_FileSeek( m_pSection->hFile, 0, FS_SEEK_SET );
	FS_Write( &m_pSection->nameLength, sizeof( m_pSection->nameLength ), m_pSection->hFile );
	FS_Write( m_pSection->name, m_pSection->nameLength, m_pSection->hFile );
	FS_Write( &m_pSection->numFields, sizeof( m_pSection->numFields ), m_pSection->hFile );
	FS_FClose( m_pSection->hFile );
#endif

	Con_DPrintf( "Finished save section '%s' with %i fields\n", m_pSection->name, m_pSection->numFields );
	
	m_nSectionDepth--;
	m_nSections++;

	m_pSection = &m_szSectionStack[ m_nSectionDepth - 1 ];
}

void CGameArchive::AddField( const char *name, int32_t type, const void *data, uint32_t dataSize )
{
	ngdfield_t field;
	int64_t i;
	
	m_pSection->numFields++;

	field.type = type;
	field.dataSize = dataSize;
	field.nameLength = strlen( name ) + 1;

	if ( field.nameLength >= MAX_SAVE_FIELD_NAME ) {
		N_Error( ERR_DROP, "%s: name '%s' too long", __func__, name );
	}
	
	for ( i = 0; i < m_nSectionDepth; i++ ) {
		Con_DPrintf( "-" );
	}
	Con_DPrintf( "> Adding field %s to save file.\n", name );

	FS_Write( &field.nameLength, sizeof( field.nameLength ), m_hFile );
	FS_Write( name, field.nameLength, m_hFile );
	FS_Write( &field.type, sizeof( field.type ), m_hFile );
	FS_Write( data, dataSize, m_hFile );
#ifdef SAVEFILE_MOD_SAFETY
	FS_Write( &field.nameLength, sizeof( field.nameLength ), m_pSection->hFile );
	FS_Write( name, field.nameLength, m_pSection->hFile );
	FS_Write( &field.type, sizeof( field.type ), m_pSection->hFile );
	FS_Write( data, dataSize, m_pSection->hFile );
#endif
}

void CGameArchive::SaveFloat( const char *name, float data ) {
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}

	AddField( name, FT_FLOAT, &data, sizeof(data) );
}

void CGameArchive::SaveByte( const char *name, uint8_t data ) {
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	
	AddField( name, FT_UCHAR, &data, sizeof(data) );
}
void CGameArchive::SaveUShort( const char *name, uint16_t data ) {
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	
	AddField( name, FT_USHORT, &data, sizeof(data) );
}
void CGameArchive::SaveUInt( const char *name, uint32_t data ) {
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	
	AddField( name, FT_UINT, &data, sizeof(data) );
}
void CGameArchive::SaveULong( const char *name, uint64_t data ) {
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	
	AddField( name, FT_ULONG, &data, sizeof(data) );
}

void CGameArchive::SaveChar( const char *name, int8_t data ) {
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	
	AddField( name, FT_CHAR, &data, sizeof(data) );
}
void CGameArchive::SaveShort( const char *name, int16_t data ) {
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	
	AddField( name, FT_SHORT, &data, sizeof(data) );
}
void CGameArchive::SaveInt( const char *name, int32_t data ) {
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	
	AddField( name, FT_INT, &data, sizeof(data) );
}
void CGameArchive::SaveLong( const char *name, int64_t data ) {
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	
	AddField( name, FT_LONG, &data, sizeof(data) );
}

void CGameArchive::SaveVec2( const char *name, const vec2_t data ) {
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	
	AddField( name, FT_VECTOR2, data, sizeof(vec2_t) );
}

void CGameArchive::SaveVec3( const char *name, const vec3_t data ) {
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	
	AddField( name, FT_VECTOR3, data, sizeof(vec3_t) );
}

void CGameArchive::SaveVec4( const char *name, const vec4_t data ) {
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	
	AddField( name, FT_VECTOR4, data, sizeof(vec4_t) );
}

void CGameArchive::SaveCString( const char *name, const char *data ) {
	ngdfield_t field;

	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !data ) {
		N_Error( ERR_DROP, "%s: data is NULL", __func__ );
	}

	m_pSection->numFields++;

	field.nameLength = strlen( name ) + 1;
	field.type = FT_STRING;
	field.dataSize = strlen( data ) + 1;
	if ( field.nameLength >= MAX_SAVE_FIELD_NAME ) {
		N_Error( ERR_DROP, "%s: name '%s' too long", __func__, name );
	}

	FS_Write( &field.nameLength, sizeof( field.nameLength ), m_hFile );
	FS_Write( name, field.nameLength, m_hFile );
	FS_Write( &field.type, sizeof( field.type ), m_hFile );
	FS_Write( &field.dataSize, sizeof(field.dataSize), m_hFile );
	FS_Write( data, field.dataSize, m_hFile );
#ifdef SAVEFILE_MOD_SAFETY
	FS_Write( &field.nameLength, sizeof( field.nameLength ), m_pSection->hFile );
	FS_Write( name, field.nameLength, m_pSection->hFile );
	FS_Write( &field.type, sizeof( field.type ), m_pSection->hFile );
	FS_Write( &field.dataSize, sizeof(field.dataSize), m_pSection->hFile );
	FS_Write( data, field.dataSize, m_pSection->hFile );
#endif
}

void CGameArchive::SaveString( const char *name, const string_t *pData ) {
	ngdfield_t field;

	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !pData ) {
		N_Error( ERR_DROP, "%s: data is NULL", __func__ );
	}

	m_pSection->numFields++;

	field.nameLength = strlen( name ) + 1;
	field.type = FT_STRING;
	field.dataSize = pData->size() + 1;
	if ( field.nameLength >= MAX_SAVE_FIELD_NAME ) {
		N_Error( ERR_DROP, "%s: name '%s' too long", __func__, name );
	}
	
	FS_Write( &field.nameLength, sizeof( field.nameLength ), m_hFile );
	FS_Write( name, field.nameLength, m_hFile );
	FS_Write( &field.type, sizeof( field.type ), m_hFile );
	FS_Write( &field.dataSize, sizeof( field.dataSize ), m_hFile );
	FS_Write( pData->data(), field.dataSize, m_hFile );
#ifdef SAVEFILE_MOD_SAFETY
	FS_Write( &field.nameLength, sizeof( field.nameLength ), m_pSection->hFile );
	FS_Write( name, field.nameLength, m_pSection->hFile );
	FS_Write( &field.type, sizeof( field.type ), m_pSection->hFile );
	FS_Write( &field.dataSize, sizeof( field.dataSize ), m_pSection->hFile );
	FS_Write( pData->data(), field.dataSize, m_pSection->hFile );
#endif
}

void CGameArchive::SaveArray( const char *name, const CScriptArray *pData )
{
	SaveArray( __func__, name, pData->GetBuffer(), pData->GetSize() *
		g_pModuleLib->GetScriptEngine()->GetTypeInfoById( pData->GetElementTypeId() )->GetSize() );
}

void CGameArchive::SaveArray( const char *func, const char *name, const void *pData, uint32_t nBytes ) {
	ngdfield_t field;

	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", func );
	}
	if ( !pData ) {
		N_Error( ERR_DROP, "%s: data is NULL", func );
	}

	m_pSection->numFields++;

	field.nameLength = strlen( name ) + 1;
	field.type = FT_STRING;
	field.dataSize = nBytes;
	if ( field.nameLength >= MAX_SAVE_FIELD_NAME ) {
		N_Error( ERR_DROP, "%s: name '%s' too long", func, name );
	}

	FS_Write( &field.nameLength, sizeof( field.nameLength ), m_hFile );
	FS_Write( name, field.nameLength, m_hFile );
	FS_Write( &field.type, sizeof( field.type ), m_hFile );
	FS_Write( &field.dataSize, sizeof( field.dataSize ), m_hFile );
	FS_Write( pData, nBytes, m_hFile );
#ifdef SAVEFILE_MOD_SAFETY
	FS_Write( &field.nameLength, sizeof( field.nameLength ), m_pSection->hFile );
	FS_Write( name, field.nameLength, m_pSection->hFile );
	FS_Write( &field.type, sizeof( field.type ), m_pSection->hFile );
	FS_Write( &field.dataSize, sizeof( field.dataSize ), m_pSection->hFile );
	FS_Write( pData, nBytes, m_pSection->hFile );
#endif
}

const ngdfield_t *CGameArchive::FindField( const char *name, int32_t type, nhandle_t hSection ) const
{
	const ngdsection_read_t *section;

	section = &m_pArchiveCache[ m_nCurrentArchive ]->m_pSectionList[ hSection ];
	const auto it = section->m_FieldCache.find( name );

	if ( it == section->m_FieldCache.end() ) {
		// we'll let the modder handle this
		Con_Printf( COLOR_YELLOW "WARNING: incompatible mod with save file, couldn't find field '%s'\n", name );
		return NULL;
	}
	if ( it->second->type != type ) {
		N_Error( ERR_DROP, "CGameArchive::FindField: save file corrupt or incompatible mod, field type doesn't match type given for '%s'", name );
	}

	FS_FileSeek( m_hFile, it->second->dataOffset, FS_SEEK_SET );

	return it->second;
}

float CGameArchive::LoadFloat( const char *name, nhandle_t hSection ) {
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_FLOAT, hSection );
	if ( !FS_Read( (void *)&field->data, field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}
	
	return field->data.f;
}

uint8_t CGameArchive::LoadByte( const char *name, nhandle_t hSection ) {
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_UCHAR, hSection );
	if ( !FS_Read( (void *)&field->data, field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}
	
	return field->data.u8;
}

uint16_t CGameArchive::LoadUShort( const char *name, nhandle_t hSection ) {
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_USHORT, hSection );
	if ( !FS_Read( (void *)&field->data, field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}
	
	return field->data.u16;
}

uint32_t CGameArchive::LoadUInt( const char *name, nhandle_t hSection ) {
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_UINT, hSection );
	if ( !FS_Read( (void *)&field->data, field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}
	
	return field->data.u32;
}

uint64_t CGameArchive::LoadULong( const char *name, nhandle_t hSection ) {
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_ULONG, hSection );
	if ( !FS_Read( (void *)&field->data, field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}
	
	return field->data.u64;
}

int8_t CGameArchive::LoadChar( const char *name, nhandle_t hSection ) {
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_CHAR, hSection );
	if ( !FS_Read( (void *)&field->data, field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}
	
	return field->data.s8;
}

int16_t CGameArchive::LoadShort( const char *name, nhandle_t hSection ) {
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_SHORT, hSection );
	if ( !FS_Read( (void *)&field->data, field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}
	
	return field->data.s16;
}

int32_t CGameArchive::LoadInt( const char *name, nhandle_t hSection ) {
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_INT, hSection );
	if ( !FS_Read( (void *)&field->data, field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}

	return field->data.s32;
}

int64_t CGameArchive::LoadLong( const char *name, nhandle_t hSection ) {
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_LONG, hSection );
	if ( !FS_Read( (void *)&field->data, field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}
	
	return field->data.s64;
}

void CGameArchive::LoadVec2( const char *name, vec2_t data, nhandle_t hSection )
{
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_VECTOR2, hSection );
	if ( !FS_Read( (void *)&field->data, field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}
	
	VectorCopy2( data, field->data.v2 );
}

void CGameArchive::LoadVec3( const char *name, vec3_t data, nhandle_t hSection )
{
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_VECTOR3, hSection );
	if ( !FS_Read( (void *)&field->data, field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}
	
	VectorCopy( data, field->data.v3 );
}

void CGameArchive::LoadVec4( const char *name, vec4_t data, nhandle_t hSection )
{
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}

	field = FindField( name, FT_VECTOR4, hSection );
	if ( !FS_Read( (void *)&field->data, field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}

	VectorCopy4( data, field->data.v2 );
}

void CGameArchive::LoadCString( const char *name, char *pBuffer, int32_t maxLength, nhandle_t hSection ) {
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_STRING, hSection );

	// FIXME:
	
	N_strncpyz( pBuffer, field->data.str, maxLength );
}

void CGameArchive::LoadString( const char *name, string_t *pString, nhandle_t hSection ) {
	const ngdfield_t *field;
	
	if ( !name ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}
	
	field = FindField( name, FT_STRING, hSection );
	
	if ( pString->size() < field->dataSize ) {
		pString->resize( field->dataSize );
	}
	memset( pString->data(), 0, pString->size() );
	if ( !FS_Read( pString->data(), field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, name );
	}
}

void CGameArchive::LoadArray( const char *pszName, CScriptArray *pData, nhandle_t hSection ) {
	const ngdfield_t *field;
	uint32_t dataSize;

	if ( !pszName ) {
		N_Error( ERR_DROP, "%s: name is NULL", __func__ );
	}
	if ( !hSection ) {
		N_Error( ERR_DROP, "%s: hSection is invalid", __func__ );
	}

	field = FindField( pszName, FT_ARRAY, hSection );

	dataSize = g_pModuleLib->GetScriptEngine()->GetTypeInfoById( pData->GetElementTypeId() )->GetSize();

	// this should never happen
	if ( field->dataSize % dataSize ) {
		N_Error( ERR_DROP, "%s: bad data type for module array (funny field size)", __func__ );
	}
	if ( pData->GetSize() < ( field->dataSize / dataSize ) ) {
		pData->Resize( field->dataSize / dataSize );
	}

	Con_DPrintf( "Successfully loaded array field '%s' containing %u bytes.\n", field->name, field->dataSize );

	if ( !FS_Read( pData->GetBuffer(), field->dataSize, m_hFile ) ) {
		N_Error( ERR_DROP, "%s: failed to read field '%s'", __func__, pszName );
	}
}

bool CGameArchive::Save( const char *filename )
{
	const char *path;
	ngdheader_t header;
	char **partFiles;
	uint64_t nPartFiles;
	union {
		void *v;
		char *b;
	} f;
	uint64_t length, i, size;
	char *namePtr;

	PROFILE_FUNCTION();

	if ( m_nSectionDepth ) {
		N_Error( ERR_DROP, "CGameArchive::Save: called when writing a section" );
	}
	
//	g_pModuleLib->ModuleCall( sgvm, ModuleOnSaveGame, 0 );

	path = va( "SaveData/%s", filename );
	m_hFile = FS_FOpenWrite( path );
	if ( m_hFile == FS_INVALID_HANDLE ) {
		Con_Printf( COLOR_RED "ERROR: failed to create save file '%s'!\n", path );
		return false;
	}

	CModuleInfo *loadList = g_pModuleLib->GetLoadList();
	
	memset( &header, 0, sizeof( header ) );
	N_strncpyz( header.gamedata.mapname, Cvar_VariableString( "mapname" ), sizeof( header.gamedata.mapname ) );
	header.gamedata.dif = (gamedif_t)Cvar_VariableInteger( "sgame_Difficulty" );
	header.gamedata.numMods = g_pModuleLib->GetModCount();

	for ( i = 0; i < g_pModuleLib->GetModCount(); i++ ) {
		if ( !loadList[i].m_pHandle->IsValid() ) {
			header.gamedata.numMods--;
		}
	}

	m_nSectionDepth = 0;
	m_nSections = 0;
	
	header.validation.ident = IDENT;
	header.validation.version.m_nVersionMajor = NOMAD_VERSION;
	header.validation.version.m_nVersionUpdate = NOMAD_VERSION_UPDATE;
	header.validation.version.m_nVersionPatch = NOMAD_VERSION_PATCH;

	FS_Write( &header.validation, sizeof( header.validation ), m_hFile );
	FS_Write( &header.numSections, sizeof( header.numSections ), m_hFile );
	FS_Write( header.gamedata.mapname, sizeof( header.gamedata.mapname ), m_hFile );
	FS_Write( &header.gamedata.dif, sizeof( header.gamedata.dif ), m_hFile );
	FS_Write( &header.gamedata.numMods, sizeof( header.gamedata.numMods ), m_hFile );

	for ( i = 0; i < header.gamedata.numMods; i++ ) {
		if ( loadList[i].m_pHandle->IsValid() ) {
			FS_Write( loadList[i].m_szName, sizeof( loadList[i].m_szName ), m_hFile );
			FS_Write( &loadList[i].m_nModVersionMajor, sizeof( loadList[i].m_nModVersionMajor ), m_hFile );
			FS_Write( &loadList[i].m_nModVersionUpdate, sizeof( loadList[i].m_nModVersionUpdate ), m_hFile );
			FS_Write( &loadList[i].m_nModVersionPatch, sizeof( loadList[i].m_nModVersionPatch ), m_hFile );
		}
	}

	for ( i = 0; i < g_pModuleLib->GetModCount(); i++ ) {
		Con_DPrintf( "Adding module '%s' save sections...\n", loadList[i].m_szName );

		g_pModuleLib->ModuleCall( &loadList[i], ModuleOnSaveGame, 0 );
/*
		for ( i = 0; i < nPartFiles; i++ ) {
			length = FS_LoadFile( partFiles[i], &f.v );
			if ( !length || !f.v ) {
				N_Error( ERR_DROP, "CGameArchive::Save: failed to load save section file '%s'", partFiles[i] );
			}

			Con_DPrintf( "Writing save section part '%s' to archive file...\n", partFiles[i] );

			// the section data is already there, so we just need to write it to the actual save file
			FS_Write( f.v, length, m_hFile );
	
			FS_FreeFile( f.v );
		}

		FS_FreeFileList( partFiles );
		*/
	}

	header.numSections = m_nSections;
	FS_FileSeek( m_hFile, 0, FS_SEEK_SET );

	FS_Write( &header.validation, sizeof( header.validation ), m_hFile );
	FS_Write( &header.numSections, sizeof( header.numSections ), m_hFile );
	
	FS_FClose( m_hFile );
	
	//
	// save to steam
	//
#ifdef NOMAD_STEAM_BUILD
#endif

	//
	// reset the cache
	//
	Z_FreeTags( TAG_SAVEFILE );
	CGameArchive();

	return true;
}

nhandle_t CGameArchive::GetSection( const char *name )
{
	ngdsection_read_t *section;
	int64_t i;

	section = NULL;
	for ( i = 0; i < m_pArchiveCache[ m_nCurrentArchive ]->m_nSections; i++ ) {
		if ( !N_strcmp( m_pArchiveCache[ m_nCurrentArchive ]->m_pSectionList[i].name, name ) ) {
			section = &m_pArchiveCache[ m_nCurrentArchive ]->m_pSectionList[i];
			break;
		}
	}
	if ( !section ) {
		N_Error( ERR_DROP, "CGameArchive::GetSection: compatibility issue with save file section '%s', section not found in file", name );
	}
	
	return i;
}

bool CGameArchive::LoadPartial( const char *filename, gamedata_t *gd )
{
    fileHandle_t f;
    ngdheader_t header;
	uint64_t i, size;
	char *namePtr;

    f = FS_FOpenRead( filename );
    if ( f == FS_INVALID_HANDLE ) {
        return false;
    }

    //
    // validate the header
    //

	size = sizeof( ngdvalidation_t ) + sizeof( uint64_t ) + sizeof( gamedif_t ) + MAX_NPATH;
    if ( FS_FileLength( f ) < size ) {
        Con_Printf( COLOR_RED "CGameArchive::Load: failed to load savefile because the file is too small to contain a header.\n" );
        return false;
    }

	FS_Read( &header.validation, sizeof( header.validation ), f );
	FS_Read( &header.numSections, sizeof( header.numSections ), f );
	FS_Read( gd->mapname, sizeof( gd->mapname ), f );
	FS_Read( &gd->dif, sizeof( gd->dif ), f );
	FS_Read( &gd->numMods, sizeof( gd->numMods ), f );

	if ( gd->numMods ) {
		gd->modList = (modlist_t *)Z_Malloc( sizeof( *gd->modList ) * gd->numMods, TAG_SAVEFILE );
		for ( i = 0; i < gd->numMods; i++ ) {
			FS_Read( gd->modList[i].name, sizeof( gd->modList[i].name ), f );
			FS_Read( &gd->modList[i].nVersionMajor, sizeof( gd->modList[i].nVersionMajor ), f );
			FS_Read( &gd->modList[i].nVersionUpdate, sizeof( gd->modList[i].nVersionUpdate ), f );
			FS_Read( &gd->modList[i].nVersionPatch, sizeof( gd->modList[i].nVersionPatch ), f );
		}
	}
	Con_DPrintf(
				"Loaded partial header:\n"
				" [mapname] %s\n"
				" [difficulty] %i\n"
				" [modCount] %lu\n"
				" [modList] "
	, gd->mapname, (int32_t)gd->dif, gd->numMods );

	for ( i = 0; i < gd->numMods; i++ ) {
		Con_DPrintf( "%s", gd->modList[i].name );
		if ( i != gd->numMods - 1 ) {
			Con_DPrintf( ", " );
		}
	}
	Con_DPrintf( "\n" );

    if ( !ValidateHeader( &header ) ) {
        return false;
    }

    FS_FClose( f );

    return true;
}

bool CGameArchive::Load( const char *name )
{
	uint64_t i;
	char szName[MAX_NPATH];
	uint64_t offset;
	bool found;

	N_strncpyz( szName, name, sizeof( szName ) );
	COM_DefaultExtension( szName, sizeof( szName ), ".ngd" );

	Con_Printf( "Loading save file '%s', please do not close out of the game...\n", szName );

	m_nCurrentArchive = m_nArchiveFiles;
	found = false;
	for ( i = 0; i < m_nArchiveFiles; i++ ) {
		if ( !N_stricmp( szName, m_pArchiveCache[i]->name ) ) {
			m_nCurrentArchive = i;
			found = true;
			break;
		}
	}
	if ( !found ) {
		N_Error( ERR_DROP, "CGameArchive::Load: attempted to load non-existing save file (... HOW?)" );
	}

	for ( i = 0; i < m_pArchiveCache[ m_nCurrentArchive ]->m_nSections; i++ ) {
		Con_DPrintf( "%lu: %-24s %-8i\n", i, m_pArchiveCache[ m_nCurrentArchive ]->m_pSectionList[i].name,
			m_pArchiveCache[ m_nCurrentArchive ]->m_pSectionList[i].size );
	}

	CModuleInfo *loadList = g_pModuleLib->GetLoadList();

	m_hFile = FS_FOpenRead( szName );
	if ( m_hFile == FS_INVALID_HANDLE ) {
		N_Error( ERR_DROP, "CGameArchive::Load: failed to open save file '%s'", szName );
	}

	offset = 0;
	offset += sizeof( ngdvalidation_t );
	offset += sizeof( uint64_t ); // numSections
	offset += MAX_NPATH; // mapname
	offset += sizeof( gamedif_t ); // difficulty
	offset += m_pArchiveCache[ m_nCurrentArchive ]->m_nMods * ( MAX_NPATH + ( sizeof( int32_t ) * 3 ) );

	FS_FileSeek( m_hFile, offset, FS_SEEK_CUR );
	for ( i = 0; i < g_pModuleLib->GetModCount(); i++ ) {
		g_pModuleLib->ModuleCall( &loadList[i], ModuleOnLoadGame, 0 );
	}
	FS_FClose( m_hFile );

	return true;
}
