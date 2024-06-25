// module_main.cpp -- initializes the module library

#include "module_public.h"
#include "angelscript/angelscript.h"
#include "module_handle.h"
#include "../game/g_game.h"
#include <glm/glm.hpp>
#include <filesystem>
#include "scriptlib/scriptjson.h"
#include "module_renderlib.h"
#include "module_funcdefs.hpp"
#include "module_stringfactory.hpp"
#include "module_debugger.h"
#include "scriptlib/scriptarray.h"
#include "scriptlib/scriptdictionary.h"
#include "scriptlib/scriptany.h"
#define JEMALLOC_NO_RENAME
#include <jemalloc/jemalloc.h>

moduleImport_t moduleImport;

static CModuleLib *s_pModuleInstance;

cvar_t *ml_angelScript_DebugPrint;
cvar_t *ml_debugMode;
cvar_t *ml_alwaysCompile;
cvar_t *ml_allowJIT;
cvar_t *ml_garbageCollectionIterations;

static void ML_CleanCache_f( void ) {
    const char *path;
    uint64_t i;
    const CModuleInfo *it;

    Con_Printf( "Clearing cached bytecode script files...\n" );
    for ( i = 0; i < g_pModuleLib->GetModCount(); i++ ) {
        it = &g_pModuleLib->GetLoadList()[ i ];

        path = va( CACHE_DIR "/%s_code.dat", it->m_pHandle->GetName().c_str() );
        FS_Remove( path );
        FS_HomeRemove( path );

        path = va( CACHE_DIR "/%s_metadata.bin", it->m_pHandle->GetName().c_str() );
        FS_Remove( path );
        FS_HomeRemove( path );
    }
}

static void ML_GarbageCollectionStats_f( void ) {
    asUINT currentSize, totalDestroyed, totalDetected, newObjects, totalNewDestroyed;
    g_pModuleLib->GetScriptEngine()->GetGCStatistics( &currentSize, &totalDestroyed, &totalDetected, &newObjects, &totalNewDestroyed );

    Con_Printf( "--------------------\n" );
    Con_Printf( "[Garbage Collection Stats]\n" );
    Con_Printf( "Current Size: %u\n", currentSize );
    Con_Printf( "Total Destroyed: %u\n", totalDestroyed );
    Con_Printf( "Total Detected: %u\n", totalDetected );
    Con_Printf( "New Objects: %u\n", newObjects );
    Con_Printf( "Total New Destroyed: %u\n", totalNewDestroyed );
    Con_Printf( "--------------------\n" );
}

const char *AS_PrintErrorString( int code )
{
    switch ( code ) {
    case asERROR:
        return "error occurred";
    case asNOT_SUPPORTED:
        return "not supported";
    case asINVALID_ARG:
        return "invalid argument";
    case asINVALID_CONFIGURATION:
        return "invalid configuration";
    case asINVALID_DECLARATION:
        return "invalid declaration";
    case asINVALID_INTERFACE:
        return "invalid interface";
    case asINVALID_NAME:
        return "invalid name";
    case asINVALID_OBJECT:
        return "invalid object";
    case asINVALID_TYPE:
        return "invalid type";
    case asNO_FUNCTION:
        return "function wasn't found";
    case asNO_GLOBAL_VAR:
        return "no global variable was found";
    case asNAME_TAKEN:
        return "name already taken";
    case asMULTIPLE_FUNCTIONS:
        return "multiple matching functions";
    case asCANT_BIND_ALL_FUNCTIONS:
        return "could not bind all imported module functions";
    case asCONTEXT_NOT_FINISHED:
        return "context not finished";
    case asCONTEXT_NOT_PREPARED:
        return "context not prepared";
    case asOUT_OF_MEMORY:
        return "out of memory... SOMEHOW?";
    case asILLEGAL_BEHAVIOUR_FOR_TYPE:
        return "illegal behaviour for type";
    case asWRONG_CALLING_CONV:
        return "specified calling convention doesn't match the function/method pointer";
    case asALREADY_REGISTERED:
        return "already registered";
    case asINIT_GLOBAL_VARS_FAILED:
        return "initialization of global vars failed";
    case asNO_MODULE:
        return "no module was found";
    case asWRONG_CONFIG_GROUP:
        return "already registered in another config group";
    case asLOWER_ARRAY_DIMENSION_NOT_REGISTERED:
        return "array templated subtype not registered yet";
    case asMODULE_IS_IN_USE:
        return "module is referred to by active object or from the engine";
    default:
        break;
    };
    return va( "<unknown error> (%i)", code );
}

namespace EA::StdC {

extern "C" EASTL_EASTDC_API int Vsnprintf(char*  EA_RESTRICT pDestination, size_t n, const char*  EA_RESTRICT pFormat, va_list arguments) {
    return N_vsnprintf( pDestination, n, pFormat, arguments );
}

EASTL_EASTDC_API int Vsnprintf(char16_t* EA_RESTRICT pDestination, size_t n, const char16_t* EA_RESTRICT pFormat, va_list arguments) {
    return 0;
}

// who the fuck even uses a char32?
EASTL_EASTDC_API int Vsnprintf(char32_t* EA_RESTRICT pDestination, size_t n, const char32_t* EA_RESTRICT pFormat, va_list arguments) {
    return 0;
}

#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
	EASTL_EASTDC_API int Vsnprintf(wchar_t* EA_RESTRICT pDestination, size_t n, const wchar_t* EA_RESTRICT pFormat, va_list arguments) {
        return 0; // TODO
    }
#endif
};

void CModuleLib::LoadModule( const char *pModule )
{
    PROFILE_FUNCTION();

    CModuleHandle *pHandle;
    nlohmann::json parse;
    nlohmann::json includePaths;
    CModuleInfo *m;
    string_t description;
    union {
        void *v;
        char *b;
    } f;

    try {
        uint64_t length = FS_LoadFile( va( "modules/%s/module.json", pModule ), &f.v );
        if ( !length || !f.v ) {
            Con_Printf( COLOR_RED "ERROR: failed to load module configuration for \"%s\"!\n", pModule );
            return;
        } else {
            parse = nlohmann::json::parse( f.b );
        }
        FS_FreeFile( f.v );
        
    } catch ( const nlohmann::json::exception& e ) {
        Con_Printf( COLOR_RED "ERROR: failed to load module configuration for \"%s\"!\n\tid: %i\n\tmessage: %s\n", pModule, e.id, e.what() );
        FS_FreeFile( f.v );
        return;
    }

    //
    // validate its an actual config
    //
    if ( !parse.contains( "SubModules" ) ) {
        Con_Printf( COLOR_RED "ERROR: invalid module configuration for \"%s\", no array 'SubModules'\n", pModule );
        return;
    }
    if ( !parse.contains( "DependedModules" ) ) {
        Con_Printf( COLOR_RED "ERROR: invalid module configuration for \"%s\", no array 'DependedModules'\n", pModule );
        return;
    }
    if ( parse.contains( "Version" ) ) {
        if ( !parse.at( "Version" ).contains( "GameVersionMajor" ) ) {
            Con_Printf( COLOR_RED "ERROR: invalid module configuration for \"%s\", no value for 'GameVersionMajor'\n", pModule );
            return;
        }
        if ( !parse.at( "Version" ).contains( "GameVersionUpdate" ) ) {
            Con_Printf( COLOR_RED "ERROR: invalid module configuration for \"%s\", no value for 'GameVersionUpdate'\n", pModule );
            return;
        }
        if ( !parse.at( "Version" ).contains( "GameVersionPatch" ) ) {
            Con_Printf( COLOR_RED "ERROR: invalid module configuration for \"%s\", no value for 'GameVersionPatch'\n", pModule );
            return;
        }
        if ( !parse.at( "Version" ).contains( "VersionMajor" ) ) {
            Con_Printf( COLOR_RED "ERROR: invalid module configuration for \"%s\", no value for 'VersionMajor'\n", pModule );
            return;
        }
        if ( !parse.at( "Version" ).contains( "VersionUpdate" ) ) {
            Con_Printf( COLOR_RED "ERROR: invalid module configuration for \"%s\", no value for 'VersionUpdate'\n", pModule );
            return;
        }
        if ( !parse.at( "Version" ).contains( "VersionPatch" ) ) {
            Con_Printf( COLOR_RED "ERROR: invalid module configuration for \"%s\", no value for 'VersionPatch'\n", pModule );
            return;
        }
    } else {
        Con_Printf( COLOR_RED "ERROR: invalid module configuration for \"%s\", no object 'version'\n", pModule );
        return;
    }

    if ( parse.contains( "IncludePaths" ) ) {
        includePaths = eastl::move( parse.at( "IncludePaths" ) );

        //
        // check if it already has the default path
        //
        bool found = false;
        const char *path = va( "%s/", pModule );
        for ( const auto& it : includePaths ) {
            if ( !N_strcmp( it.get<string_t>().c_str(), path ) ) {
                found = true;
                break;
            }
        }
        if ( !found ) {
            includePaths.emplace_back( path );
        }
    } else {
        includePaths.emplace_back( va( "%s/", pModule ) );
    }
    description = parse.contains( "Description" ) ? parse[ "Description" ].get<string_t>() : "";
    const int32_t versionMajor = parse[ "Version" ][ "VersionMajor" ];
    const int32_t versionUpdate = parse[ "Version" ][ "VersionUpdate" ];
    const int32_t versionPatch = parse[ "Version" ][ "VersionPatch" ];

    pHandle = new ( Hunk_Alloc( sizeof( *pHandle ), h_high ) ) CModuleHandle( pModule, description.c_str(), parse.at( "SubModules" ),
        versionMajor, versionUpdate, versionPatch, includePaths );
    m = new ( &m_pLoadList[ m_nModuleCount ] ) CModuleInfo( parse, pHandle );
    m_nModuleCount++;
}

void CModuleLib::RunModules( EModuleFuncId nCallId, uint32_t nArgs, ... )
{
    va_list argptr;
    uint64_t j;
    uint32_t i;
    int *args;
    CTimer time;

    if ( nCallId >= NumFuncs ) {
        N_Error( ERR_FATAL, "CModuleLib::RunModules: invalid call id" );
    }
    
    args = (int *)alloca( sizeof( *args ) * nArgs );
    va_start( argptr, nArgs );
    for ( i = 0; i < nArgs; i++ ) {
        args[i] = va_arg( argptr, int );
    }
    va_end( argptr );

    time.Start();
    AS_Printf( "Garbage collection started...\n" );
    g_pModuleLib->GetScriptEngine()->GarbageCollect( asGC_DETECT_GARBAGE | asGC_DESTROY_GARBAGE
        | asGC_FULL_CYCLE, (uint32_t)ml_garbageCollectionIterations->i );
    
    time.Stop();
    AS_Printf( "Garbage collection: %lims, %li iterations\n", time.Milliseconds(), ml_garbageCollectionIterations->i );

    for ( j = 0; j < m_nModuleCount; j++ ) {
        if ( sgvm == &m_pLoadList[j] ) {
            continue; // avoid running it twice
        }
        if ( m_pLoadList[j].m_pHandle->CallFunc( nCallId, nArgs, args ) == -1 ) {
            Con_Printf( COLOR_YELLOW "WARNING: module \"%s\" returned error code on call to proc \"%s\".\n",
                m_pLoadList[i].m_szName, funcDefs[ nCallId ].name );
        }
    }
}

int CModuleLib::ModuleCall( CModuleInfo *pModule, EModuleFuncId nCallId, uint32_t nArgs, ... )
{
    va_list argptr;
    uint32_t i;
    int *args;
    const char *name;
    CTimer time;

    if ( !pModule ) {
        N_Error( ERR_FATAL, "CModuleLib::ModuleCall: invalid module" );
    }
    if ( nCallId >= NumFuncs ) {
        N_Error( ERR_FATAL, "CModuleLib::ModuleCall: invalid call id" );
    }
    
    args = (int *)alloca( sizeof( *args ) * nArgs );
    va_start( argptr, nArgs );
    for ( i = 0; i < nArgs; i++ ) {
        args[i] = va_arg( argptr, int );
    }
    va_end( argptr );

    name = funcDefs[ nCallId ].name;

    time.Start();
    AS_Printf( "Garbage collection started (%s)...\n", name );
    g_pModuleLib->GetScriptEngine()->GarbageCollect( asGC_DETECT_GARBAGE | asGC_DESTROY_GARBAGE
        | asGC_FULL_CYCLE, (uint32_t)ml_garbageCollectionIterations->i );
    time.Stop();
    AS_Printf( "Garbage collection: %lims, %li iterations\n", time.Milliseconds(), ml_garbageCollectionIterations->i );

    return pModule->m_pHandle->CallFunc( nCallId, nArgs, args );
}

void Module_ASMessage_f( const asSMessageInfo *pMsg, void *param )
{
    bool error;

    error = false;

    switch ( pMsg->type ) {
    case asMSGTYPE_ERROR:
        Con_Printf( COLOR_RED "ERROR: [AngelScript](%s:%i:%i) %s\n",
            pMsg->section, pMsg->row, pMsg->col, pMsg->message );
        error = true;
        break;
    case asMSGTYPE_WARNING:
        Con_Printf( COLOR_YELLOW "WARNING: [AngelScript](%s:%i:%i) %s\n", pMsg->section, pMsg->row, pMsg->col, pMsg->message );
        break;
    case asMSGTYPE_INFORMATION: {
        if ( !ml_angelScript_DebugPrint->i ) {
            return;
        }
        Con_Printf( "[AngelScript](%s:%i:%i) %s\n", pMsg->section, pMsg->row, pMsg->col, pMsg->message );
        break; }
    };

    if ( error ) {
        //Sys_MessageBox( "*** ERROR DURING MODULE COMPILATION ***",
        //    va( "An error occurred during the compilation of a module section\n"
        //        "%s:%i:%i %s"
        //    , pMsg->section, pMsg->row, pMsg->col, pMsg->message ),
        //    false );
    }
}

#ifdef _NOMAD_DEBUG
void *AS_Alloc( size_t nSize, const char *pFilename, uint32_t lineNumber ) {
//    return Z_Malloc( nSize, TAG_MODULES );
    return Mem_Alloc( nSize );
}
#else
void *AS_Alloc( size_t nSize ) {
//    return Z_Malloc( nSize, TAG_MODULES );
    return Mem_Alloc( nSize );
}
#endif

#ifdef _NOMAD_DEBUG
void AS_Free( void *ptr, const char *pFilename, uint32_t lineNumber ) {
//    Z_Free( ptr );
    Mem_Free( ptr );
}
#else
void AS_Free( void *ptr ) {
//    Z_Free( ptr );
    Mem_Free( ptr );
}
#endif

/*
* AS_Printf: a debugging tool used for whenever the angelscript compiler decides
* to break on me
*/
void GDR_ATTRIBUTE((format(printf, 1, 2))) AS_Printf( const char *fmt, ... )
{
    va_list argptr;
    char msg[MAXPRINTMSG];

    if ( !ml_angelScript_DebugPrint->i ) {
        return;
    }

    va_start( argptr, fmt );
    N_vsnprintf( msg, sizeof( msg ) - 1, fmt, argptr );
    va_end( argptr );

    Con_Printf( COLOR_GREEN "[AngelScript DEBUG]: %s", msg );
}

int Module_IncludeCallback_f( const char *pInclude, const char *pFrom, CScriptBuilder *pBuilder, void *unused )
{
    union {
        void *v;
        char *b;
    } f;
    uint64_t length;
    const char *path;
    const nlohmann::json& includePaths = g_pModuleLib->GetCurrentHandle()->GetIncludePaths();

    for ( const auto& it : includePaths ) {
        path = va( "%s%s", it.get<string_t>().c_str(), pInclude );
        length = FS_LoadFile( path, &f.v );
        if ( !f.v ) {
            continue;
        }

        pBuilder->AddSectionFromMemory( COM_SkipPath( const_cast<char *>( pInclude ) ), f.b, length );
        FS_FreeFile( f.v );

        g_pModuleLib->GetScriptEngine()->WriteMessage( g_pModuleLib->GetCurrentHandle()->GetModulePath(),
            0, 0, asMSGTYPE_INFORMATION, va( "Added include file \"%s\" to \"%s\"\n", path, pFrom ) );
        return 1;
    }

    (void)unused; // shut up compiler
    g_pModuleLib->GetScriptEngine()->WriteMessage(
        g_pModuleLib->GetCurrentHandle()->GetModulePath(),
        0, 0, asMSGTYPE_WARNING, va( "failed to load include preprocessor file \"%s\" from \"%s\"", pInclude, pFrom ) );
    return -1;
}

bool CModuleLib::AddDefaultProcs( void ) const {
    if ( m_bRegistered ) {
        return true;
    }

    RegisterScriptArray( m_pEngine );
    RegisterStdString_Generic( g_pModuleLib->GetScriptEngine() );
    RegisterScriptHandle( m_pEngine );
    RegisterScriptAny( m_pEngine );

    CheckASCall( g_pModuleLib->GetScriptEngine()->SetDefaultNamespace( "TheNomad::GameSystem" ) );
    CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterface( "GameObject" ) );
    CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterfaceMethod( "GameObject",
        "void OnInit()" ) );
    CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterfaceMethod( "GameObject",
        "void OnShutdown()" ) );
    CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterfaceMethod( "GameObject",
        "void OnLevelStart()" ) );
    CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterfaceMethod( "GameObject",
        "void OnLevelEnd()" ) );
    CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterfaceMethod( "GameObject",
        "void OnRunTic()" ) );
//  CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterfaceMethod( "GameObject",
//      "void OnKeyEvent( uint key, uint down )" ) );
//  CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterfaceMethod( "GameObject",
//      "void OnMouseEvent( uint dx, uint dy )" ) );
    CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterfaceMethod( "GameObject",
        "void OnRenderScene()" ) );
    CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterfaceMethod( "GameObject",
        "bool OnConsoleCommand( const string& in )" ) );
    CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterfaceMethod( "GameObject",
        "void OnLoad()" ) );
    CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterfaceMethod( "GameObject",
        "void OnSave() const" ) );
    CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterInterfaceMethod( "GameObject",
        "const string& GetName() const" ) );
    CheckASCall( g_pModuleLib->GetScriptEngine()->SetDefaultNamespace( "" ) );

    RegisterScriptDictionary( m_pEngine );
    RegisterScriptMath( m_pEngine );
    RegisterScriptJson( m_pEngine );

    ModuleLib_Register_Engine();
    const_cast<CModuleLib *>( this )->m_bRegistered = qtrue;

    return true;
}

CModuleLib::CModuleLib( void )
{
    const char *path;
    char **fileList;
    uint64_t nFiles, i;

    if ( s_pModuleInstance && s_pModuleInstance->m_pEngine ) {
        return;
    }

    g_pModuleLib = this;

    memset( this, 0, sizeof( *this ) );

    //
    // init angelscript api
    //
    m_pEngine = dynamic_cast<asCScriptEngine *>( asCreateScriptEngine() );
    if ( !m_pEngine ) {
        N_Error( ERR_DROP, "CModuleLib::Init: failed to create an AngelScript Engine context" );
    }
    CheckASCall( m_pEngine->SetMessageCallback( asFUNCTION( Module_ASMessage_f ), NULL, asCALL_CDECL ) );
    CheckASCall( m_pEngine->SetEngineProperty( asEP_ALLOW_MULTILINE_STRINGS, true ) );
    CheckASCall( m_pEngine->SetEngineProperty( asEP_ALLOW_UNSAFE_REFERENCES, false ) );
    CheckASCall( m_pEngine->SetEngineProperty( asEP_ALWAYS_IMPL_DEFAULT_CONSTRUCT, false ) );
    CheckASCall( m_pEngine->SetEngineProperty( asEP_COMPILER_WARNINGS, true ) );
    CheckASCall( m_pEngine->SetEngineProperty( asEP_OPTIMIZE_BYTECODE, true ) );
    CheckASCall( m_pEngine->SetEngineProperty( asEP_INCLUDE_JIT_INSTRUCTIONS, ml_allowJIT->i ) );
    CheckASCall( m_pEngine->SetEngineProperty( asEP_REQUIRE_ENUM_SCOPE, true ) );
    CheckASCall( m_pEngine->SetEngineProperty( asEP_USE_CHARACTER_LITERALS, true ) );
    CheckASCall( m_pEngine->SetEngineProperty( asEP_AUTO_GARBAGE_COLLECT, true ) );
    CheckASCall( m_pEngine->SetEngineProperty( asEP_HEREDOC_TRIM_MODE, 0 ) );
    CheckASCall( m_pEngine->SetEngineProperty( asEP_INIT_GLOBAL_VARS_AFTER_BUILD, true ) );

    m_pScriptBuilder = new ( Hunk_Alloc( sizeof( *m_pScriptBuilder ), h_low ) ) CScriptBuilder();
    g_pDebugger = new ( Hunk_Alloc( sizeof( *g_pDebugger ), h_low ) ) CDebugger();

    if ( ml_allowJIT->i ) {
        m_pCompiler = new ( Hunk_Alloc( sizeof( *m_pCompiler ), h_low ) ) asCJITCompiler();
        CheckASCall( m_pEngine->SetJITCompiler( m_pCompiler ) );
    }
    m_pScriptBuilder->SetIncludeCallback( Module_IncludeCallback_f, NULL );

    //
    // load all the modules
    //

    Con_Printf( "Loading module configurations from \"modules/\"...\n" );

    path = "modules/";
    fileList = FS_ListFiles( "modules/", "/", &nFiles );
    m_pLoadList = (CModuleInfo *)Hunk_Alloc( sizeof( *m_pLoadList ) * nFiles, h_high );
    
    for ( i = 0; i < nFiles; i++ ) {
        if ( N_streq( fileList[i], "." ) || N_streq( fileList[i], ".." ) ) {
            continue;
        }
        Con_Printf( "...found module directory \"%s\".\n", fileList[i] );
        LoadModule( fileList[i] );
    }

    FS_FreeFileList( fileList );

/*
    try {
        // this is really inefficient but it'll do for now
        for ( const auto& it : std::filesystem::directory_iterator{ path } ) {
            if ( it.is_directory() ) {
                const std::string path = eastl::move( it.path().filename().string() );
                Con_Printf( "...found module directory \"%s\".\n", path.c_str() );

                LoadModule( path.c_str() );
            }
        }
    } catch ( const std::exception& e ) {
        N_Error( ERR_FATAL, "InitModuleLib: failed to load module directories, std::exception was thrown -> %s", e.what() );
    }
*/

    // bind all the functions
    for ( i = 0; i < m_nModuleCount; i++ ) {
        if ( !m_pLoadList[i].m_pHandle->GetModule() ) { // failed to compile or didn't have any source files
            continue;
        }

        // this might fail, yes but, it'll just crash automatically if we're missing a function, and not
        // all functions have to be imported
        m_pLoadList[i].m_pHandle->GetModule()->BindAllImportedFunctions();
    }

    #undef CALL
}

CModuleLib::~CModuleLib() {
}

static void ML_PrintStringCache_f( void ) {
    uint64_t i;

    Con_Printf( "Module StringFactory Cache:\n" );

    auto it = g_pStringFactory->m_StringCache.cbegin();
    for ( i = 0; i < g_pStringFactory->m_StringCache.size(); i++, it++ ) {
        Con_Printf( "%8lu: 0x%08lx (%i references)", i, (uintptr_t)(void *)it->first.c_str(), it->second );
    }
}

CModuleLib *InitModuleLib( const moduleImport_t *pImport, const renderExport_t *pExport, version_t nGameVersion )
{
    PROFILE_FUNCTION();

    Con_Printf( "---------- InitModuleLib ----------\n" );
    Con_Printf( "Initializing mod library...\n" );

    //
    // init cvars
    //
    ml_angelScript_DebugPrint = Cvar_Get( "ml_angelScript_DebugPrint", "1", CVAR_LATCH | CVAR_PRIVATE | CVAR_SAVE );
    Cvar_SetDescription( ml_angelScript_DebugPrint, "Set to 1 if you want verbose AngelScript API logging" );
    ml_alwaysCompile = Cvar_Get( "ml_alwaysCompile", "0", CVAR_LATCH | CVAR_PRIVATE | CVAR_SAVE );
    Cvar_SetDescription( ml_alwaysCompile, "Toggle forced compilation of a module every time the game loads as opposed to using cached bytecode" );
    ml_allowJIT = Cvar_Get( "ml_allowJIT", "0", CVAR_LATCH | CVAR_PRIVATE | CVAR_SAVE );
    Cvar_SetDescription( ml_allowJIT, "Toggle JIT compilation of a module" );
    ml_debugMode = Cvar_Get( "ml_debugMode", "0", CVAR_LATCH | CVAR_TEMP );
    Cvar_SetDescription( ml_debugMode, "Set to 1 whenever a module is being debugged" );
    ml_garbageCollectionIterations = Cvar_Get( "ml_garbageCollectionIterations", "4", CVAR_TEMP | CVAR_PRIVATE );
    Cvar_SetDescription( ml_garbageCollectionIterations, "Sets the number of iterations per garbage collection loop" );

    Cmd_AddCommand( "ml.clean_script_cache", ML_CleanCache_f );
    Cmd_AddCommand( "ml.garbage_collection_stats", ML_GarbageCollectionStats_f );
    Cmd_AddCommand( "ml_debug.print_string_cache", ML_PrintStringCache_f );

    Mem_Init();

    // FIXME: angelscript's thread manager is fucking broken on unix (stalls forever)
    asSetGlobalMemoryFunctions( AS_Alloc, AS_Free );

    g_pModuleLib = new ( Hunk_Alloc( sizeof( *g_pModuleLib ), h_low ) ) CModuleLib();

    Con_Printf( "--------------------\n" );

    return g_pModuleLib;
}

void CModuleLib::Shutdown( qboolean quit )
{
    uint64_t i, j;
    memoryStats_t allocs, frees;

    if ( m_bRecursiveShutdown ) {
        Con_Printf( COLOR_YELLOW "WARNING: CModuleLib::Shutdown (recursive)\n" );
        return;
    }
    m_bRecursiveShutdown = qtrue;

    Con_Printf( "CModuleLib::Shutdown: shutting down modules...\n" );

    if ( com_errorEntered ) {
        if ( asGetActiveContext() ) {
            Cbuf_ExecuteText( EXEC_NOW, va( "ml_debug.set_active \"%s\"", m_pCurrentHandle->GetName().c_str() ) );
            g_pDebugger->PrintCallstack( asGetActiveContext() );
        }

        // TODO: use the serializer to create a sort of coredump like file for the active script
    }

    Cmd_RemoveCommand( "ml.clean_script_cache" );
    Cmd_RemoveCommand( "ml.garbage_collection_stats" );
    Cmd_RemoveCommand( "ml_debug.set_active" );
	Cmd_RemoveCommand( "ml_debug.print_help" );
	Cmd_RemoveCommand( "ml_debug.stacktrace" );
	Cmd_RemoveCommand( "ml_debug.backtrace" );
	Cmd_RemoveCommand( "ml_debug.clear_breakpoint" );
	Cmd_RemoveCommand( "ml_debug.set_breakpoint" );
	Cmd_RemoveCommand( "ml_debug.continue" );
	Cmd_RemoveCommand( "ml_debug.step_into" );
	Cmd_RemoveCommand( "ml_debug.step_out" );
	Cmd_RemoveCommand( "ml_debug.step_over" );
    Cmd_RemoveCommand( "ml_debug.print_array_memory_stats" );
    Cmd_RemoveCommand( "ml_debug.print_string_cache" );

    m_pEngine->GarbageCollect( asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE, 100 );
    for ( i = 0; i < m_nModuleCount; i++ ) {
        if ( m_pLoadList[i].m_pHandle ) {
            m_pLoadList[i].m_pHandle->CallFunc( ModuleShutdown, 0, NULL );
            m_pLoadList[i].m_pHandle->ClearMemory();
            m_pLoadList[i].m_pHandle->~CModuleHandle();
        }
    }
    
    if ( m_bRegistered ) {
        if ( m_pCompiler ) {
            m_pCompiler->~asCJITCompiler();
        }
        m_pScriptBuilder->~CScriptBuilder();
        g_pDebugger->~CDebugger();
    }

    asSetThreadManager( NULL );

    Mem_GetFrameStats( allocs, frees );
    Con_Printf( "\n" );
    Con_Printf( "Allocated Bytes: %li\n", allocs.totalSize );
    Con_Printf( "Total Allocations: %li\n", allocs.num );
    Con_Printf( "Biggest Allocation: %li\n", allocs.maxSize );
    Con_Printf( "Smallest Allocation: %li\n", allocs.minSize );
    Con_Printf( "Released Bytes: %li\n", frees.totalSize );
    Con_Printf( "Total Frees: %li\n", frees.num );
    Con_Printf( "Biggest Free: %li\n", frees.maxSize );
    Con_Printf( "Smallest Free: %li\n", frees.minSize );

    m_bRegistered = qfalse;
    m_bRecursiveShutdown = qfalse;
    g_pModuleLib = NULL;
    g_pDebugger = NULL;

    Z_FreeTags( TAG_MODULES );
    Mem_Shutdown();
}

asIScriptEngine *CModuleLib::GetScriptEngine( void ) {
    return m_pEngine;
}

CScriptBuilder *CModuleLib::GetScriptBuilder( void ) {
    return m_pScriptBuilder;
}

CContextMgr *CModuleLib::GetContextManager( void ) {
    return m_pContextManager;
}

CModuleInfo *CModuleLib::GetModule( const char *pName ) {
    PROFILE_FUNCTION();
    
    uint64_t i;

    for ( i = 0; i < m_nModuleCount; i++ ) {
        if ( !N_stricmp( m_pLoadList[i].m_szName, pName ) ) {
            return &m_pLoadList[i];
        }
    }
    return NULL;
}

CModuleInfo *CModuleLib::GetLoadList( void ) {
    return m_pLoadList;
}

uint64_t CModuleLib::GetModCount( void ) const {
    return m_nModuleCount;
}
