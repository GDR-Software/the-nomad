#include "g_game.h"
#include "g_sound.h"
#include "../engine/n_threads.h"

#include <ALsoft/al.h>
#include <ALsoft/alc.h>
#include <ALsoft/alext.h>
#include <sndfile.h>
//#include <ogg/ogg.h>
//#include <vorbis/vorbisfile.h>

#define MAX_SOUND_SOURCES 1024
#define MAX_MUSIC_QUEUE 12

#define CLAMP_VOLUME 10.0f

cvar_t *snd_musicvol;
cvar_t *snd_sfxvol;
cvar_t *snd_musicon;
cvar_t *snd_sfxon;
cvar_t *snd_mastervol;
cvar_t *snd_debugPrint;

#define Snd_HashFileName(x) Com_GenerateHashValue((x),MAX_SOUND_SOURCES)

#define SNDBUF_FLOAT 0
#define SNDBUF_DOUBLE 1
#define SNDBUF_16BIT 2
#define SNDBUF_8BIT 3

#define ALCall(x) x; AL_CheckError(#x)

static void AL_CheckError( const char *op )
{
    ALenum error;

    error = alGetError();
    switch ( error ) {
    case AL_OUT_OF_MEMORY:
    #ifdef _NOMAD_DEBUG
        N_Error( ERR_FATAL, "alGetError() -- 0x%04x, AL_OUT_OF_MEMORY after '%s'", AL_OUT_OF_MEMORY, op );
    #else
        Con_Printf( COLOR_RED "WARNING: alGetError() -- 0x%04x, AL_OUT_OF_MEMORY after '%s'\n", AL_OUT_OF_MEMORY, op );
        Con_Printf( "Dumping stacktrace of 64 frames...\n" );
        Sys_DebugStacktrace( 64 );
    #endif
        break;
    case AL_INVALID_ENUM:
    #ifdef _NOMAD_DEBUG
        N_Error( ERR_FATAL, "alGetError() -- 0x%04x, AL_ILLEGAL_ENUM after '%s'", AL_INVALID_ENUM, op );
    #else
        Con_Printf( COLOR_RED "WARNING: alGetError() -- 0x%04x, AL_INVALID_ENUM after '%s'\n", AL_INVALID_ENUM, op );
        Con_Printf( "Dumping stacktrace of 64 frames...\n" );
        Sys_DebugStacktrace( 64 );
    #endif
        break;
    case AL_INVALID_OPERATION:
    #ifdef _NOMAD_DEBUG
        N_Error( ERR_FATAL, "alGetError() -- 0x%04x, AL_INVALID_OPERATION after '%s'", AL_INVALID_OPERATION, op );
    #else
        Con_Printf( COLOR_RED "WARNING: alGetError() -- 0x%04x, AL_INVALID_OPERATION after '%s'\n", AL_INVALID_OPERATION, op );
        Con_Printf( "Dumping stacktrace of 64 frames...\n" );
        Sys_DebugStacktrace( 64 );
    #endif
        break;
    case AL_INVALID_VALUE:
    #ifdef _NOMAD_DEBUG
        N_Error( ERR_FATAL, "alGetError() -- 0x%04x, AL_INVALID_VALUE after '%s'", AL_INVALID_VALUE, op );
    #else
        Con_Printf( COLOR_RED "WARNING: alGetError() -- 0x%04x, AL_INVALID_VALUE after '%s'\n", AL_INVALID_VALUE, op );
        Con_Printf( "Dumping stacktrace of 64 frames...\n" );
        Sys_DebugStacktrace( 64 );
    #endif
        break;
    case AL_INVALID_NAME:
    #ifdef _NOMAD_DEBUG
        N_Error( ERR_FATAL, "alGetError() -- 0x%04x, AL_INVALID_NAME after '%s'", AL_INVALID_NAME, op );
    #else
        Con_Printf( COLOR_RED "WARNING: alGetError() -- 0x%04x, AL_INVALID_NAME after '%s'\n", AL_INVALID_NAME, op );
        Con_Printf( "Dumping stacktrace of 64 frames...\n" );
        Sys_DebugStacktrace( 64 );
    #endif
        break;
    case AL_NO_ERROR:
    default:
        if ( snd_debugPrint->i ) {
            Con_DPrintf( "OpenAL operation '%s' all good.\n", op );
        }
        break;
    };
}

class CSoundSource
{
public:
    CSoundSource( void );
    ~CSoundSource();

    void Init( void );
    void Shutdown( void );
    bool LoadFile( const char *npath, int64_t tag );

    void SetVolume( void ) const;

    bool IsPlaying( void ) const;
    bool IsPaused( void ) const;
    bool IsLooping( void ) const;

    const char *GetName( void ) const;
    uint32_t GetTag( void ) const { return m_iTag; }

    inline uint32_t GetSource( void ) const { return m_iSource; }
    inline uint32_t GetBuffer( void ) const { return m_iBuffer; }
    inline void SetSource( uint32_t source ) { m_iSource = source; }

    void Play( bool loop = false );
    void Stop( void );
    void Pause( void );

    CSoundSource *m_pNext;
private:
    int64_t FileFormat( const char *ext ) const;
    void Alloc( void );
    ALenum Format( void ) const;

    char m_pName[MAX_NPATH];

    uint32_t m_iType;
    uint32_t m_iTag;

    // AL data
    uint32_t m_iSource;
    uint32_t m_iBuffer;

    bool m_bLoop;

    SF_INFO m_hFData;
};

typedef struct trackQueue_s {
    struct trackQueue_s *next;
    struct trackQueue_s *prev;
    CSoundSource *track;
    qboolean used;
} trackQueue_t;

class CSoundManager
{
public:
    CSoundManager( void );
    ~CSoundManager();

    void Init( void );
    void Shutdown( void );
    void Restart( void );

    void PlaySound( CSoundSource *snd );
    void StopSound( CSoundSource *snd );
    void LoopTrack( CSoundSource *snd );
    void DisableSounds( void );
    void Update( int64_t msec );

    inline CSoundSource **GetSources( void ) { return m_pSources; }

    void UpdateParm( int64_t tag );
    uint32_t GetMusicSource( void ) const { return m_iMusicSource; }

    void AddSourceToHash( CSoundSource *src );
    CSoundSource *InitSource( const char *filename, int64_t tag );

    inline uint64_t NumSources( void ) const { return m_nSources; }
    inline CSoundSource *GetSource( sfxHandle_t handle ) { return m_pSources[handle]; }
    inline const CSoundSource *GetSource( sfxHandle_t handle ) const { return m_pSources[handle]; }

    CSoundSource *m_pCurrentTrack, *m_pQueuedTrack;

    CThreadMutex m_hAllocLock;
    CThreadMutex m_hQueueLock;
private:
    CSoundSource *m_pSources[MAX_SOUND_SOURCES];
    uint64_t m_nSources;

    ALCdevice *m_pDevice;
    ALCcontext *m_pContext;

    uint32_t m_iMusicSource;

    qboolean m_bClearedQueue;
    qboolean m_bRegistered;
};

static CSoundManager *sndManager;

CSoundSource::CSoundSource( void ) {
    Init();
}

CSoundSource::~CSoundSource() {
    Shutdown();
}

const char *CSoundSource::GetName( void ) const {
    return m_pName;
}

bool CSoundSource::IsPaused( void ) const {
    ALint state;
    ALCall( alGetSourcei( m_iSource, AL_SOURCE_STATE, &state ) );
    return state == AL_PAUSED;
}

bool CSoundSource::IsPlaying( void ) const {
    ALint state;
    ALCall( alGetSourcei( m_iSource, AL_SOURCE_STATE, &state ) );
    return state == AL_PLAYING;
}

bool CSoundSource::IsLooping( void ) const {
    ALint state;
    ALCall( alGetSourcei( m_iSource, AL_SOURCE_STATE, &state ) );
    return state == AL_LOOPING;
}

void CSoundSource::Init( void )
{
    memset( m_pName, 0, sizeof( m_pName ) );
    memset( &m_hFData, 0, sizeof( m_hFData ) );

    // this could be the music source, so don't allocate a new redundant source just yet
    m_iSource = 0;

    if ( m_iBuffer == 0 ) {
        ALCall( alGenBuffers( 1, &m_iBuffer) );
    }
    m_iType = 0;
    m_bLoop = false;
}

void CSoundSource::Shutdown( void )
{
    if ( m_iTag != TAG_MUSIC ) {
        // stop the source if we're playing anything
        if ( IsPlaying() ) {
            ALCall( alSourceStop( m_iSource ) );
        }

        ALCall( alSourcei( m_iSource, AL_BUFFER, AL_NONE ) );
        ALCall( alDeleteSources( 1, &m_iSource ) );
    }
    ALCall( alDeleteBuffers( 1, &m_iBuffer ) );

    m_iBuffer = 0;
    m_iSource = 0;
    m_iType = 0;
}


void CSoundSource::SetVolume( void ) const
{
    ALCall(alSourcef( m_iSource, AL_GAIN, snd_sfxvol->f / CLAMP_VOLUME ));
}

ALenum CSoundSource::Format( void ) const
{
    return m_hFData.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
}

int64_t CSoundSource::FileFormat( const char *ext ) const
{
    if (!N_stricmp( ext, "wav" )) {
        return SF_FORMAT_WAV;
    } else if (!N_stricmp( ext, "aiff" )) {
        return SF_FORMAT_AIFF;
    } else if (!N_stricmp( ext, "ogg" )) {
        return SF_FORMAT_OGG;
    } else if (!N_stricmp( ext, "opus" )) {
        return SF_FORMAT_OPUS;
    } else if (!N_stricmp( ext, "flac" )) {
        return SF_FORMAT_FLAC;
    } else if (!N_stricmp( ext, "sd2" )) {
        return SF_FORMAT_SD2;
    } else {
        Con_Printf( COLOR_YELLOW "WARNING: unknown audio file format extension '%s', refusing to load\n", ext );
        return 0;
    }
}

void CSoundSource::Alloc( void )
{
    switch ( m_hFData.format & SF_FORMAT_SUBMASK ) {
    case SF_FORMAT_ALAW:
        break;
    case SF_FORMAT_ULAW:
        break;
    case SF_FORMAT_FLOAT:
        m_iType = SNDBUF_FLOAT;
        break;
    case SF_FORMAT_DOUBLE:
        m_iType = SNDBUF_DOUBLE;
        break;
    case SF_FORMAT_PCM_S8:
    case SF_FORMAT_PCM_U8:
    case SF_FORMAT_DPCM_8:
        m_iType = SNDBUF_8BIT;
        break;
    case SF_FORMAT_ALAC_16:
    case SF_FORMAT_PCM_16:
    case SF_FORMAT_DPCM_16:
    case SF_FORMAT_DWVW_16:
    case SF_FORMAT_NMS_ADPCM_16:
        m_iType = SNDBUF_16BIT;
        break;
    };
}

void CSoundSource::Play( bool loop )
{
    if ( IsPlaying() || ( IsLooping() && loop ) ) {
        return;
    }
    if ( loop ) {
        ALCall( alSourcei( m_iSource, AL_LOOPING, AL_TRUE ) );
    }
    ALCall( alSourcePlay( m_iSource ) );
}

void CSoundSource::Pause( void ) {
    if ( !IsPlaying() && !IsLooping() ) {
        return;
    }
    ALCall( alSourcePause( m_iSource ) );
}

void CSoundSource::Stop( void ) {
    if ( !IsPlaying() && !IsLooping() ) {
        return; // nothing's playing
    }
    ALCall( alSourceStop( m_iSource ) );
}

static sf_count_t SndFile_Read( void *data, sf_count_t size, void *file ) {
    return FS_Read( data, size, *(fileHandle_t *)file );
}

static sf_count_t SndFile_Tell( void *file ) {
    return FS_FileTell( *(fileHandle_t *)file );
}

static sf_count_t SndFile_GetFileLen(void *file) {
    return FS_FileLength( *(fileHandle_t *)file );
}

static sf_count_t SndFile_Seek( sf_count_t offset, int64_t whence, void *file ) {
    fileHandle_t f = *(fileHandle_t *)file;
    switch ( whence ) {
    case SEEK_SET:
        return FS_FileSeek( f, (fileOffset_t)offset, FS_SEEK_SET );
    case SEEK_CUR:
        return FS_FileSeek( f, (fileOffset_t)offset, FS_SEEK_CUR );
    case SEEK_END:
        return FS_FileSeek( f, (fileOffset_t)offset, FS_SEEK_END );
    default:
        break;
    };
    N_Error( ERR_FATAL, "SndFile_Seek: bad whence" );
    return 0; // quiet compiler warning
}

bool CSoundSource::LoadFile( const char *npath, int64_t tag )
{
    SNDFILE *sf;
    SF_VIRTUAL_IO vio;
    ALenum format;
    fileHandle_t f;
    FILE *fp;
    void *buffer;
    uint64_t length;
    const char *ospath;
    void *data;
    uint64_t dataSize;

    m_iTag = tag;

    // clear audio file data before anything
    memset( &m_hFData, 0, sizeof( m_hFData ) );
    memset( &vio, 0, sizeof( vio ) );

    N_strncpyz( m_pName, npath, sizeof( m_pName ) );

    // hash it so that if we try loading it
    // even if it's failed, we won't try loading
    // it again
    sndManager->AddSourceToHash( this );

    length = FS_LoadFile( npath, &buffer );

    if ( !length || !buffer ) {
        Con_Printf( COLOR_RED "CSoundSource::LoadFile: failed to load file '%s'.\n", npath );
        return false;
    }

    fp = tmpfile();
    AssertMsg( fp, "Failed to open temprorary file!" );

/*
    vio.get_filelen = SndFile_GetFileLen;
    vio.write = NULL; // no need for this
    vio.read = SndFile_Read;
    vio.tell = SndFile_Tell;
    vio.seek = SndFile_Seek;

    sf = sf_open_virtual( &vio, SFM_READ, &m_hFData, &f );
    if (!sf) {
        Con_Printf(COLOR_YELLOW "WARNING: libsndfile sf_open_virtual failed on '%s', sf_sterror(): %s\n", npath, sf_strerror( sf ));
        return false;
    }
*/
    fwrite( buffer, length, 1, fp );
    fseek( fp, 0L, SEEK_SET );
    
    sf = sf_open_fd( fileno( fp ), SFM_READ, &m_hFData, SF_FALSE );
    if ( !sf ) {
        Con_Printf( COLOR_YELLOW "WARNING: libsndfile sf_open_fd failed on '%s', sf_strerror(): %s\n", npath, sf_strerror( sf ) );
        return false;
    }
    
    // allocate the buffer
    Alloc();

    switch ( m_iType ) {
    default:
    case SNDBUF_16BIT:
        dataSize = sizeof( short );
        break;
    case SNDBUF_8BIT:
        dataSize = sizeof( char );
        break;
    case SNDBUF_DOUBLE:
        dataSize = sizeof( double );
        break;
    case SNDBUF_FLOAT:
        dataSize = sizeof( float );
        break;
    };

    dataSize *= m_hFData.channels * m_hFData.frames;
    data = Hunk_AllocateTempMemory( dataSize );

    if ( !sf_read_raw( sf, data, dataSize ) ) {
        N_Error( ERR_FATAL, "CSoundSource::LoadFile(%s): failed to read %lu bytes from audio stream, sf_strerror(): %s\n",
            npath, dataSize, sf_strerror( sf ) );
    }
    
    sf_close( sf );
    fclose( fp );

    format = Format();
    if ( format == 0 ) {
        Con_Printf( COLOR_RED "Bad soundfile format for '%s', refusing to load\n", npath );
        return false;
    }

    // generate a brand new source for each individual sfx
    if ( tag == TAG_SFX && m_iSource == 0 ) {
        ALCall( alGenSources( 1, &m_iSource ) );
    }

    ALCall( alBufferData( m_iBuffer, format, data, dataSize, m_hFData.samplerate ) );
    ALCall( alSourcei( m_iSource, AL_BUFFER, m_iBuffer ) );

    if ( tag == TAG_SFX ) {
        ALCall( alSourcef( m_iSource, AL_GAIN, snd_sfxvol->f ) );
    } else if (tag == TAG_MUSIC) {
        ALCall( alSourcef( m_iSource, AL_GAIN, snd_musicvol->f ) );
    }

    Hunk_FreeTempMemory( data );
    FS_FreeFile( buffer );

    return true;
}

void CSoundManager::Init( void )
{
    memset( this, 0, sizeof( *this ) );

    m_pDevice = alcOpenDevice( NULL );
    if ( !m_pDevice ) {
        N_Error( ERR_FATAL, "Snd_Init: failed to open OpenAL device" );
    }

    m_pContext = alcCreateContext( m_pDevice, NULL );
    if ( !m_pContext ) {
        N_Error( ERR_FATAL, "Snd_Init: failed to create OpenAL context, reason: %s", alcGetString( m_pDevice, alcGetError( m_pDevice ) ) );
    }

    alcMakeContextCurrent( m_pContext );
    m_bRegistered = true;

    // generate the recyclable music source
    ALCall( alGenSources( 1, &m_iMusicSource ) );
}

void CSoundManager::PlaySound( CSoundSource *snd ) {
    snd->Play();
}

void CSoundManager::StopSound( CSoundSource *snd ) {
    snd->Stop();
}


void CSoundManager::Shutdown( void )
{
    uint64_t i;
    trackQueue_t *pTrack, *pNext;

    for ( i = 0; i < m_nSources; i++ ) {
        m_pSources[i]->Shutdown();
    }

    memset( m_pSources, 0, sizeof( m_pSources ) );
    m_nSources = 0;
    m_bRegistered = false;

    ALCall( alDeleteSources( 1, &m_iMusicSource ) );

    Z_FreeTags( TAG_SFX, TAG_MUSIC );

    Cmd_RemoveCommand( "snd.setvolume" );
    Cmd_RemoveCommand( "snd.toggle" );
    Cmd_RemoveCommand( "snd.updatevolume" );
    Cmd_RemoveCommand( "snd.list_files" );

    alcMakeContextCurrent( NULL );
    alcDestroyContext( m_pContext );
    alcCloseDevice( m_pDevice );
}

void CSoundManager::AddSourceToHash( CSoundSource *src )
{
    nhandle_t hash;

    hash = Snd_HashFileName( src->GetName() );

    src->m_pNext = m_pSources[hash];
    m_pSources[hash] = src;
}

void CSoundManager::Restart( void )
{
    uint64_t i;

    // shutdown everything
    Shutdown();
    
    // re-init
    Init();

    // reload all sources
    for ( i = 0; i < m_nSources; i++ ) {
        m_pSources[i]->Init();
        m_pSources[i]->LoadFile( m_pSources[i]->GetName(), m_pSources[i]->GetTag() );
    }
}

//
// CSoundManager::InitSource: allocates a new sound source with openal
// NOTE: even if sound and music is disabled, we'll still allocate the memory,
// we just won't play any of the sources
//
CSoundSource *CSoundManager::InitSource( const char *filename, int64_t tag )
{
    CSoundSource *src;
    nhandle_t hash;

    hash = Snd_HashFileName( filename );

    //
    // check if we already have it loaded
    //
    for ( src = m_pSources[hash]; src; src = src->m_pNext ) {
        if ( !N_stricmp( filename, src->GetName() ) ) {
            return src;
        }
    }

    if ( strlen( filename ) >= MAX_NPATH ) {
        Con_Printf( "CSoundManager::InitSource: name '%s' too long\n", filename );
        return NULL;
    }
    if ( m_nSources == MAX_SOUND_SOURCES ) {
        N_Error( ERR_DROP, "CSoundManager::InitSource: MAX_SOUND_SOURCES hit" );
    }

    m_hAllocLock.Lock();

    src = (CSoundSource *)Z_Malloc( sizeof( *src ), (memtag_t)tag );
    memset( src, 0, sizeof( *src ) );
    src->Init();

    if (tag == TAG_MUSIC) {
        if ( !alIsSource( m_iMusicSource ) ) { // make absolutely sure its a valid source
            ALCall( alGenSources( 1, &m_iMusicSource ));
        }
        src->SetSource( m_iMusicSource );
    }

    if ( !src->LoadFile( filename, tag ) ) {
        Con_Printf( COLOR_YELLOW "WARNING: failed to load sound file '%s'\n", filename );
        m_hAllocLock.Unlock();
        return NULL;
    }

    src->SetVolume();

    m_hAllocLock.Unlock();

    return src;
}

void CSoundManager::UpdateParm( int64_t tag )
{
    for ( uint64_t i = 0; i < m_nSources; i++ ) {
        if ( m_pSources[i]->GetTag() == tag ) {
            m_pSources[i]->SetVolume();
        }
    }
}

void CSoundManager::DisableSounds( void ) {
    for ( uint64_t i = 0; i < m_nSources; i++ ) {
        m_pSources[i]->Stop();
    }
}


void Snd_DisableSounds( void ) {
    sndManager->DisableSounds();
    ALCall( alListenerf( AL_GAIN, 0.0f ) );
}

void Snd_StopAll( void ) {
    sndManager->DisableSounds();
}

void Snd_PlaySfx(sfxHandle_t sfx) {
    if ( sfx == FS_INVALID_HANDLE || !snd_sfxon->i ) {
        return;
    }

    sndManager->PlaySound( sndManager->GetSource( sfx ) );
}

void Snd_StopSfx( sfxHandle_t sfx ) {
    if ( sfx == FS_INVALID_HANDLE || !snd_sfxon->i ) {
        return;
    }

    sndManager->StopSound( sndManager->GetSource( sfx ) );
}

void Snd_Restart( void ) {
    if ( !sndManager ) {
        return;
    }
    sndManager->Restart();
}

void Snd_Shutdown( void ) {
    if ( !sndManager ) {
        return;
    }
    sndManager->Shutdown();
}

sfxHandle_t Snd_RegisterTrack( const char *npath ) {
    CSoundSource *track;

    track = sndManager->InitSource( npath, TAG_MUSIC );
    if ( !track ) {
        return FS_INVALID_HANDLE;
    }

    return Snd_HashFileName( track->GetName() );
}

sfxHandle_t Snd_RegisterSfx( const char *npath ) {
    CSoundSource *sfx;

    sfx = sndManager->InitSource( npath, TAG_SFX );
    if ( !sfx ) {
        return FS_INVALID_HANDLE;
    }

    return Snd_HashFileName( sfx->GetName() );
}

void Snd_SetLoopingTrack( sfxHandle_t handle ) {
    CSoundSource *track;
    trackQueue_t *pTrack;

    if ( !snd_musicon->i ) {
        return;
    }

    if ( handle == FS_INVALID_HANDLE ) {
        Con_Printf( COLOR_RED "Snd_SetLoopingTrack: invalid handle, ignoring call.\n" );
        return;
    }

    CThreadAutoLock<CThreadMutex> lock( sndManager->m_hQueueLock );
    track = sndManager->GetSource( handle );
    if ( !track ) {
        Con_Printf( COLOR_RED "Snd_SetLoopingTrack: invalid handle, ignoring call.\n" );
        return;
    } else if ( sndManager->m_pCurrentTrack == track ) {
        return; // already playing
    }

    if ( !sndManager->m_pCurrentTrack ) {
        sndManager->m_pCurrentTrack = track;
        sndManager->m_pCurrentTrack->Play( true );
        sndManager->m_pQueuedTrack = NULL;
    } else {
        ALCall( alSourcei( sndManager->m_pCurrentTrack->GetSource(), AL_LOOPING, AL_FALSE ) );
        sndManager->m_pQueuedTrack = track;
    }
}

void Snd_ClearLoopingTrack( void ) {
    if ( !snd_musicon->i || !sndManager->m_pCurrentTrack || !sndManager->m_pCurrentTrack->IsLooping() ) {
        return;
    }

    // stop the track and pop it
    sndManager->m_pCurrentTrack->Stop();

    // play the next track
    if ( sndManager->m_pQueuedTrack ) {
        sndManager->m_pCurrentTrack = sndManager->m_pQueuedTrack;
        sndManager->m_pCurrentTrack->Play();
        ALCall( alSourcei( sndManager->m_pCurrentTrack->GetSource(), AL_LOOPING, AL_TRUE ) );
        sndManager->m_pQueuedTrack = NULL;
    }
}

static void Snd_AudioInfo_f( void )
{
    Con_Printf( "\n----- Audio Info -----\n" );
    Con_Printf( "Audio Driver: %s\n", SDL_GetCurrentAudioDriver() );
    Con_Printf( "Current Track: %s\n", sndManager->m_pCurrentTrack ? sndManager->m_pCurrentTrack->GetName() : "None" );
    Con_Printf( "Number of Sound Sources: %lu\n", sndManager->NumSources() );
}

static void Snd_Toggle_f( void )
{
    const char *var;
    const char *toggle;
    bool option;

    if ( Cmd_Argc() < 3 ) {
        Con_Printf( "usage: snd.toggle <sfx|music> <on|off, 1|0>\n" );
        return;
    }

    var = Cmd_Argv( 1 );
    toggle = Cmd_Argv( 2 );

    if ( ( toggle[0] == '1' && toggle[1] == '\0' ) || !N_stricmp( toggle, "on" ) ) {
        option = true;
    }
    else if ( ( toggle[0] == '0' && toggle[1] == '\0' ) || !N_stricmp( toggle, "off" ) ) {
        option = false;
    }

    if ( !N_stricmp( var, "sfx" ) ) {
        Cvar_Set( "snd_sfxon", va( "%i", option ) );
    }
    else if ( !N_stricmp( var, "music" ) ) {
        Cvar_Set( "snd_musicon", va( "%i", option ) );
    }
    else {
        Con_Printf( "snd.toggle: unknown parameter '%s', use either 'sfx' or 'music'\n", var );
        return;
    }
}

static void Snd_SetVolume_f( void )
{
    float vol;
    const char *change;

    if ( Cmd_Argc() < 3 ) {
        Con_Printf( "usage: snd.setvolume <sfx|music> <volume>\n" );
        return;
    }

    vol = N_atof( Cmd_Argv( 1 ) );
    vol = CLAMP( vol, 0.0f, 100.0f );

    change = Cmd_Argv( 2 );
    if ( !N_stricmp( change, "sfx" ) ) {
        if ( snd_sfxvol->f != vol ) {
            Cvar_Set( "snd_sfxvol", va("%f", vol) );
            sndManager->UpdateParm( TAG_SFX );
        }
    }
    else if ( !N_stricmp( change, "music" ) ) {
        if ( snd_musicvol->f != vol ) {
            Cvar_Set( "snd_musicvol", va( "%f", vol ) );
            sndManager->UpdateParm( TAG_MUSIC );
        }
    }
    else {
        Con_Printf( "snd.setvolume: unknown parameter '%s', use either 'sfx' or 'music'\n", change );
        return;
    }
}

static void Snd_UpdateVolume_f( void ) {
    sndManager->UpdateParm( TAG_SFX );
    sndManager->UpdateParm( TAG_MUSIC );
}

/*
* Snd_Update: checks all sound system cvars for updates
*/
void Snd_Update( int32_t msec )
{
    if ( snd_mastervol->modified ) {
        snd_sfxvol->modified = qtrue;
        snd_musicvol->modified = qtrue;
    }
    if ( snd_sfxvol->modified ) {
        for ( uint64_t i = 0; i < sndManager->NumSources(); i++ ) {
            if ( sndManager->GetSources()[i]->GetTag() == TAG_SFX ) {
                sndManager->GetSources()[i]->SetVolume();
            }
        }
        snd_sfxvol->modified = qfalse;
    }
    if ( snd_musicvol->modified ) {
        ALCall( alSourcef( sndManager->GetMusicSource(), AL_GAIN, CLAMP( snd_musicvol->f, 0.0f, snd_mastervol->f ) / CLAMP_VOLUME ) );
        snd_musicvol->modified = qfalse;
    }
    if ( sndManager->m_pCurrentTrack && !sndManager->m_pCurrentTrack->IsPlaying() || sndManager->m_pQueuedTrack ) {
        Snd_ClearLoopingTrack();
    }
}

static void Snd_ListFiles_f( void )
{
    Con_Printf( "\n---------- Snd_ListFiles_f ----------\n" );
    Con_Printf( "Total sound files loaded: %lu\n", sndManager->NumSources() );

    for ( uint64_t i = 0; i < sndManager->NumSources(); i++ ) {
        Con_Printf( "[Sound File #%lu]\n", i );
        Con_Printf( "path: %s\n", sndManager->GetSources()[i]->GetName() );
    }
}

void Snd_Init( void )
{
    snd_sfxon = Cvar_Get( "snd_sfxon", "1", CVAR_SAVE | CVAR_LATCH );
    Cvar_CheckRange( snd_sfxon, "0", "1", CVT_INT );
    Cvar_SetDescription( snd_sfxon, "Toggles sound effects on or off." );

    snd_musicon = Cvar_Get( "snd_musicon", "1", CVAR_SAVE | CVAR_LATCH );
    Cvar_CheckRange( snd_musicon, "0", "1", CVT_INT );
    Cvar_SetDescription( snd_musicon, "Toggles music on or off." );

    snd_sfxvol = Cvar_Get( "snd_sfxvol", "50", CVAR_SAVE | CVAR_LATCH );
    Cvar_CheckRange( snd_sfxvol, "0", "100", CVT_INT );
    Cvar_SetDescription( snd_sfxvol, "Sets global sound effects volume." );

    snd_musicvol = Cvar_Get( "snd_musicvol", "80", CVAR_SAVE | CVAR_LATCH );
    Cvar_CheckRange( snd_musicvol, "0", "100", CVT_INT );
    Cvar_SetDescription( snd_musicvol, "Sets volume for music." );

    snd_mastervol = Cvar_Get( "snd_mastervol", "80", CVAR_SAVE | CVAR_LATCH );
    Cvar_CheckRange( snd_mastervol, "0", "100", CVT_INT );
    Cvar_SetDescription( snd_mastervol, "Sets the cap for sfx and music volume." );

    snd_debugPrint = Cvar_Get( "snd_debugPrint", "0", CVAR_CHEAT | CVAR_TEMP );
    Cvar_CheckRange( snd_debugPrint, "0", "1", CVT_INT );
    Cvar_SetDescription( snd_debugPrint, "Toggles OpenAL-soft debug messages, don't use if you aren't a developer." );

    Con_Printf("---------- Snd_Init ----------\n");

    // init sound manager
    sndManager = (CSoundManager *)Z_Malloc( sizeof(*sndManager), TAG_GAME );
    sndManager->Init();

    Cmd_AddCommand( "snd.setvolume", Snd_SetVolume_f );
    Cmd_AddCommand( "snd.toggle", Snd_Toggle_f );
    Cmd_AddCommand( "snd.updatevolume", Snd_UpdateVolume_f );
    Cmd_AddCommand( "snd.list_files", Snd_ListFiles_f );
}
