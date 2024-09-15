#ifndef __SND_LOCAL__
#define __SND_LOCAL__

#pragma once

#include "snd_public.h"

#define ERRCHECK( call ) { FMOD_RESULT result = call; if ( result != FMOD_OK ) { FMOD_Error( #call, result ); } }
void FMOD_Error( const char *call, FMOD_RESULT result );

#define MAX_SOUND_CHANNELS 1024
#define DISTANCEFACTOR 1.0f
#define MAX_SOUND_SOURCES 2048
#define MAX_MUSIC_QUEUE 12
#define MAX_SOUND_BANKS 528

#define Snd_HashFileName(x) Com_GenerateHashValue((x),MAX_SOUND_SOURCES)

class CSoundSystem;
extern CSoundSystem *sndManager;

class CSoundSource
{
	friend class CSoundSystem;
public:
	CSoundSource( void )
	{ }
	~CSoundSource()
	{ }

	void Release( void );
	bool Load( const char *npath );
	
	void Play( bool bLooping = false, uint64_t nTimeOffset = 0 );
	void Stop( void );

	inline const char *GetName( void ) const
	{ return m_szName; }
private:
	char m_szName[ MAX_NPATH ];

	FMOD::Studio::EventInstance *m_pEmitter;
	FMOD::Sound *m_pStream;
	FMOD::Channel *m_pChannel;

	CSoundSource *m_pNext;
};

class CSoundBank
{
public:
	CSoundBank( void )
	{ }
	~CSoundBank()
	{ }

	bool Load( const char *npath );
	void Shutdown( void );

	FMOD::Studio::EventDescription *GetEvent( const char *pName );
private:
	FMOD::Studio::Bank *m_pBank;
	FMOD::Studio::Bank *m_pStrings;

	FMOD::Studio::EventDescription **m_pEventList;
	int m_nEventCount;
};

typedef struct {
	FMOD_3D_ATTRIBUTES attribs;
	uint32_t nListenerMask;
	float nVolume;
} emitter_t;

typedef struct {
	vec3_t origin;
	uint32_t nListenerMask;
	float nVolume;
} listener_t;

class CSoundWorld
{
public:
	CSoundWorld( void )
	{ }
	~CSoundWorld()
	{ }

	void Update( void );
private:
	
};

typedef struct {
	char szName[ MAX_NPATH ];
	FMOD_GUID guid;
	nhandle_t hBank;
} soundEvent_t;

class CSoundSystem
{
	friend class CSoundSource;
public:
	CSoundSystem( void )
	{ }
	~CSoundSystem()
	{ }

	void Init( void );
	void Update( void );
	void Shutdown( void );

	CSoundSource *LoadSound( const char *npath );

	inline CSoundBank **GetBankList( void )
	{ return m_szBanks; }
	inline CSoundSource *GetSound( sfxHandle_t hSfx )
	{ return m_szSources[ hSfx ]; }

	inline static FMOD::Studio::System *GetStudioSystem( void )
	{ return sndManager->m_pStudioSystem; }
	inline static FMOD::System *GetCoreSystem( void )
	{ return sndManager->m_pSystem; }
	inline static FMOD::ChannelGroup *GetSFXGroup( void )
	{ return sndManager->m_pUIGroup; }

	void AddSourceToHash( CSoundSource *pSource );

	eastl::fixed_vector<CSoundSource *, 10> m_szLoopingTracks;
private:
	bool LoadBank( const char *pName );

	FMOD::Studio::System *m_pStudioSystem;
	FMOD::System *m_pSystem;

	uint64_t m_nSources;

	CSoundSource *m_szSources[ MAX_SOUND_SOURCES ];
	CSoundBank *m_szBanks[ MAX_SOUND_BANKS ];

	FMOD::ChannelGroup *m_pUIGroup;
	FMOD::ChannelGroup *m_pSFXGroup;

	FMOD::Studio::EventDescription *m_pSnapshot_Paused;
};

extern cvar_t *snd_musicOn;
extern cvar_t *snd_effectsOn;

#endif