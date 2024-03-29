#include "../engine/n_shared.h"
#include "sg_local.h"

void SG_Init( void );
void SG_Shutdown( void );
int SG_RunLoop( int levelTime, int frameTime );
int SG_DrawFrame( void );

void SaveGame( void );
void LoadGame( void );

typedef struct {
    qboolean initialized;
    int maxGfx;
    int difficulty;
} sgameSettings_t;

static sgameSettings_t settings;

static void SG_SaveSettings( void );
static void SG_DrawSettings( void );

/*
vmMain

this is the only way control passes into the module.
this must be the very first function compiled into the .qvm file
*/
int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7,
    int arg8, int arg9, int arg10 )
{
    switch ( command ) {
    case SGAME_RUNTIC:
        return SG_RunLoop( arg0, arg1 );
    case SGAME_CONSOLE_COMMAND:
        SGameCommand();
        return 0;
    case SGAME_MOUSE_EVENT:
        return 0;
    case SGAME_KEY_EVENT:
        SG_KeyEvent( arg0, arg1 );
        return 0;
    case SGAME_LOADLEVEL:
        return SG_StartLevel();
    case SGAME_CONSOLE_COMMAND:
        SGameCommand();
        return 0;
    case SGAME_INIT:
        SG_Init();
        return 0;
    case SGAME_SHUTDOWN:
        SG_Shutdown();
        return 0;
    case SGAME_GET_STATE:
        return sg.state;
    case SGAME_ENDLEVEL:
        return SG_EndLevel();
    case SGAME_DRAW_ADVANCED_SETTINGS:
        SG_DrawSettings();
        return 0;
    case SGAME_SAVE_SETTINGS:
        SG_SaveSettings();
        return 0;
    case SGAME_LOAD_GAME:
        LoadGame();
        return 0;
    case SGAME_SAVE_GAME:
        SaveGame();
        return 0;
    case SGAME_EVENT_HANDLING:
    case SGAME_EVENT_NONE:
        return 0;
    default:
        break;
    };

    SG_Error( "vmMain: unrecognized command %i", command );
    return -1;
}

sgGlobals_t sg;

vmCvar_t sg_printEntities;
vmCvar_t sg_debugPrint;
vmCvar_t sg_paused;
vmCvar_t sg_mouseInvert;
vmCvar_t sg_mouseAcceleration;
vmCvar_t sg_printLevelStats;
vmCvar_t sg_decalDetail;
vmCvar_t sg_gibs;
vmCvar_t sg_levelIndex;
vmCvar_t sg_savename;
vmCvar_t sg_gameDifficulty;
vmCvar_t sg_numSaves;
vmCvar_t sg_memoryDebug;
vmCvar_t sg_maxGfx;

vmCvar_t pm_groundFriction;
vmCvar_t pm_waterFriction;
vmCvar_t pm_airFriction;
vmCvar_t pm_waterAccel;
vmCvar_t pm_baseAccel;
vmCvar_t pm_baseSpeed;
vmCvar_t pm_airAccel;
vmCvar_t pm_wallrunAccelVertical;
vmCvar_t pm_wallrunAccelMove;
vmCvar_t pm_wallTime;

vmCvar_t sgc_infiniteHealth;
vmCvar_t sgc_infiniteRage;
vmCvar_t sgc_infiniteAmmo;
vmCvar_t sgc_blindMobs;
vmCvar_t sgc_deafMobs;
vmCvar_t sg_cheatsOn;
vmCvar_t sgc_godmode;

typedef struct {
    vmCvar_t *vmCvar;
    const char *cvarName;
    const char *defaultValue;
    int cvarFlags;
    int modificationCount; // for tracking changes
    qboolean trackChange;       // track this variable, and announce if changed
} cvarTable_t;

static cvarTable_t cvarTable[] = {
    // noset vars
    { NULL,                     "gamename",             GLN_VERSION,    CVAR_ROM,                   0, qfalse },
    { NULL,                     "gamedate",             __DATE__,       CVAR_ROM,                   0, qfalse },
    { &sg_printEntities,        "sg_printEntities",     "0",            0,                          0, qfalse },
    { &sg_debugPrint,           "sg_debugPrint",        "1",            CVAR_TEMP,                  0, qfalse },
    { &sg_paused,               "g_paused",             "1",            CVAR_TEMP | CVAR_LATCH,     0, qfalse },
    { &pm_groundFriction,       "pm_groundFriction",    "0.6f",         CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &pm_waterFriction,        "pm_waterFriction",     "0.06f",        CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &pm_airFriction,          "pm_airFriction",       "0.01f",        CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &pm_airAccel,             "pm_airAccel",          "1.5f",         CVAR_LATCH | CVAR_SAVE,     0, qfalse },
    { &pm_waterAccel,           "pm_waterAccel",        "0.5f",         CVAR_LATCH | CVAR_SAVE,     0, qfalse },
    { &pm_baseAccel,            "pm_baseAccel",         "1.0f",         CVAR_LATCH | CVAR_SAVE,     0, qfalse },
    { &pm_baseSpeed,            "pm_baseSpeed",         "0.02f",        CVAR_LATCH | CVAR_SAVE,     0, qfalse },
    { &sg_mouseInvert,          "g_mouseInvert",        "0",            CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &sg_mouseAcceleration,    "g_mouseAcceleration",  "0",            CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &sg_printLevelStats,      "sg_printLevelStats",   "1",            CVAR_LATCH | CVAR_SAVE,     0, qfalse },
    { &sg_decalDetail,          "sg_decalDetail",       "3",            CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &sg_gibs,                 "sg_gibs",              "0",            CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &sg_gameDifficulty,       "sg_gameDifficulty",    "2",            CVAR_LATCH | CVAR_TEMP,     0, qtrue },
    { &sg_savename,             "sg_savename",          "savedata",     CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &sg_numSaves,             "sg_numSaves",          "0",            CVAR_LATCH | CVAR_SAVE,     0, qfalse },
    { &sg_maxGfx,               "sg_maxGfx",            "1024",         CVAR_LATCH | CVAR_SAVE,     0, qtrue },
#ifdef _NOMAD_DEBUG
    { &sg_memoryDebug,          "sg_memoryDebug",       "1",            CVAR_LATCH | CVAR_TEMP,     0, qfalse },
#else
    { &sg_memoryDebug,          "sg_memoryDebug",       "0",            CVAR_LATCH | CVAR_TEMP,     0, qfalse },
#endif
    { &sgc_infiniteHealth,      "sgc_infiniteHealth",   "0",            CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &sgc_infiniteAmmo,        "sgc_infiniteAmmo",     "0",            CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &sgc_infiniteRage,        "sgc_infiniteRage",     "0",            CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &sgc_godmode,             "sgc_godmode",          "0",            CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &sgc_blindMobs,           "sgc_blindMobs",        "0",            CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &sgc_deafMobs,            "sgc_deafMobs",         "0",            CVAR_LATCH | CVAR_SAVE,     0, qtrue },
    { &sg_cheatsOn,             "sg_cheatsOn",          "0",            CVAR_LATCH | CVAR_SAVE,     0, qtrue },
};

static const int cvarTableSize = arraylen(cvarTable);

static void SG_RegisterCvars( void )
{
    int i;
    cvarTable_t *cv;

    for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ ) {
        Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultValue, cv->cvarFlags );
        if ( cv->vmCvar ) {
            cv->modificationCount = cv->vmCvar->modificationCount;
        }
    }
}

void SG_UpdateCvars( void )
{
    int i;
    cvarTable_t *cv;

    for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ ) {
        if ( cv->vmCvar ) {
            Cvar_Update( cv->vmCvar );

            if ( cv->modificationCount != cv->vmCvar->modificationCount ) {
                cv->modificationCount = cv->vmCvar->modificationCount;

                if ( cv->trackChange ) {
                    trap_SendConsoleCommand( va( "Changed \"%s\" to \"%s\"", cv->cvarName, cv->vmCvar->s ) );
                }
            }
        }
    }
}

void GDR_ATTRIBUTE((format(printf, 1, 2))) GDR_DECL G_Printf( const char *fmt, ... )
{
    va_list argptr;
    char msg[MAXPRINTMSG];
    int length;

    va_start( argptr, fmt );
    length = vsprintf( msg, fmt, argptr );
    va_end( argptr );

    if ( length >= sizeof(msg) ) {
        trap_Error( "G_Printf: buffer overflow" );
    }

    trap_Print( msg );
}

void GDR_ATTRIBUTE((format(printf, 1, 2))) GDR_DECL G_Error( const char *fmt, ... )
{
    va_list argptr;
    char msg[MAXPRINTMSG];
    int length;

    va_start( argptr, fmt );
    length = vsprintf( msg, fmt, argptr );
    va_end( argptr );

    if ( length >= sizeof(msg) ) {
        trap_Error( "G_Error: buffer overflow" );
    }

    trap_Error( msg );
}

void GDR_ATTRIBUTE((format(printf, 1, 2))) GDR_DECL SG_Printf( const char *fmt, ... )
{
    va_list argptr;
    char msg[MAXPRINTMSG];
    int length;

    va_start( argptr, fmt );
    length = vsprintf( msg, fmt, argptr );
    va_end( argptr );

    if ( length >= sizeof(msg) ) {
        trap_Error( "SG_Printf: buffer overflow" );
    }

    trap_Print( msg );
}

void GDR_ATTRIBUTE((format(printf, 1, 2))) GDR_DECL SG_Error( const char *fmt, ... )
{
    va_list argptr;
    char msg[MAXPRINTMSG];
    int length;

    va_start( argptr, fmt );
    length = vsprintf( msg, fmt, argptr );
    va_end( argptr );

    if ( length >= sizeof(msg) ) {
        trap_Error( "SG_Error: buffer overflow" );
    }

    trap_Error( msg );
}

//#ifndef SGAME_HARD_LINKED
// this is only here so the functions in n_shared.c and bg_*.c can link

void GDR_DECL GDR_ATTRIBUTE((format(printf, 1, 2))) Con_Printf( const char *fmt, ... )
{
    va_list argptr;
    char msg[MAXPRINTMSG];
    int length;

    va_start( argptr, fmt );
    length = vsprintf( msg, fmt, argptr );
    va_end( argptr );

    if ( length >= sizeof(msg) ) {
        trap_Error( "SG_Printf: buffer overflow" );
    }

    trap_Print( msg );
}

void GDR_DECL GDR_ATTRIBUTE((format(printf, 2, 3))) N_Error( errorCode_t code, const char *err, ... )
{
    va_list argptr;
    char msg[MAXPRINTMSG];
    int length;

    va_start( argptr, err );
    length = vsprintf( msg, err, argptr );
    va_end( argptr );

    if ( length >= sizeof(msg) ) {
        trap_Error( "N_Error: buffer overflow" );
    }

    trap_Error( msg );
}

//#endif

int SG_RunLoop( int levelTime, int frameTime )
{
    int i;
    int start, end;
    int msec;
    sgentity_t *ent;

    if ( sg.state == SG_INACTIVE ) {
        return 0;
    }

    // get any cvar changes
    SG_UpdateCvars();

    if ( sg_paused.i ) {
        return 0;
    }

    sg.framenum++;
    sg.previousTime = sg.framenum;
    sg.levelTime = levelTime;
    msec = sg.levelTime - sg.previousTime;

    //
    // go through all allocated entities
    //
    start = Sys_Milliseconds();
    ent = &sg_entities[0];
    for ( i = 0; i < sg.numEntities; i++) {
        if ( !ent->health ) {
            continue;
        }

        ent->ticker--;

        if ( ent->ticker <= -1 ) {
            Ent_SetState( ent, ent->state->nextstate );
            continue;
        }

        // update the current entity's animation frame
        if ( ent->state->frames > 0 ) {
            if ( ent->frame == ent->state->frames ) {
                ent->frame = 0; // reset animation
            }
            else if ( ent->ticker % ent->state->frames ) {
                ent->frame++;
            }
        }

        ent->state->action.acp1( ent );
    }
    end = Sys_Milliseconds();

    SG_DrawFrame();

    if ( sg_printEntities.i ) {
        for ( i = 0; i < sg.numEntities; i++ ) {
            G_Printf( "%4i: %s\n", i, sg_entities[i].classname );
        }
        Cvar_Set( "sg_printEntities", "0" );
    }

    return 1;
}

char *SG_LoadFile( const char *filename )
{
    int len;
    fileHandle_t f;
    static char text[20000];

    len = trap_FS_FOpenFile( filename, &f, FS_OPEN_READ );
    if ( !len ) {
        SG_Error( "SG_LoadFile: failed to open file %s", filename );
    }
    if ( len >= sizeof(text) ) {
        SG_Error( "SG_LoadFile: file %s too long", filename );
    }
    trap_FS_Read( text, len, f );
    trap_FS_FClose( f );

    return text;
}

nhandle_t SG_LoadResource( const char *name, nhandle_t (*fn)( const char * ) )
{
    nhandle_t handle;

    G_Printf( "Loading Resource %s...\n", name );
    handle = fn( name );
    if ( handle == FS_INVALID_HANDLE ) {
        G_Printf( COLOR_YELLOW "WARNING: Failed to load %s.\n", name );
    }

    return handle;
}

static void SG_LoadMedia( void )
{
    int i;
    int start, end;

    SG_Printf( "Loading media...\n" );

    start = Sys_Milliseconds();

    //
    // shaders
    //
    for ( i = 0; i < arraylen( sg.media.bloodSplatterShader ); i++ ) {
        sg.media.bloodSplatterShader[i] = SG_LoadResource( va( "gfx/bloodSplatter%i", i ), RE_RegisterShader );
    }
    sg.media.bulletMarkShader = SG_LoadResource( "gfx/bulletMarks", RE_RegisterShader );

    //
    // sfx
    //
    sg.media.footstepsGround = SG_LoadResource( "sfx/player/footstepsGround.ogg", Snd_RegisterSfx );
    sg.media.footstepsMetal = SG_LoadResource( "sfx/player/footstepsMetal.ogg", Snd_RegisterSfx );
    sg.media.footstepsWater = SG_LoadResource( "sfx/player/footstepsWater.ogg", Snd_RegisterSfx );
    sg.media.footstepsWood = SG_LoadResource( "sfx/player/footstepsWood.ogg", Snd_RegisterSfx );

    sg.media.grappleHit = SG_LoadResource( "sfx/player/grappleHit.ogg", Snd_RegisterSfx );

    sg.media.bladeModeEnter = SG_LoadResource( "sfx/player/bladeModeEnter.ogg", Snd_RegisterSfx );

    end = Sys_Milliseconds();

    if ( sg_debugPrint.i ) {
        SG_Printf( "Loaded resource files in %i msec.\n", end - start );
    }

    //
    // levels
    //

    SG_Printf( "Initializing level configs...\n" );

    start = Sys_Milliseconds();
    SG_LoadLevels();
    end = Sys_Milliseconds();

    if ( sg_debugPrint.i ) {
        SG_Printf( "Loaded level configurations in %i msec.\n", end - start );
    }
}

void SG_Init( void )
{
    G_Printf( "---------- Game Initialization ----------\n" );
    G_Printf( "gamename: %s\n", GLN_VERSION );
    G_Printf( "gamedate: %s\n", __DATE__ );

    trap_Key_SetCatcher( trap_Key_GetCatcher() | KEYCATCH_SGAME );

    // clear sgame state
    memset( &sg, 0, sizeof(sg) );
    
    // cache redundant calculations
    Sys_GetGPUConfig( &sg.gpuConfig );

    // for 1024x768 virtualized screen
	sg.scale = sg.gpuConfig.vidHeight * (1.0/768.0);
	if ( sg.gpuConfig.vidWidth * 768 > sg.gpuConfig.vidHeight * 1024 ) {
		// wide screen
		sg.bias = 0.5 * ( sg.gpuConfig.vidWidth - ( sg.gpuConfig.vidHeight * (1024.0/768.0) ) );
	}
	else {
		// no wide screen
		sg.bias = 0;
	}

    // register sgame cvars
    SG_RegisterCvars();

    SG_MemInit();

    // load assets/resources
    SG_LoadMedia();

    // register commands
    SG_InitCommands();

    sg.state = SG_INACTIVE;

    G_Printf( "-----------------------------------\n" );
}

void SG_Shutdown( void )
{
    G_Printf( "Shutting down sgame...\n" );

    SG_ShutdownCommands();
    SaveGame();

    memset( &sg, 0, sizeof(sg) );

    sg.state = SG_INACTIVE;
}

void SaveGame( void )
{
    G_Printf( "Saving game...\n" );

    SG_SaveLevelData();

    G_Printf( "Done" );
}

void LoadGame( void )
{
    char savename[MAX_NPATH];

    Cvar_VariableStringBuffer( "sg_savename", savename, sizeof(savename) );

    G_Printf( "Loading save file '%s'...\n", savename );

    SG_LoadLevelData();
}

static void SG_DrawSettings( void )
{
    ImGui_BeginTable( "##SGameSettings", 2 );
    
    ImGui_TableNextColumn();
    ImGui_TextUnformatted( "Maximum GFX" );
    ImGui_TableNextColumn();
    ImGui_SliderInt( "##MaximumGraphicsEffects", &settings.maxGfx, 0, MAX_GFX );

    ImGui_TableNextRow();

    if ( sg.state == SG_IN_LEVEL ) {
        ImGui_TableNextColumn();
        ImGui_TextUnformatted( "Difficulty" );
        ImGui_TableNextColumn();
        ImGui_ArrowButton( "##DifficultySelectorLeft", ImGuiDir_Left );
        ImGui_SameLine();
        ImGui_Text( "%s",  );
        ImGui_SameLine();
        ImGui_ArrowButton( "##DifficultySelectorRight", ImGuiDir_Right );
    }

    ImGui_EndTable();
}

void GDR_ATTRIBUTE((format(printf, 2, 3))) GDR_DECL trap_FS_Printf( fileHandle_t f, const char *fmt, ... )
{
    va_list argptr;
    char msg[MAXPRINTMSG];

    va_start( argptr, fmt );
    vsprintf( msg, fmt, argptr );
    va_end( argptr );

    trap_FS_Write( msg, strlen( msg ), f );
}


const vec3_t dirvectors[NUMDIRS] = {
    { -1.0f, -1.0f, 0.0f },
    {  0.0f, -1.0f, 0.0f },
    {  1.0f, -1.0f, 0.0f },
    {  1.0f,  0.0f, 0.0f },
    {  1.0f,  1.0f, 0.0f },
    {  0.0f,  1.0f, 0.0f },
    { -1.0f,  1.0f, 0.0f },
    { -1.0f,  0.0f, 0.0f }
};

const dirtype_t inversedirs[NUMDIRS] = {
    DIR_SOUTH_EAST,
    DIR_SOUTH,
    DIR_SOUTH_WEST,
    DIR_WEST,
    DIR_NORTH_WEST,
    DIR_NORTH,
    DIR_NORTH_EAST,
    DIR_EAST
};
