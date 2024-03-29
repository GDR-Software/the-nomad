#include "module_public.h"
#include <glm/gtc/type_ptr.hpp>
#include <limits.h>
#include "module_alloc.h"
#include "aswrappedcall.h"
#include "imgui_stdlib.h"
#include "../ui/ui_lib.h"
#include "module_stringfactory.hpp"
#include "../game/g_world.h"
#include "module_funcdefs.hpp"

#include "module_engine/module_bbox.h"
#include "module_engine/module_linkentity.h"
#include "module_engine/module_polyvert.h"
#include "module_engine/module_raycast.h"
#include "../system/sys_timer.h"
#include "scriptjson.h"
#include "module_engine/module_gpuconfig.h"

//
// c++ compatible wrappers around angelscript engine function calls
//

// glm has a lot of very fuzzy template types
using vec2 = glm::vec<2, float, glm::packed_highp>;
using vec3 = glm::vec<3, float, glm::packed_highp>;
using vec4 = glm::vec<4, float, glm::packed_highp>;
using ivec2 = glm::vec<2, int, glm::packed_highp>;
using ivec3 = glm::vec<3, int, glm::packed_highp>;
using ivec4 = glm::vec<4, int, glm::packed_highp>;
using uvec2 = glm::vec<2, unsigned, glm::packed_highp>;
using uvec3 = glm::vec<3, unsigned, glm::packed_highp>;
using uvec4 = glm::vec<4, unsigned, glm::packed_highp>;

#define REQUIRE_ARG_COUNT( amount ) \
    Assert( pGeneric->GetArgCount() == amount )
#define DEFINE_CALLBACK( name ) \
    static void ModuleLib_##name ( asIScriptGeneric *pGeneric )
#define GETVAR( type, index ) \
    *(const type *)pGeneric->GetArgAddress( index )

#define REGISTER_GLOBAL_VAR( name, addr ) \
    ValidateGlobalVar( __func__, name, g_pModuleLib->GetScriptEngine()->RegisterGlobalProperty( name, (void *)addr ) )
#define REGISTER_ENUM_TYPE( name ) \
    ValidateEnumType( __func__, name, g_pModuleLib->GetScriptEngine()->RegisterEnum( name ) )
#define REGISTER_ENUM_VALUE( type, name, value ) \
    ValidateEnumValue( __func__, name, g_pModuleLib->GetScriptEngine()->RegisterEnumValue( type, name, value ) )
#define REGISTER_METHOD_FUNCTION( obj, decl, classType, name, parameters, returnType ) \
    ValidateMethod( __func__, decl, g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( obj, decl, WRAP_MFN_PR( classType, name, parameters, returnType ), asCALL_GENERIC ) )
#define REGISTER_GLOBAL_FUNCTION( decl, funcPtr ) \
    ValidateFunction( __func__, decl,\
        g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction( decl, asFUNCTION( ModuleLib_##funcPtr ), asCALL_GENERIC ) )
#define REGISTER_OBJECT_TYPE( name, obj, traits ) \
    ValidateObjectType( __func__, name, g_pModuleLib->GetScriptEngine()->RegisterObjectType( name, sizeof(obj), traits | asGetTypeTraits<obj>() ) )
#define REGISTER_OBJECT_PROPERTY( obj, var, offset ) \
    ValidateObjectProperty( __func__, var, g_pModuleLib->GetScriptEngine()->RegisterObjectProperty( obj, var, offset ) )
#define REGISTER_OBJECT_BEHAVIOUR( obj, type, decl, funcPtr ) \
    ValidateObjectBehaviour( __func__, #type, g_pModuleLib->GetScriptEngine()->RegisterObjectBehaviour( obj, type, decl, funcPtr, asCALL_GENERIC ) )
#define REGISTER_TYPEDEF( type, alias ) \
    ValidateTypedef( __func__, alias, g_pModuleLib->GetScriptEngine()->RegisterTypedef( alias, type ) )

#define SET_NAMESPACE( name ) g_pModuleLib->GetScriptEngine()->SetDefaultNamespace( name )
#define RESET_NAMESPACE() g_pModuleLib->GetScriptEngine()->SetDefaultNamespace( "" )

static bool Add( const char *name ) {
    for ( const auto& it : g_pModuleLib->m_RegisteredProcs ) {
        if ( !N_strcmp( name, it.c_str() ) ) {
            return false;
        }
    }
    g_pModuleLib->m_RegisteredProcs.emplace_back( name );
    return true;
}

static void ValidateEnumType( const char *func, const char *name, int result ) {
    if ( !Add( name ) ) {
        return;
    }
    if ( result < 0 ) {
        N_Error( ERR_DROP, "%s: failed to register enumeration type %s -- %s", func, name, AS_PrintErrorString( result ) );
    }
}

static void ValidateEnumValue( const char *func, const char *name, int result ) {
    if ( !Add( name ) ) {
        return;
    }
    if ( result < 0 ) {
        N_Error( ERR_DROP, "%s: failed to register enumeration value %s -- %s", func, name, AS_PrintErrorString( result ) );
    }
}

static void ValidateFunction( const char *func, const char *name, int result ) {
    if ( !Add( name ) ) {
        return;
    }
    if ( result < 0 ) {
        N_Error( ERR_DROP, "%s: failed to register global function %s -- %s", func, name, AS_PrintErrorString( result ) );
    }
}

static void ValidateMethod( const char *func, const char *name, int result ) {
    if ( !Add( name ) ) {
        return;
    }
    if ( result < 0 ) {
        N_Error( ERR_DROP, "%s: failed to register method function %s -- %s", func, name, AS_PrintErrorString( result ) );
    }
}

static void ValidateObjectType( const char *func, const char *name, int result ) {
    if ( !Add( name ) ) {
        return;
    }
    if ( result < 0 ) {
        N_Error( ERR_DROP, "%s: failed to register object type %s -- %s", func, name, AS_PrintErrorString( result ) );
    }
}

static void ValidateObjectBehaviour( const char *func, const char *name, int result ) {
    if ( !Add( name ) ) {
        return;
    }
    if ( result < 0 ) {
        N_Error( ERR_DROP, "%s: failed to register object behaviour %s -- %s", func, name, AS_PrintErrorString( result ) );
    }
}

static void ValidateObjectProperty( const char *func, const char *name, int result ) {
    if ( !Add( name ) ) {
        return;
    }
    if ( result < 0 ) {
        N_Error( ERR_DROP, "%s: failed to register object property %s -- %s", func, name, AS_PrintErrorString( result ) );
    }
}

static void ValidateTypedef( const char *func, const char *name, int result ) {
    if ( !Add( name ) ) {
        return;
    }
    if ( result < 0 ) {
        N_Error( ERR_DROP, "%s: failed to register typedef %s -- %s", func, name, AS_PrintErrorString( result ) );
    }
}

static void ValidateGlobalVar( const char *func, const char *name, int result ) {
    if ( !Add( name ) ) {
        return;
    }
    if ( result < 0 ) {
        N_Error( ERR_DROP, "%s: failed to register global variable %s -- %s", func, name, AS_PrintErrorString( result ) );
    }
}

DEFINE_CALLBACK( CvarRegisterGeneric ) {
    vmCvar_t vmCvar;

    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    string_t *value = (string_t *)pGeneric->GetArgObject( 1 );
    asDWORD flags = pGeneric->GetArgDWord( 2 );
    asINT64 *intValue = (asINT64 *)pGeneric->GetArgAddress( 3 );
    float *floatValue = (float *)pGeneric->GetArgAddress( 4 );
    asINT32 *modificationCount = (asINT32 *)pGeneric->GetArgAddress( 5 );
    cvarHandle_t *cvarHandle = (cvarHandle_t *)pGeneric->GetArgAddress( 6 );

    Cvar_Register( &vmCvar, name->c_str(), value->c_str(), flags, CVAR_PRIVATE );

    *value = (const char *)vmCvar.s;
    *intValue = vmCvar.i;
    *floatValue = vmCvar.f;
    *modificationCount = vmCvar.modificationCount;
    *cvarHandle = vmCvar.handle;
}

DEFINE_CALLBACK( CvarUpdateGeneric ) {
    vmCvar_t vmCvar;

    string_t *value = (string_t *)pGeneric->GetArgObject( 1 );
    asINT64 *intValue = (asINT64 *)pGeneric->GetArgAddress( 2 );
    float *floatValue = (float *)pGeneric->GetArgAddress( 3 );
    asINT32 *modificationCount = (asINT32 *)pGeneric->GetArgAddress( 4 );
    const cvarHandle_t cvarHandle = pGeneric->GetArgWord( 5 );

    memset( &vmCvar, 0, sizeof(vmCvar) );
    N_strncpyz( vmCvar.s, value->c_str(), sizeof(vmCvar.s) );
    vmCvar.i = *intValue;
    vmCvar.f = *floatValue;
    vmCvar.modificationCount = *modificationCount;
    vmCvar.handle = cvarHandle;
    Cvar_Update( &vmCvar, CVAR_PRIVATE );

    *value = (const char *)vmCvar.s;
    *intValue = vmCvar.i;
    *floatValue = vmCvar.f;
    *modificationCount = vmCvar.modificationCount;
}

DEFINE_CALLBACK( CvarSetValueGeneric ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const string_t *value = (const string_t *)pGeneric->GetArgObject( 1 );
    Cvar_Set( name->c_str(), value->c_str() );
}

DEFINE_CALLBACK( CvarVariableString ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) string_t( Cvar_VariableString( ( (const string_t *)pGeneric->GetArgObject( 0 ) )->c_str() ) );
}

DEFINE_CALLBACK( CvarVariableInteger ) {
    *(int64_t *)pGeneric->GetAddressOfReturnLocation() = Cvar_VariableInteger( ( (const string_t *)pGeneric->GetArgObject( 0 ) )->c_str() );
}

DEFINE_CALLBACK( CvarVariableFloat ) {
    *(float *)pGeneric->GetAddressOfReturnLocation() = Cvar_VariableFloat( ( (const string_t *)pGeneric->GetArgObject( 0 ) )->c_str() );
}

DEFINE_CALLBACK( Snd_PlaySfx ) {
    Snd_PlaySfx( (sfxHandle_t)pGeneric->GetArgDWord( 0 ) );
}

DEFINE_CALLBACK( Snd_SetLoopingTrack ) {
    Snd_SetLoopingTrack( (sfxHandle_t)pGeneric->GetArgDWord( 0 ) );
}

DEFINE_CALLBACK( Snd_ClearLoopingTrack ) {
    Snd_ClearLoopingTrack();
}

DEFINE_CALLBACK( Snd_RegisterSfx ) {
    pGeneric->SetReturnDWord( Snd_RegisterSfx( ( (const string_t *)pGeneric->GetArgObject( 0 ) )->c_str() ) );
}

DEFINE_CALLBACK( Snd_RegisterTrack ) {
    pGeneric->SetReturnDWord( Snd_RegisterTrack( ( (const string_t *)pGeneric->GetArgObject( 0 ) )->c_str() ) );
}


DEFINE_CALLBACK( BeginSaveSection ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    g_pArchiveHandler->BeginSaveSection( name->c_str() );
}

DEFINE_CALLBACK( EndSaveSection ) {
    g_pArchiveHandler->EndSaveSection();
}

DEFINE_CALLBACK( SaveInt8 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const int8_t arg = *(const int8_t *)pGeneric->GetArgAddress( 1 );
    g_pArchiveHandler->SaveChar( name->c_str(), arg );
}

DEFINE_CALLBACK( SaveInt16 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const int16_t arg = *(const int16_t *)pGeneric->GetArgAddress( 1 );
    g_pArchiveHandler->SaveShort( name->c_str(), arg );
}

DEFINE_CALLBACK( SaveInt32 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const int32_t arg = *(const int32_t *)pGeneric->GetArgAddress( 1 );
    g_pArchiveHandler->SaveInt( name->c_str(), arg );
}

DEFINE_CALLBACK( SaveInt64 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const int64_t arg = *(const int64_t *)pGeneric->GetArgAddress( 1 );
    g_pArchiveHandler->SaveLong( name->c_str(), arg );
}

DEFINE_CALLBACK( SaveUInt8 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const uint8_t arg = *(const uint8_t *)pGeneric->GetArgAddress( 1 );
    g_pArchiveHandler->SaveByte( name->c_str(), arg );
}

DEFINE_CALLBACK( SaveUInt16 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const uint16_t arg = *(const uint16_t *)pGeneric->GetArgAddress( 1 );
    g_pArchiveHandler->SaveUShort( name->c_str(), arg );
}

DEFINE_CALLBACK( SaveUInt32 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const uint32_t arg = *(const uint32_t *)pGeneric->GetArgAddress( 1 );
    g_pArchiveHandler->SaveUInt( name->c_str(), arg );
}

DEFINE_CALLBACK( SaveUInt64 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const uint64_t arg = *(const uint64_t *)pGeneric->GetArgAddress( 1 );
    g_pArchiveHandler->SaveULong( name->c_str(), arg );
}

DEFINE_CALLBACK( SaveArray ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const CScriptArray *arg = (const CScriptArray *)pGeneric->GetArgObject( 1 );
    g_pArchiveHandler->SaveArray( name->c_str(), arg );
}

DEFINE_CALLBACK( SaveFloat ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const float arg = pGeneric->GetArgFloat( 1 );
    g_pArchiveHandler->SaveFloat( name->c_str(), arg );
}

DEFINE_CALLBACK( SaveString ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const string_t *arg = (const string_t *)pGeneric->GetArgObject( 1 );
    g_pArchiveHandler->SaveString( name->c_str(), arg );
}

DEFINE_CALLBACK( SaveVec2 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const vec2 v = *(const vec2 *)pGeneric->GetArgObject( 1 );
    g_pArchiveHandler->SaveVec2( name->c_str(), glm::value_ptr( v ) );
}

DEFINE_CALLBACK( SaveVec3 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const vec3 v = *(const vec3 *)pGeneric->GetArgObject( 1 );
    g_pArchiveHandler->SaveVec3( name->c_str(), glm::value_ptr( v ) );
}

DEFINE_CALLBACK( SaveVec4 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    const glm::vec4 v = *(const glm::vec4 *)pGeneric->GetArgObject( 1 );
    g_pArchiveHandler->SaveVec4( name->c_str(), glm::value_ptr( v ) );
}

DEFINE_CALLBACK( FindSaveSection ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    *(int32_t *)pGeneric->GetAddressOfReturnLocation() = g_pArchiveHandler->GetSection( name->c_str() );
}

DEFINE_CALLBACK( LoadInt8 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    *(int8_t *)pGeneric->GetAddressOfReturnLocation() = g_pArchiveHandler->LoadChar( name->c_str(), (int32_t)pGeneric->GetArgDWord( 1 ) );
}

DEFINE_CALLBACK( LoadInt16 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    *(int16_t *)pGeneric->GetAddressOfReturnLocation() = g_pArchiveHandler->LoadShort( name->c_str(), (int32_t)pGeneric->GetArgDWord( 1 ) );
}

DEFINE_CALLBACK( LoadInt32 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    *(int32_t *)pGeneric->GetAddressOfReturnLocation() = g_pArchiveHandler->LoadInt( name->c_str(), (int32_t)pGeneric->GetArgDWord( 1 ) );
}

DEFINE_CALLBACK( LoadInt64 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    *(int64_t *)pGeneric->GetAddressOfReturnLocation() = g_pArchiveHandler->LoadLong( name->c_str(), (int32_t)pGeneric->GetArgDWord( 1 ) );
}

DEFINE_CALLBACK( LoadUInt8 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    *(uint8_t *)pGeneric->GetAddressOfReturnLocation() = g_pArchiveHandler->LoadByte( name->c_str(), (int32_t)pGeneric->GetArgDWord( 1 ) );
}

DEFINE_CALLBACK( LoadUInt16 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    *(uint16_t *)pGeneric->GetAddressOfReturnLocation() = g_pArchiveHandler->LoadUShort( name->c_str(), (int32_t)pGeneric->GetArgDWord( 1 ) );
}

DEFINE_CALLBACK( LoadUInt32 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    *(uint32_t *)pGeneric->GetAddressOfReturnLocation() = g_pArchiveHandler->LoadUInt( name->c_str(), (int32_t)pGeneric->GetArgDWord( 1 ) );
}

DEFINE_CALLBACK( LoadUInt64 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = g_pArchiveHandler->LoadULong( name->c_str(), (int32_t)pGeneric->GetArgDWord( 1 ) );
}

DEFINE_CALLBACK( LoadFloat ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    *(float *)pGeneric->GetAddressOfReturnLocation() = g_pArchiveHandler->LoadFloat( name->c_str(), (int32_t)pGeneric->GetArgDWord( 1 ) );
}

DEFINE_CALLBACK( LoadString ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    string_t *arg = (string_t *)pGeneric->GetArgObject( 1 );
    g_pArchiveHandler->LoadString( name->c_str(), arg, (int32_t)pGeneric->GetArgDWord( 2 ) );
}

DEFINE_CALLBACK( LoadArray ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    CScriptArray *arg = (CScriptArray *)pGeneric->GetArgObject( 1 );
    g_pArchiveHandler->LoadArray( name->c_str(), arg, (int32_t)pGeneric->GetArgDWord( 2 ) );
}

DEFINE_CALLBACK( LoadVec2 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    vec2 *arg = (vec2 *)pGeneric->GetArgObject( 1 );
    g_pArchiveHandler->LoadVec2( name->c_str(), glm::value_ptr( *arg ), (int32_t)pGeneric->GetArgDWord( 2 ) );
}

DEFINE_CALLBACK( LoadVec3 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    vec3 *arg = (vec3 *)pGeneric->GetArgObject( 1 );
    g_pArchiveHandler->LoadVec3( name->c_str(), glm::value_ptr( *arg ), (int32_t)pGeneric->GetArgDWord( 2 ) );
}

DEFINE_CALLBACK( LoadVec4 ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    glm::vec4 *arg = (glm::vec4 *)pGeneric->GetArgObject( 1 );
    g_pArchiveHandler->LoadVec4( name->c_str(), glm::value_ptr( *arg ), (int32_t)pGeneric->GetArgDWord( 2 ) );
}


DEFINE_CALLBACK( AddVec4Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) vec4( *(const vec4 *)pGeneric->GetObjectData() + *(const vec4 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( SubVec4Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) vec4( *(const vec4 *)pGeneric->GetObjectData() - *(const vec4 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( MulVec4Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) vec4( *(const vec4 *)pGeneric->GetObjectData() * *(const vec4 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( DivVec4Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) vec4( *(const vec4 *)pGeneric->GetObjectData() / *(const vec4 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( IndexVec4Generic ) {
    pGeneric->SetReturnAddress( eastl::addressof( ( *(vec4 *)pGeneric->GetObjectData() )[ pGeneric->GetArgDWord( 0 ) ] ) );
}

DEFINE_CALLBACK( EqualsVec4Generic ) {
    *(bool *)pGeneric->GetAddressOfReturnLocation() = *(const vec4 *)pGeneric->GetObjectData() == *(const vec4 *)pGeneric->GetArgObject( 0 );
}

DEFINE_CALLBACK( CmpVec4Generic ) {
    const vec4& b = *(const vec4 *)pGeneric->GetArgObject( 0 );
    const vec4& a = *(const vec4 *)pGeneric->GetObjectData();

    int cmp = 0;
    if ( a.x < b.x && a.y < b.y && a.z < b.z && a.w < b.w ) {
        cmp = -1;
    } else if ( b.x < a.x && b.y < a.y && b.z < a.z && b.w < a.w ) {
        cmp = 1;
    }

    pGeneric->SetReturnDWord( (asDWORD)cmp );
}


DEFINE_CALLBACK( AddVec3Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) vec3( *(const vec3 *)pGeneric->GetObjectData() + *(const vec3 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( SubVec3Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) vec3( *(const vec3 *)pGeneric->GetObjectData() - *(const vec3 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( MulVec3Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) vec3( *(const vec3 *)pGeneric->GetObjectData() * *(const vec3 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( DivVec3Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) vec3( *(const vec3 *)pGeneric->GetObjectData() / *(const vec3 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( IndexVec3Generic ) {
    pGeneric->SetReturnAddress( eastl::addressof( ( *(vec3 *)pGeneric->GetObjectData() )[ pGeneric->GetArgDWord( 0 ) ] ) );
}

DEFINE_CALLBACK( EqualsVec3Generic ) {
    *(bool *)pGeneric->GetAddressOfReturnLocation() = *(const vec3 *)pGeneric->GetObjectData() == *(const vec3 *)pGeneric->GetArgObject( 0 );
}

DEFINE_CALLBACK( CmpVec3Generic ) {
    const vec3& b = *(const vec3 *)pGeneric->GetArgObject( 0 );
    const vec3& a = *(const vec3 *)pGeneric->GetObjectData();

    int cmp = 0;
    if ( a.x < b.x && a.y < b.y && a.z < b.z ) {
        cmp = -1;
    } else if ( b.x < a.x && b.y < a.y && b.z < a.z ) {
        cmp = 1;
    }

    pGeneric->SetReturnDWord( (asDWORD)cmp );
}

DEFINE_CALLBACK( AddVec2Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) vec2( *(const vec2 *)pGeneric->GetObjectData() + *(const vec2 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( SubVec2Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) vec2( *(const vec2 *)pGeneric->GetObjectData() - *(const vec2 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( MulVec2Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) vec2( *(const vec2 *)pGeneric->GetObjectData() * *(const vec2 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( DivVec2Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) vec2( *(const vec2 *)pGeneric->GetObjectData() / *(const vec2 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( IndexVec2Generic ) {
    pGeneric->SetReturnAddress( eastl::addressof( ( *(vec2 *)pGeneric->GetObjectData() )[ pGeneric->GetArgDWord( 0 ) ] ) );
}

DEFINE_CALLBACK( EqualsVec2Generic ) {
    *(bool *)pGeneric->GetAddressOfReturnLocation() = *(const vec2 *)pGeneric->GetObjectData() == *(const vec2 *)pGeneric->GetArgObject( 0 );
}

DEFINE_CALLBACK( CmpVec2Generic ) {
    const vec2& b = *(const vec2 *)pGeneric->GetArgObject( 0 );
    const vec2& a = *(const vec2 *)pGeneric->GetObjectData();

    int cmp = 0;
    if ( a.x < b.x && a.y < b.y ) {
        cmp = -1;
    } else if ( b.x < a.x && b.y < a.y ) {
        cmp = 1;
    }

    pGeneric->SetReturnDWord( (asDWORD)cmp );
}


DEFINE_CALLBACK( AddIVec4Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) ivec4( *(const ivec4 *)pGeneric->GetObjectData() + *(const ivec4 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( SubIVec4Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) ivec4( *(const ivec4 *)pGeneric->GetObjectData() - *(const ivec4 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( MulIVec4Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) ivec4( *(const ivec4 *)pGeneric->GetObjectData() * *(const ivec4 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( DivIVec4Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) ivec4( *(const ivec4 *)pGeneric->GetObjectData() / *(const ivec4 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( IndexIVec4Generic ) {
    pGeneric->SetReturnAddress( eastl::addressof( ( *(vec4 *)pGeneric->GetObjectData() )[ pGeneric->GetArgDWord( 0 ) ] ) );
}

DEFINE_CALLBACK( EqualsIVec4Generic ) {
    *(bool *)pGeneric->GetAddressOfReturnLocation() = *(const ivec4 *)pGeneric->GetObjectData() == *(const ivec4 *)pGeneric->GetArgObject( 0 );
}

DEFINE_CALLBACK( CmpIVec4Generic ) {
    const ivec4& b = *(const ivec4 *)pGeneric->GetArgObject( 0 );
    const ivec4& a = *(const ivec4 *)pGeneric->GetObjectData();

    int cmp = 0;
    if ( a.x < b.x && a.y < b.y && a.z < b.z && a.w < b.w ) {
        cmp = -1;
    } else if ( b.x < a.x && b.y < a.y && b.z < a.z && b.w < a.w ) {
        cmp = 1;
    }

    pGeneric->SetReturnDWord( (asDWORD)cmp );
}


DEFINE_CALLBACK( AddIVec3Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) ivec3( *(const ivec3 *)pGeneric->GetObjectData() + *(const ivec3 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( SubIVec3Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) ivec3( *(const ivec3 *)pGeneric->GetObjectData() - *(const ivec3 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( MulIVec3Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) ivec3( *(const ivec3 *)pGeneric->GetObjectData() * *(const ivec3 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( DivIVec3Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) ivec3( *(const ivec3 *)pGeneric->GetObjectData() / *(const ivec3 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( IndexIVec3Generic ) {
    pGeneric->SetReturnAddress( eastl::addressof( ( *(ivec3 *)pGeneric->GetObjectData() )[ pGeneric->GetArgDWord( 0 ) ] ) );
}

DEFINE_CALLBACK( EqualsIVec3Generic ) {
    *(bool *)pGeneric->GetAddressOfReturnLocation() = *(const ivec3 *)pGeneric->GetObjectData() == *(const ivec3 *)pGeneric->GetArgObject( 0 );
}

DEFINE_CALLBACK( CmpIVec3Generic ) {
    const ivec3& b = *(const ivec3 *)pGeneric->GetArgObject( 0 );
    const ivec3& a = *(const ivec3 *)pGeneric->GetObjectData();

    int cmp = 0;
    if ( a.x < b.x && a.y < b.y && a.z < b.z ) {
        cmp = -1;
    } else if ( b.x < a.x && b.y < a.y && b.z < a.z ) {
        cmp = 1;
    }

    pGeneric->SetReturnDWord( (asDWORD)cmp );
}

DEFINE_CALLBACK( AddIVec2Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) ivec2( *(const ivec2 *)pGeneric->GetObjectData() + *(const ivec2 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( SubIVec2Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) ivec2( *(const ivec2 *)pGeneric->GetObjectData() - *(const ivec2 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( MulIVec2Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) ivec2 ( *(const ivec2 *)pGeneric->GetObjectData() * *(const ivec2 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( DivIVec2Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) ivec2( *(const ivec2 *)pGeneric->GetObjectData() / *(const ivec2 *)pGeneric->GetArgObject( 0 ) ) ;
}

DEFINE_CALLBACK( IndexIVec2Generic ) {
    pGeneric->SetReturnAddress( eastl::addressof( ( *(ivec2 *)pGeneric->GetObjectData() )[ pGeneric->GetArgDWord( 0 ) ] ) );
}

DEFINE_CALLBACK( EqualsIVec2Generic ) {
    *(bool *)pGeneric->GetAddressOfReturnLocation() = *(const ivec2 *)pGeneric->GetObjectData() == *(const ivec2 *)pGeneric->GetArgObject( 0 );
}

DEFINE_CALLBACK( CmpIVec2Generic ) {
    const ivec2& b = *(const ivec2 *)pGeneric->GetArgObject( 0 );
    const ivec2& a = *(const ivec2 *)pGeneric->GetObjectData();

    int cmp = 0;
    if ( a.x < b.x && a.y < b.y ) {
        cmp = -1;
    } else if ( b.x < a.x && b.y < a.y ) {
        cmp = 1;
    }

    pGeneric->SetReturnDWord( (asDWORD)cmp );
}

DEFINE_CALLBACK( AddUVec4Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) uvec4( *(const uvec4 *)pGeneric->GetObjectData() + *(const uvec4 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( SubUVec4Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) uvec4( *(const uvec4 *)pGeneric->GetObjectData() - *(const uvec4 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( MulUVec4Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) uvec4( *(const uvec4 *)pGeneric->GetObjectData() * *(const uvec4 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( DivUVec4Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) uvec4( *(const uvec4 *)pGeneric->GetObjectData() / *(const uvec4 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( IndexUVec4Generic ) {
    pGeneric->SetReturnAddress( eastl::addressof( ( *(vec4 *)pGeneric->GetObjectData() )[ pGeneric->GetArgDWord( 0 ) ] ) );
}

DEFINE_CALLBACK( EqualsUVec4Generic ) {
    *(bool *)pGeneric->GetAddressOfReturnLocation() = *(const uvec4 *)pGeneric->GetObjectData() == *(const uvec4 *)pGeneric->GetArgObject( 0 );
}

DEFINE_CALLBACK( CmpUVec4Generic ) {
    const uvec4& b = *(const uvec4 *)pGeneric->GetArgObject( 0 );
    const uvec4& a = *(const uvec4 *)pGeneric->GetObjectData();

    int cmp = 0;
    if ( a.x < b.x && a.y < b.y && a.z < b.z && a.w < b.w ) {
        cmp = -1;
    } else if ( b.x < a.x && b.y < a.y && b.z < a.z && b.w < a.w ) {
        cmp = 1;
    }

    pGeneric->SetReturnDWord( (asDWORD)cmp );
}


DEFINE_CALLBACK( AddUVec3Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) uvec3( *(const uvec3 *)pGeneric->GetObjectData() + *(const uvec3 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( SubUVec3Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) uvec3( *(const uvec3 *)pGeneric->GetObjectData() - *(const uvec3 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( MulUVec3Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) uvec3( *(const uvec3 *)pGeneric->GetObjectData() * *(const uvec3 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( DivUVec3Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) uvec3( *(const uvec3 *)pGeneric->GetObjectData() / *(const uvec3 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( IndexUVec3Generic ) {
    pGeneric->SetReturnAddress( eastl::addressof( ( *(uvec3 *)pGeneric->GetObjectData() )[ pGeneric->GetArgDWord( 0 ) ] ) );
}

DEFINE_CALLBACK( EqualsUVec3Generic ) {
    *(bool *)pGeneric->GetAddressOfReturnLocation() = *(const uvec3 *)pGeneric->GetObjectData() == *(const uvec3 *)pGeneric->GetArgObject( 0 );
}

DEFINE_CALLBACK( CmpUVec3Generic ) {
    const uvec3& b = *(const uvec3 *)pGeneric->GetArgObject( 0 );
    const uvec3& a = *(const uvec3 *)pGeneric->GetObjectData();

    int cmp = 0;
    if ( a.x < b.x && a.y < b.y && a.z < b.z ) {
        cmp = -1;
    } else if ( b.x < a.x && b.y < a.y && b.z < a.z ) {
        cmp = 1;
    }

    pGeneric->SetReturnDWord( (asDWORD)cmp );
}

DEFINE_CALLBACK( AddUVec2Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) uvec2( *(const uvec2 *)pGeneric->GetObjectData() + *(const uvec2 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( SubUVec2Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) uvec2( *(const uvec2 *)pGeneric->GetObjectData() - *(const uvec2 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( MulUVec2Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) uvec2( *(const uvec2 *)pGeneric->GetObjectData() * *(const uvec2 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( DivUVec2Generic ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) uvec2( *(const uvec2 *)pGeneric->GetObjectData() / *(const uvec2 *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( IndexUVec2Generic ) {
    pGeneric->SetReturnAddress( eastl::addressof( ( *(uvec2 *)pGeneric->GetObjectData() )[ pGeneric->GetArgDWord( 0 ) ] ) );
}

DEFINE_CALLBACK( EqualsUVec2Generic ) {
    *(bool *)pGeneric->GetAddressOfReturnLocation() = *(const uvec2 *)pGeneric->GetObjectData() == *(const uvec2 *)pGeneric->GetArgObject( 0 );
}

DEFINE_CALLBACK( CmpUVec2Generic ) {
    const uvec2& b = *(const uvec2 *)pGeneric->GetArgObject( 0 );
    const uvec2& a = *(const uvec2 *)pGeneric->GetObjectData();

    int cmp = 0;
    if ( a.x < b.x && a.y < b.y ) {
        cmp = -1;
    } else if ( b.x < a.x && b.y < a.y ) {
        cmp = 1;
    }

    pGeneric->SetReturnDWord( (asDWORD)cmp );
}

DEFINE_CALLBACK( ConsolePrint ) {
    const string_t *msg = (const string_t *)pGeneric->GetArgObject( 0 );
    Con_Printf( "%s", msg->c_str() );
}

DEFINE_CALLBACK( ConsoleWarning ) {
    const string_t *msg = (const string_t *)pGeneric->GetArgObject( 0 );
    Con_Printf( COLOR_YELLOW "WARNING: %s", msg->c_str() );
}

DEFINE_CALLBACK( GameError ) {
    const string_t *msg = (const string_t *)pGeneric->GetArgObject( 0 );
    Con_Printf( COLOR_RED "(ERROR) ModuleException Thrown: %s\n", msg->c_str() );
    throw ModuleException( msg );
}

DEFINE_CALLBACK( StringBufferToInt ) {
    pGeneric->SetReturnDWord( (asDWORD)atoi( (const char *)pGeneric->GetArgAddress( 0 ) ) );
}

DEFINE_CALLBACK( StringBufferToUInt ) {
    pGeneric->SetReturnDWord( (asDWORD)atoll( (const char *)pGeneric->GetArgAddress( 0 ) ) );
}

DEFINE_CALLBACK( StringBufferToFloat ) {
    pGeneric->SetReturnFloat( N_atof( (const char *)pGeneric->GetArgAddress( 0 ) ) );
}

DEFINE_CALLBACK( StringToInt ) {
    pGeneric->SetReturnDWord( (asDWORD)atoi( ( (const string_t *)pGeneric->GetArgObject( 0 ) )->c_str() ) );
}

DEFINE_CALLBACK( StringToUInt ) {
    pGeneric->SetReturnDWord( (asDWORD)atoll( ( (const string_t *)pGeneric->GetArgObject( 0 ) )->c_str() ) );
}

DEFINE_CALLBACK( StringToFloat ) {
    pGeneric->SetReturnFloat( N_atof( ( (const string_t *)pGeneric->GetArgObject( 0 ) )->c_str() ) );
}

DEFINE_CALLBACK( DistanceVec3Vec3 ) {
    const vec3& src = *(const vec3 *)pGeneric->GetArgObject( 0 );
    const vec3& target = *(const vec3 *)pGeneric->GetArgObject( 1 );
    pGeneric->SetReturnFloat( disBetweenOBJ( glm::value_ptr( src ), glm::value_ptr( target ) ) );
}

DEFINE_CALLBACK( BoundsIntersect ) {
    const bbox_t a = ( (const CModuleBoundBox *)pGeneric->GetArgObject( 0 ) )->ToPOD();
    const bbox_t b = ( (const CModuleBoundBox *)pGeneric->GetArgObject( 1 ) )->ToPOD();

    pGeneric->SetReturnDWord( (asDWORD)BoundsIntersect( &a, &b ) );
}

DEFINE_CALLBACK( BoundsIntersectPoint ) {
    const bbox_t a = ( (const CModuleBoundBox *)pGeneric->GetArgObject( 0 ) )->ToPOD();
    const vec3& point = *(const vec3 *)pGeneric->GetArgObject( 1 );

    pGeneric->SetReturnDWord( (asDWORD)BoundsIntersectPoint( &a, glm::value_ptr( point ) ) );
}

DEFINE_CALLBACK( BoundsIntersectSphere ) {
    const bbox_t a = ( (const CModuleBoundBox *)pGeneric->GetArgObject( 0 ) )->ToPOD();
    const vec3& point = *(const vec3 *)pGeneric->GetArgObject( 1 );
    float radius = pGeneric->GetArgFloat( 2 );

    pGeneric->SetReturnDWord( (asDWORD)BoundsIntersectSphere( &a, glm::value_ptr( point ), radius ) );
}

DEFINE_CALLBACK( StrICmp ) {
    const string_t *arg0 = (const string_t *)pGeneric->GetArgObject( 0 );
    const string_t *arg1 = (const string_t *)pGeneric->GetArgObject( 1 );
    pGeneric->SetReturnDWord( (asDWORD)N_stricmp( arg0->c_str(), arg1->c_str() ) );
}

DEFINE_CALLBACK( StrCmp ) {
    const string_t *arg0 = (const string_t *)pGeneric->GetArgObject( 0 );
    const string_t *arg1 = (const string_t *)pGeneric->GetArgObject( 1 );
    pGeneric->SetReturnDWord( (asDWORD)N_strcmp( arg0->c_str(), arg1->c_str() ) );
}

DEFINE_CALLBACK( ClearScene ) {
    re.ClearScene();
}

DEFINE_CALLBACK( RenderScene ) {
    renderSceneRef_t refdef;
    memset( &refdef, 0, sizeof( refdef ) );
    refdef.x = pGeneric->GetArgDWord( 0 );
    refdef.y = pGeneric->GetArgDWord( 1 );
    refdef.width = pGeneric->GetArgDWord( 2 );
    refdef.height = pGeneric->GetArgDWord( 3 );
    refdef.flags = pGeneric->GetArgDWord( 4 );
    refdef.time = pGeneric->GetArgDWord( 5 );
    re.RenderScene( &refdef );
}

DEFINE_CALLBACK( AddEntityToScene ) {
    refEntity_t refEntity;
    memset( &refEntity, 0, sizeof( refEntity ) );
    refEntity.hShader = (nhandle_t)pGeneric->GetArgDWord( 0 );
    VectorCopy( refEntity.origin, (float *)pGeneric->GetArgAddress( 1 ) );
    refEntity.flags = pGeneric->GetArgDWord( 2 );
}

DEFINE_CALLBACK( AddPolyToScene ) {
    CScriptArray *pPolyList = (CScriptArray *)pGeneric->GetArgObject( 1 );
    re.AddPolyToScene( (nhandle_t)pGeneric->GetArgDWord( 0 ), (const polyVert_t *)pPolyList->GetBuffer(), pPolyList->GetSize() );
}

DEFINE_CALLBACK( AddSpriteToScene ) {
    re.AddSpriteToScene( glm::value_ptr( *(const vec3 *)pGeneric->GetArgObject( 0  ) ), (nhandle_t)pGeneric->GetArgDWord( 1 ),
        (nhandle_t)pGeneric->GetArgDWord( 2 ) );
}

DEFINE_CALLBACK( RegisterShader ) {
    pGeneric->SetReturnDWord( re.RegisterShader( ( (const string_t *)pGeneric->GetArgObject( 0 ) )->c_str() ) );
}

DEFINE_CALLBACK( RegisterSprite ) {
    pGeneric->SetReturnDWord( re.RegisterSprite( (nhandle_t)pGeneric->GetArgDWord( 0 ), pGeneric->GetArgDWord( 1 ) ) );
}

DEFINE_CALLBACK( RegisterSpriteSheet ) {
    const string_t *npath = (const string_t *)pGeneric->GetArgObject( 0 );
    pGeneric->SetReturnDWord( re.RegisterSpriteSheet( npath->c_str(), pGeneric->GetArgDWord( 1 ), pGeneric->GetArgDWord( 2 ),
        pGeneric->GetArgDWord( 3 ), pGeneric->GetArgDWord( 4 ) ) );
}

DEFINE_CALLBACK( CheckWallHit ) {
    const vec3& origin = *(const vec3 *)pGeneric->GetArgObject( 0 );
    *(bool *)pGeneric->GetAddressOfReturnLocation() = g_world.CheckWallHit( glm::value_ptr( origin ), dirtype_t( pGeneric->GetArgDWord( 1 ) ) );
}

DEFINE_CALLBACK( CastRay ) {
    g_world.CastRay( (ray_t *)pGeneric->GetArgObject( 0 ) );
}

DEFINE_CALLBACK( LoadMap ) {
    pGeneric->SetReturnDWord( (asDWORD)G_LoadMap( ( (const string_t *)pGeneric->GetArgObject( 0 ) )->c_str() ) );
}

DEFINE_CALLBACK( SetActiveMap ) {
    nhandle_t hMap = (nhandle_t)pGeneric->GetArgDWord( 0 );
    uint32_t *nCheckpoints = (uint32_t *)pGeneric->GetArgAddress( 1 );
    uint32_t *nSpawns = (uint32_t *)pGeneric->GetArgAddress( 2 );
    uint32_t *nTiles = (uint32_t *)pGeneric->GetArgAddress( 3 );
    float *soundBits = (float *)pGeneric->GetArgAddress( 4 );
    CModuleLinkEntity *activeEnts = (CModuleLinkEntity *)pGeneric->GetArgObject( 5 );

    G_SetActiveMap( hMap, nCheckpoints, nSpawns, nTiles, soundBits, &activeEnts->handle );
}

DEFINE_CALLBACK( GetTileData ) {
    uint32_t *data = (uint32_t *)pGeneric->GetArgAddress( 0 );
    G_GetTileData( data );
}

DEFINE_CALLBACK( GetCheckpointData ) {
    uvec3 *data = (uvec3 *)pGeneric->GetArgObject( 0 );
    G_GetCheckpointData( (uvec_t *)data, pGeneric->GetArgDWord( 1 ) );
}

DEFINE_CALLBACK( GetSpawnData ) {
    uvec3 *origin = (uvec3 *)pGeneric->GetArgObject( 0 );
    uint32_t *type = (uint32_t *)pGeneric->GetArgAddress( 1 );
    uint32_t *id = (uint32_t *)pGeneric->GetArgAddress( 2 );
    G_GetSpawnData( (uvec_t *)origin, type, id, pGeneric->GetArgDWord( 3 ) );
}

DEFINE_CALLBACK( GetGPUConfig ) {
    CModuleGPUConfig *config = (CModuleGPUConfig *)pGeneric->GetArgObject( 0 );
    config->gpuConfig = gi.gpuConfig;
    config->shaderVersionString = config->gpuConfig.shader_version_str;
    config->versionString = config->gpuConfig.version_string;
    config->rendererString = config->gpuConfig.renderer_string;
    config->extensionsString = config->gpuConfig.extensions_string;
}

DEFINE_CALLBACK( IsAnyKeyDown ) {
    pGeneric->SetReturnDWord( (asDWORD)Key_AnyDown() );
}

DEFINE_CALLBACK( KeyIsDown ) {
    pGeneric->SetReturnDWord( (asDWORD)Key_IsDown( pGeneric->GetArgDWord( 0 ) ) );
}

DEFINE_CALLBACK( OpenFileRead ) {
    const string_t *fileName = (const string_t *)pGeneric->GetArgObject( 0 );
    *(nhandle_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_FOpenRead( fileName->c_str(), H_SGAME );
}

DEFINE_CALLBACK( OpenFileWrite ) {
    const string_t *fileName = (const string_t *)pGeneric->GetArgObject( 0 );
    *(nhandle_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_FOpenWrite( fileName->c_str(), H_SGAME );
}

DEFINE_CALLBACK( OpenFileAppend ) {
    const string_t *fileName = (const string_t *)pGeneric->GetArgObject( 0 );
    *(nhandle_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_FOpenAppend( fileName->c_str(), H_SGAME );
}

DEFINE_CALLBACK( OpenFileRW ) {
    const string_t *fileName = (const string_t *)pGeneric->GetArgObject( 0 );
    *(nhandle_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_FOpenRW( fileName->c_str(), H_SGAME );
}

DEFINE_CALLBACK( OpenFile ) {
    const string_t *fileName = (const string_t *)pGeneric->GetArgObject( 0 );
    fileHandle_t *hFile = (fileHandle_t *)pGeneric->GetArgAddress( 1 );
    fileMode_t mode = (fileMode_t )pGeneric->GetArgDWord( 2 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_FOpenFile( fileName->c_str(), hFile, mode, H_SGAME );
}

DEFINE_CALLBACK( CloseFile ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    FS_VM_FClose( arg, H_SGAME );
}

DEFINE_CALLBACK( GetFileLength ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_FileLength( arg, H_SGAME );
}

DEFINE_CALLBACK( GetFilePosition ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_FileTell( arg, H_SGAME );
}

DEFINE_CALLBACK( FileSetPosition ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    uint64_t offset = pGeneric->GetArgQWord( 1 );
    uint32_t whence = pGeneric->GetArgDWord( 2 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_FileSeek( arg, offset, whence, H_SGAME );
}

DEFINE_CALLBACK( WriteInt8 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    int8_t data = (int8_t)pGeneric->GetArgByte( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Write( &data, sizeof( data ), arg, H_SGAME );
}

DEFINE_CALLBACK( WriteInt16 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    int16_t data = (int16_t)pGeneric->GetArgWord( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Write( &data, sizeof( data ), arg, H_SGAME );
}

DEFINE_CALLBACK( WriteInt32 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    int32_t data = (int32_t)pGeneric->GetArgDWord( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Write( &data, sizeof( data ), arg, H_SGAME );
}

DEFINE_CALLBACK( WriteInt64 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    int64_t data = (int64_t)pGeneric->GetArgQWord( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Write( &data, sizeof( data ), arg, H_SGAME );
}

DEFINE_CALLBACK( WriteUInt8 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    uint8_t data = pGeneric->GetArgByte( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Write( &data, sizeof( data ), arg, H_SGAME );
}

DEFINE_CALLBACK( WriteUInt16 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    uint16_t data = pGeneric->GetArgWord( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Write( &data, sizeof( data ), arg, H_SGAME );
}

DEFINE_CALLBACK( WriteUInt32 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    uint32_t data = pGeneric->GetArgDWord( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Write( &data, sizeof( data ), arg, H_SGAME );
}

DEFINE_CALLBACK( WriteUInt64 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    uint64_t data = pGeneric->GetArgQWord( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Write( &data, sizeof( data ), arg, H_SGAME );
}

DEFINE_CALLBACK( WriteString ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    const string_t *data = (const string_t *)pGeneric->GetArgObject( 0 );
    uint64_t length;

    length = data->size();
    if ( !FS_VM_Write( &length, sizeof( length ), arg, H_SGAME ) ) {
        *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = 0;
        return;
    }

    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Write( data->c_str(), length, arg, H_SGAME );
}

DEFINE_CALLBACK( ReadInt8 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    int8_t *data = (int8_t *)pGeneric->GetArgAddress( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Read( data, sizeof( *data ), arg, H_SGAME );
}

DEFINE_CALLBACK( ReadInt16 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    int16_t *data = (int16_t *)pGeneric->GetArgAddress( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Read( data, sizeof( *data ), arg, H_SGAME );
}

DEFINE_CALLBACK( ReadInt32 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    int32_t *data = (int32_t *)pGeneric->GetArgAddress( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Read( data, sizeof( *data ), arg, H_SGAME );
}

DEFINE_CALLBACK( ReadInt64 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    int64_t *data = (int64_t *)pGeneric->GetArgAddress( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Read( data, sizeof( *data ), arg, H_SGAME );
}

DEFINE_CALLBACK( ReadUInt8 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    uint8_t *data = (uint8_t *)pGeneric->GetArgAddress( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Read( data, sizeof( *data ), arg, H_SGAME );
}

DEFINE_CALLBACK( ReadUInt16 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    uint16_t *data = (uint16_t *)pGeneric->GetArgAddress( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Read( data, sizeof( *data ), arg, H_SGAME );
}

DEFINE_CALLBACK( ReadUInt32 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    uint32_t *data = (uint32_t *)pGeneric->GetArgAddress( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Read( data, sizeof( *data ), arg, H_SGAME );
}

DEFINE_CALLBACK( ReadUInt64 ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    uint64_t *data = (uint64_t *)pGeneric->GetArgAddress( 0 );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Read( data, sizeof( *data ), arg, H_SGAME );
}

DEFINE_CALLBACK( ReadString ) {
    fileHandle_t arg = (fileHandle_t)pGeneric->GetArgDWord( 1 );
    string_t *data = (string_t *)pGeneric->GetArgObject( 0 );
    uint64_t length;

    if ( !FS_VM_Read( &length, sizeof( length ), arg, H_SGAME ) ) {
        *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = 0;
        return;
    }

    data->resize( length );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = FS_VM_Read( data->data(), length, arg, H_SGAME );
}

DEFINE_CALLBACK( LoadFileString ) {
    const string_t *fileName = (const string_t *)pGeneric->GetArgObject( 0 );
    string_t *buffer = (string_t *)pGeneric->GetArgObject( 1 );

    void *v;
    const uint64_t length = FS_LoadFile( fileName->c_str(), &v );
    if ( !length || !v ) {
        Con_Printf( COLOR_RED "ERROR: failed to load file '%s' at vm request\n", fileName->c_str() );
        *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = 0;
        return;
    }

    buffer->resize( length );
    memcpy( buffer->data(), v, length );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = length;

    FS_FreeFile( v );
}

DEFINE_CALLBACK( LoadFile ) {
    const string_t *fileName = (const string_t *)pGeneric->GetArgObject( 0 );
    CScriptArray *buffer = (CScriptArray *)pGeneric->GetArgObject( 1 );

    void *v;
    uint64_t length = FS_LoadFile( fileName->c_str(), &v );
    if ( !length || !v ) {
        Con_Printf( COLOR_RED "ERROR: failed to load file '%s' at vm request\n", fileName->c_str() );
        *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = 0;
        return;
    }

    buffer->Resize( length );
    memcpy( buffer->GetBuffer(), v, length );
    *(uint64_t *)pGeneric->GetAddressOfReturnLocation() = length;

    FS_FreeFile( v );
}

DEFINE_CALLBACK( GetModuleDependencies ) {
    CScriptArray *depList = (CScriptArray *)pGeneric->GetArgObject( 0 );
    const string_t *modName = (const string_t *)pGeneric->GetArgObject( 1 );
    const CModuleInfo *info = g_pModuleLib->GetModule( modName->c_str() );

    if ( !info ) {
        Con_Printf( COLOR_YELLOW "GetModuleDependencies: no such module '%s'!\n", modName->c_str() );
        return;
    }

    depList->Resize( info->m_Dependencies.size() );
    for ( uint64_t i = 0; info->m_Dependencies.size(); i++ ) {
        *(string_t *)depList->At( i ) = info->m_Dependencies[i];
    }
}

DEFINE_CALLBACK( GetModuleList ) {
    CScriptArray *modList = (CScriptArray *)pGeneric->GetArgObject( 0 );
    const UtlVector<CModuleInfo *>& loadList = g_pModuleLib->GetLoadList();

    modList->Resize( loadList.size() );
    for ( uint64_t i = 0; i < loadList.size(); i++ ) {
        *(string_t *)modList->At( i ) = loadList[i]->m_szName;
    }
}

DEFINE_CALLBACK( IsModuleActive ) {
    const string_t *modName = (const string_t *)pGeneric->GetArgObject( 0 );
    *(bool *)pGeneric->GetAddressOfReturnLocation() = ModsMenu_IsModuleActive( modName->c_str() );
}

DEFINE_CALLBACK( ModuleAssertion ) {
}

DEFINE_CALLBACK( CmdArgvFixedGeneric ) {
    char *pBuffer = (char *)pGeneric->GetAddressOfArg( 0 );
    uint32_t size = pGeneric->GetArgDWord( 1 );
    Cmd_ArgvBuffer( pGeneric->GetArgDWord( 3 ), pBuffer, size );
}

DEFINE_CALLBACK( CmdArgcGeneric ) {
    *(uint32_t *)pGeneric->GetAddressOfReturnLocation() = Cmd_Argc();
}

DEFINE_CALLBACK( CmdArgvGeneric ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) string_t( Cmd_Argv( pGeneric->GetArgDWord( 0 ) ) );
}

DEFINE_CALLBACK( CmdArgsGeneric ) {
    new ( pGeneric->GetAddressOfReturnLocation() ) string_t( Cmd_ArgsFrom( pGeneric->GetArgDWord( 0 ) ) );
}

DEFINE_CALLBACK( CmdAddCommandGeneric ) {
    Cmd_AddCommand( ( (string_t *)pGeneric->GetArgObject( 0 ) )->c_str(), NULL );
}

DEFINE_CALLBACK( CmdRemoveCommandGeneric ) {
    Cmd_RemoveCommand( ( (string_t *)pGeneric->GetArgObject( 0 ) )->c_str() );
}

DEFINE_CALLBACK( CmdExecuteCommandGeneric ) {
    Cbuf_ExecuteText( EXEC_APPEND, ( (const string_t *)pGeneric->GetArgObject( 0 ) )->c_str() );
}

DEFINE_CALLBACK( PolyVertAssign ) {
    *(CModulePolyVert *)pGeneric->GetAddressOfReturnLocation() = *(CModulePolyVert *)pGeneric->GetArgObject( 0 );
}

DEFINE_CALLBACK( GetString ) {
    const string_t *name = (const string_t *)pGeneric->GetArgObject( 0 );
    string_t *value = (string_t *)pGeneric->GetArgObject( 1 );

    const stringHash_t *hash = strManager->ValueForKey( name->c_str() );
    *value = hash->value;
}

typedef struct ImGuiManager_s {
    ImGuiManager_s( void ) {
        memset( this, 0, sizeof( *this ) );
    }

    int nWindowStackDepth;
    int nColorStackDepth;
    int nStyleVarStackDepth;
    int nTableStackDepth;
    int nComboStackDepth;
} ImGuiManager_t;

static ImGuiManager_t ImGuiManager;

static bool ImGui_BeginCombo( const string_t *label, const string_t *preview ) {
    ImGuiManager.nComboStackDepth++;
    return ImGui::BeginCombo( label->c_str(), preview->c_str() );
}

static void ImGui_EndCombo( void ) {
    if ( !ImGuiManager.nComboStackDepth ) {
        N_Error( ERR_DROP, "ImGui::EndCombo: no combo active" );
    }
    ImGuiManager.nComboStackDepth--;
    ImGui::EndCombo();
}

static bool ImGui_Begin( const string_t *label, bool *open, ImGuiWindowFlags flags ) {
    ImGuiManager.nWindowStackDepth++;
    return ImGui::Begin( label->c_str(), open, flags );
}

static void ImGui_SetWindowSize( const vec2 *size ) {
    if ( !ImGuiManager.nWindowStackDepth ) {
        N_Error( ERR_DROP, "ImGui::SetWindowSize: no window active" );
    }
    ImGui::SetWindowSize( ImVec2( size->x, size->y ) );
}

static void ImGui_SetWindowPos( const vec2 *pos ) {
    if ( !ImGuiManager.nWindowStackDepth ) {
        N_Error( ERR_DROP, "ImGui::SetWindowPos: no window active" );
    }
    ImGui::SetWindowPos( ImVec2( pos->x, pos->y ) );
}

static void ImGui_End( void ) {
    if ( !ImGuiManager.nWindowStackDepth ) {
        N_Error( ERR_DROP, "ImGui::End: no window active" );
    }
    ImGuiManager.nWindowStackDepth--;
    ImGui::End();
}

static bool ImGui_ArrowButton( const string_t *label, ImGuiDir dir ) {
    return ImGui::ArrowButton( label->c_str(), dir );
}

static bool ImGui_Button( const string_t *label, const vec2 *size ) {
    return ImGui::Button( label->c_str(), ImVec2( size->x, size->y ) );
}

static void ImGui_PushStyleColor_U32( ImGuiCol idx, ImU32 col ) {
    ImGui::PushStyleColor( idx, col );
    ImGuiManager.nColorStackDepth++;
}

static void ImGui_PushStyleColor_V4( ImGuiCol idx, const vec4 *col ) {
    ImGui::PushStyleColor( idx, ImVec4( col->r, col->g, col->b, col->a ) );
    ImGuiManager.nColorStackDepth++;
}

static void ImGui_PopStyleColor( int amount ) {
    if ( !ImGuiManager.nColorStackDepth || ImGuiManager.nColorStackDepth - amount < 0 ) {
        N_Error( ERR_DROP, "ImGui::PopStyleColor: color stack underflow" );
    }
    ImGui::PopStyleColor( amount );
}

static bool ImGui_BeginTable( const string_t *label, int nColumns, ImGuiTableFlags flags ) {
    ImGuiManager.nTableStackDepth++;
    return ImGui::BeginTable( label->c_str(), nColumns, flags );
}

static void ImGui_TableNextColumn( void ) {
    if ( !ImGuiManager.nTableStackDepth ) {
        N_Error( ERR_DROP, "ImGui::TableNextColumn: no table active" );
    }
    ImGui::TableNextColumn();
}

static void ImGui_TableNextRow( void ) {
    if ( !ImGuiManager.nTableStackDepth ) {
        N_Error( ERR_DROP, "ImGui::TableNextRow: no table active" );
    }
    ImGui::TableNextRow();
}

static void ImGui_EndTable( void ) {
    if ( !ImGuiManager.nTableStackDepth ) {
        N_Error( ERR_DROP, "ImGui::EndTable: no table active" );
    }
    ImGuiManager.nTableStackDepth--;
    ImGui::EndTable();
}

static bool ImGui_Selectable( const string_t *label, bool selected, int, const vec2 * ) {
    return ImGui::Selectable( label->c_str(), selected );
}

static bool ImGui_InputText( const string_t *label, string_t *str, ImGuiInputTextFlags flags ) {
    return ImGui::InputText( label->c_str(), str, flags );
}

static void ImGui_Text( const string_t *text ) {
    ImGui::TextUnformatted( text->c_str() );
}

static bool ImGui_ColorEdit3( const string_t *label, vec3 *col, ImGuiColorEditFlags flags ) {
    return ImGui::ColorEdit3( label->c_str(), (float *)col, flags );
}

static bool ImGui_ColorEdit4( const string_t *label, vec4 *col, ImGuiColorEditFlags flags ) {
    return ImGui::ColorEdit4( label->c_str(), (float *)col, flags );
}

// stfu gcc
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
static void ImGui_TextColored( const vec4 *color, const string_t *text ) {
    ImGui::TextColored( ImVec4( color->r, color->g, color->b, color->a ), text->c_str() );
}
#pragma GCC diagnostic pop

static void ImGui_SliderInt( asIScriptGeneric *pGeneric ) {
    const string_t *label = (const string_t *)pGeneric->GetArgObject( 0 );
    int32_t *v = (int32_t *)pGeneric->GetArgAddress( 1 );
    int32_t min = (int32_t)pGeneric->GetArgDWord( 2 );
    int32_t max = (int32_t)pGeneric->GetArgDWord( 3 );
    ImGuiSliderFlags flags = pGeneric->GetArgDWord( 4 );

    *(bool *)pGeneric->GetAddressOfReturnLocation() = ImGui::SliderInt( label->c_str(), v, min, max, "%i", flags );
}

static void ImGui_SliderFloat( asIScriptGeneric *pGeneric ) {
    const string_t *label = (const string_t *)pGeneric->GetArgObject( 0 );
    float *v = (float *)pGeneric->GetAddressOfArg( 1 );
    float min = pGeneric->GetArgFloat( 2 );
    float max = pGeneric->GetArgFloat( 3 );
    ImGuiSliderFlags flags = *(int *)pGeneric->GetArgAddress( 4 );

    *(bool *)pGeneric->GetAddressOfReturnLocation() = ImGui::SliderFloat( label->c_str(), v, min, max, "%.3f", flags );
}

static void ImGui_SliderFloat2( asIScriptGeneric *pGeneric ) {
    const string_t *label = (const string_t *)pGeneric->GetArgObject( 0 );
    vec2 *v = (vec2 *)pGeneric->GetArgAddress( 1 );
    float min = pGeneric->GetArgFloat( 2 );
    float max = pGeneric->GetArgFloat( 3 );
    ImGuiSliderFlags flags = pGeneric->GetArgDWord( 4 );

    *(bool *)pGeneric->GetAddressOfReturnLocation() = ImGui::SliderFloat2( label->c_str(), (float *)v, min, max, "%.3f", flags );
}

static void ImGui_SliderFloat3( asIScriptGeneric *pGeneric ) {
    const string_t *label = (const string_t *)pGeneric->GetArgObject( 0 );
    vec3 *v = (vec3 *)pGeneric->GetArgAddress( 1 );
    float min = pGeneric->GetArgFloat( 2 );
    float max = pGeneric->GetArgFloat( 3 );
    ImGuiSliderFlags flags = pGeneric->GetArgDWord( 4 );

    *(bool *)pGeneric->GetAddressOfReturnLocation() = ImGui::SliderFloat3( label->c_str(), (float *)v, min, max, "%.3f", flags );
}

static void ImGui_SliderFloat4( asIScriptGeneric *pGeneric ) {
    const string_t *label = (const string_t *)pGeneric->GetArgObject( 0 );
    vec4 *v = (vec4 *)pGeneric->GetArgAddress( 1 );
    float min = pGeneric->GetArgFloat( 2 );
    float max = pGeneric->GetArgFloat( 3 );
    ImGuiSliderFlags flags = pGeneric->GetArgDWord( 4 );

    *(bool *)pGeneric->GetAddressOfReturnLocation() = ImGui::SliderFloat4( label->c_str(), (float *)v, min, max, "%.3f", flags );
}

static void ImGui_SliderAngle( asIScriptGeneric *pGeneric ) {
    const string_t *label = (const string_t *)pGeneric->GetArgObject( 0 );
    float *v = (float *)pGeneric->GetArgAddress( 1 );
    float min = pGeneric->GetArgFloat( 2 );
    float max = pGeneric->GetArgFloat( 3 );
    ImGuiSliderFlags flags = pGeneric->GetArgDWord( 4 );

    *(bool *)pGeneric->GetAddressOfReturnLocation() = ImGui::SliderAngle( label->c_str(), v, min, max, "%.0f deg", flags );
}

static bool ImGui_RadioButton( const string_t *label, bool selected ) {
    return ImGui::RadioButton( label->c_str(), selected );
}

//
// script globals
//
static const int8_t script_S_COLOR_BLACK = S_COLOR_BLACK;
static const int8_t script_S_COLOR_RED = S_COLOR_RED;
static const int8_t script_S_COLOR_GREEN = S_COLOR_GREEN;
static const int8_t script_S_COLOR_YELLOW = S_COLOR_YELLOW;
static const int8_t script_S_COLOR_BLUE = S_COLOR_BLUE;
static const int8_t script_S_COLOR_CYAN = S_COLOR_CYAN;
static const int8_t script_S_COLOR_MAGENTA = S_COLOR_MAGENTA;
static const int8_t script_S_COLOR_WHITE = S_COLOR_WHITE;
static const int8_t script_S_COLOR_RESET = S_COLOR_RESET;

static const string_t script_COLOR_BLACK = COLOR_BLACK;
static const string_t script_COLOR_RED = COLOR_RED;
static const string_t script_COLOR_GREEN = COLOR_GREEN;
static const string_t script_COLOR_YELLOW = COLOR_YELLOW;
static const string_t script_COLOR_BLUE = COLOR_BLUE;
static const string_t script_COLOR_CYAN = COLOR_CYAN;
static const string_t script_COLOR_MAGENTA = COLOR_MAGENTA;
static const string_t script_COLOR_WHITE = COLOR_WHITE;
static const string_t script_COLOR_RESET = COLOR_RESET;

static const asDWORD script_MAX_MAP_WIDTH = MAX_MAP_WIDTH;
static const asDWORD script_MAX_MAP_HEIGHT = MAX_MAP_HEIGHT;

static const asQWORD script_MAX_TOKEN_CHARS = MAX_TOKEN_CHARS;
static const asQWORD script_MAX_STRING_CHARS = MAX_STRING_CHARS;
static const asQWORD script_MAX_STRING_TOKENS = MAX_STRING_TOKENS;
static const asQWORD script_MAX_INFO_KEY = MAX_INFO_KEY;
static const asQWORD script_MAX_INFO_STRING = MAX_INFO_STRING;
static const asQWORD script_MAX_INFO_VALUE = MAX_INFO_VALUE;
static const asQWORD script_MAX_CVAR_NAME = MAX_CVAR_NAME;
static const asQWORD script_MAX_CVAR_VALUE = MAX_CVAR_VALUE;
static const asQWORD script_MAX_EDIT_LINE = MAX_EDIT_LINE;
static const asQWORD script_MAX_NPATH = MAX_NPATH;
static const asQWORD script_MAX_OSPATH = MAX_OSPATH;
static const asQWORD script_MAX_VERTS_ON_POLY = MAX_VERTS_ON_POLY;
static const asQWORD script_MAX_UI_FONTS = MAX_UI_FONTS;

static const asDWORD script_RSF_NORWORLDMODEL = RSF_NOWORLDMODEL;
static const asDWORD script_RSF_ORTHO_TYPE_CORDESIAN = RSF_ORTHO_TYPE_CORDESIAN;
static const asDWORD script_RSF_ORTHO_TYPE_WORLD = RSF_ORTHO_TYPE_WORLD;
static const asDWORD script_RSF_ORTHO_TYPE_SCREENSPACE = RSF_ORTHO_TYPE_SCREENSPACE;

static const asDWORD script_RT_SPRITE = RT_SPRITE;
static const asDWORD script_RT_LIGHTNING = RT_LIGHTNING;
static const asDWORD script_RT_POLY = RT_POLY;

static const int32_t script_FS_INVALID_HANDLE = FS_INVALID_HANDLE;
static const asDWORD script_FS_OPEN_READ = (asDWORD)FS_OPEN_READ;
static const asDWORD script_FS_OPEN_WRITE = (asDWORD)FS_OPEN_WRITE;
static const asDWORD script_FS_OPEN_APPEND = (asDWORD)FS_OPEN_APPEND;

static const asQWORD script_MAX_INT8 = CHAR_MAX;
static const asQWORD script_MAX_INT16 = SHRT_MAX;
static const asQWORD script_MAX_INT32 = INT_MAX;
static const asQWORD script_MAX_INT64 = LONG_MAX;
static const asQWORD script_MAX_UINT8 = UCHAR_MAX;
static const asQWORD script_MAX_UINT16 = USHRT_MAX;
static const asQWORD script_MAX_UINT32 = UINT_MAX;
static const asQWORD script_MAX_UINT64 = ULONG_MAX;

static const asQWORD script_MIN_INT8 = CHAR_MIN;
static const asQWORD script_MIN_INT16 = SHRT_MIN;
static const asQWORD script_MIN_INT32 = INT_MIN;

static const asDWORD script_CVAR_ROM = CVAR_ROM;
static const asDWORD script_CVAR_CHEAT = CVAR_CHEAT;
static const asDWORD script_CVAR_INIT = CVAR_INIT;
static const asDWORD script_CVAR_LATCH = CVAR_LATCH;
static const asDWORD script_CVAR_NODEFAULT = CVAR_NODEFAULT;
static const asDWORD script_CVAR_NORESTART = CVAR_NORESTART;
static const asDWORD script_CVAR_NOTABCOMPLETE = CVAR_NOTABCOMPLETE;
static const asDWORD script_CVAR_PROTECTED = CVAR_PROTECTED;
static const asDWORD script_CVAR_SAVE = CVAR_SAVE;
static const asDWORD script_CVAR_TEMP = CVAR_TEMP;
static const asDWORD script_CVAR_INVALID_HANDLE = CVAR_INVALID_HANDLE;

static const asDWORD script_SURFACEPARM_CHECKPOINT = SURFACEPARM_CHECKPOINT;
static const asDWORD script_SURFACEPARM_SPAWN = SURFACEPARM_SPAWN;
static const asDWORD script_SURFACEPARM_FLESH = SURFACEPARM_FLESH;
static const asDWORD script_SURFACEPARM_LAVA = SURFACEPARM_LAVA;
static const asDWORD script_SURFACEPARM_METAL = SURFACEPARM_METAL;
static const asDWORD script_SURFACEPARM_WOOD = SURFACEPARM_WOOD;
static const asDWORD script_SURFACEPARM_NODAMAGE = SURFACEPARM_NODAMAGE;
static const asDWORD script_SURFACEPARM_NODLIGHT = SURFACEPARM_NODLIGHT;
static const asDWORD script_SURFACEPARM_NOMARKS = SURFACEPARM_NOMARKS;
static const asDWORD script_SURFACEPARM_NOMISSILE = SURFACEPARM_NOMISSILE;
static const asDWORD script_SURFACEPARM_NOSTEPS = SURFACEPARM_NOSTEPS;
static const asDWORD script_SURFACEPARM_WATER = SURFACEPARM_WATER;

static const asDWORD script_NOMAD_VERSION = NOMAD_VERSION;
static const asDWORD script_NOMAD_VERSION_UPDATE = NOMAD_VERSION_UPDATE;
static const asDWORD script_NOMAD_VERSION_PATCH = NOMAD_VERSION_PATCH;

static const float script_M_PI = M_PI;

void ModuleLib_Register_Engine( void )
{
    PROFILE_FUNCTION();

    REGISTER_TYPEDEF( "int8", "char" );

//    SET_NAMESPACE( "TheNomad::Constants" );
    { // Constants
        REGISTER_GLOBAL_VAR( "const float M_PI", &script_M_PI );

        REGISTER_GLOBAL_VAR( "const uint32 SURFACEPARM_CHECKPOINT", &script_SURFACEPARM_CHECKPOINT );
        REGISTER_GLOBAL_VAR( "const uint32 SURFACEPARM_SPAWN", &script_SURFACEPARM_SPAWN );
        REGISTER_GLOBAL_VAR( "const uint32 SURFACEPARM_FLESH", &script_SURFACEPARM_FLESH );
        REGISTER_GLOBAL_VAR( "const uint32 SURFACEPARM_LAVA", &script_SURFACEPARM_LAVA );
        REGISTER_GLOBAL_VAR( "const uint32 SURFACEPARM_METAL", &script_SURFACEPARM_METAL );
        REGISTER_GLOBAL_VAR( "const uint32 SURFACEPARM_WOOD", &script_SURFACEPARM_WOOD );
        REGISTER_GLOBAL_VAR( "const uint32 SURFACEPARM_NODAMAGE", &script_SURFACEPARM_NODAMAGE );
        REGISTER_GLOBAL_VAR( "const uint32 SURFACEPARM_NODLIGHT", &script_SURFACEPARM_NODLIGHT );
        REGISTER_GLOBAL_VAR( "const uint32 SURFACEPARM_NOMARKS", &script_SURFACEPARM_NOMARKS );
        REGISTER_GLOBAL_VAR( "const uint32 SURFACEPARM_NOMISSILE", &script_SURFACEPARM_NOMISSILE );
        REGISTER_GLOBAL_VAR( "const uint32 SURFACEPARM_NOSTEPS", &script_SURFACEPARM_NOSTEPS );
        REGISTER_GLOBAL_VAR( "const uint32 SURFACEPARM_WATER", &script_SURFACEPARM_WATER );

        REGISTER_GLOBAL_VAR( "const int32 FS_INVALID_HANDLE", &script_FS_INVALID_HANDLE );
        REGISTER_GLOBAL_VAR( "const uint32 FS_OPEN_READ", &script_FS_OPEN_READ );
        REGISTER_GLOBAL_VAR( "const uint32 FS_OPEN_WRITE", &script_FS_OPEN_WRITE );
        REGISTER_GLOBAL_VAR( "const uint32 FS_OPEN_APPEND", &script_FS_OPEN_APPEND );

        REGISTER_GLOBAL_VAR( "const uint32 MAX_TOKEN_CHARS", &script_MAX_TOKEN_CHARS );
        REGISTER_GLOBAL_VAR( "const uint32 MAX_STRING_CHARS", &script_MAX_STRING_CHARS );
        REGISTER_GLOBAL_VAR( "const uint32 MAX_STRING_TOKENS", &script_MAX_STRING_TOKENS );
        REGISTER_GLOBAL_VAR( "const uint32 MAX_NPATH", &script_MAX_NPATH );
        REGISTER_GLOBAL_VAR( "const uint32 MAX_MAP_WIDTH", &script_MAX_MAP_WIDTH );
        REGISTER_GLOBAL_VAR( "const uint32 MAX_MAP_HEIGHT", &script_MAX_MAP_HEIGHT );

        REGISTER_GLOBAL_VAR( "const string COLOR_BLACK", &script_COLOR_BLACK );
        REGISTER_GLOBAL_VAR( "const string COLOR_RED", &script_COLOR_RED );
        REGISTER_GLOBAL_VAR( "const string COLOR_GREEN", &script_COLOR_GREEN );
        REGISTER_GLOBAL_VAR( "const string COLOR_YELLOW", &script_COLOR_YELLOW );
        REGISTER_GLOBAL_VAR( "const string COLOR_BLUE", &script_COLOR_BLUE );
        REGISTER_GLOBAL_VAR( "const string COLOR_CYAN", &script_COLOR_CYAN );
        REGISTER_GLOBAL_VAR( "const string COLOR_MAGENTA", &script_COLOR_MAGENTA );
        REGISTER_GLOBAL_VAR( "const string COLOR_WHITE", &script_COLOR_WHITE );
        REGISTER_GLOBAL_VAR( "const string COLOR_RESET", &script_COLOR_RESET );

        REGISTER_GLOBAL_VAR( "const int8 S_COLOR_BLACK", &script_S_COLOR_BLACK );
        REGISTER_GLOBAL_VAR( "const int8 S_COLOR_RED", &script_S_COLOR_RED );
        REGISTER_GLOBAL_VAR( "const int8 S_COLOR_GREEN", &script_S_COLOR_GREEN );
        REGISTER_GLOBAL_VAR( "const int8 S_COLOR_YELLOW", &script_S_COLOR_YELLOW );
        REGISTER_GLOBAL_VAR( "const int8 S_COLOR_BLUE", &script_S_COLOR_BLUE );
        REGISTER_GLOBAL_VAR( "const int8 S_COLOR_CYAN", &script_S_COLOR_CYAN );
        REGISTER_GLOBAL_VAR( "const int8 S_COLOR_MAGENTA", &script_S_COLOR_MAGENTA );
        REGISTER_GLOBAL_VAR( "const int8 S_COLOR_WHITE", &script_S_COLOR_WHITE );
        REGISTER_GLOBAL_VAR( "const int8 S_COLOR_RESET", &script_S_COLOR_RESET );

        REGISTER_GLOBAL_VAR( "const uint32 RSF_NOWORLDMODEL", &script_RSF_NORWORLDMODEL );
        REGISTER_GLOBAL_VAR( "const uint32 RSF_ORTHO_TYPE_CORDESIAN", &script_RSF_ORTHO_TYPE_CORDESIAN );
        REGISTER_GLOBAL_VAR( "const uint32 RSF_ORTHO_TYPE_WORLD", &script_RSF_ORTHO_TYPE_WORLD );
        REGISTER_GLOBAL_VAR( "const uint32 RSF_ORTHO_TYPE_SCREENSPACE", &script_RSF_ORTHO_TYPE_SCREENSPACE );

        REGISTER_GLOBAL_VAR( "const uint32 CVAR_CHEAT", &script_CVAR_CHEAT );
        REGISTER_GLOBAL_VAR( "const uint32 CVAR_ROM", &script_CVAR_ROM );
        REGISTER_GLOBAL_VAR( "const uint32 CVAR_INIT", &script_CVAR_INIT );
        REGISTER_GLOBAL_VAR( "const uint32 CVAR_LATCH", &script_CVAR_LATCH );
        REGISTER_GLOBAL_VAR( "const uint32 CVAR_NODEFAULT", &script_CVAR_NODEFAULT );
        REGISTER_GLOBAL_VAR( "const uint32 CVAR_NORESTART", &script_CVAR_NORESTART );
        REGISTER_GLOBAL_VAR( "const uint32 CVAR_NOTABCOMPLETE", &script_CVAR_NOTABCOMPLETE );
        REGISTER_GLOBAL_VAR( "const uint32 CVAR_PROTECTED", &script_CVAR_PROTECTED );
        REGISTER_GLOBAL_VAR( "const uint32 CVAR_TEMP", &script_CVAR_TEMP );
        REGISTER_GLOBAL_VAR( "const uint32 CVAR_SAVE", &script_CVAR_SAVE );

        REGISTER_GLOBAL_VAR( "const uint32 NOMAD_VERSION", &script_NOMAD_VERSION );
        REGISTER_GLOBAL_VAR( "const uint32 NOMAD_VERSION_UPDATE", &script_NOMAD_VERSION_UPDATE );
        REGISTER_GLOBAL_VAR( "const uint32 NOMAD_VERSION_PATCH", &script_NOMAD_VERSION_PATCH );
    }

    { // ModuleInfo
        REGISTER_GLOBAL_VAR( "const int32 MODULE_VERSION_MAJOR", g_pModuleLib->GetCurrentHandle()->VersionMajor() );
        REGISTER_GLOBAL_VAR( "const int32 MODULE_VERSION_UPDATE", g_pModuleLib->GetCurrentHandle()->VersionUpdate() );
        REGISTER_GLOBAL_VAR( "const int32 MODULE_VERSION_PATCH", g_pModuleLib->GetCurrentHandle()->VersionPatch() );
        REGISTER_GLOBAL_VAR( "const string MODULE_NAME", eastl::addressof( g_pModuleLib->GetCurrentHandle()->GetName() ) );
    }

    { // Math
        RESET_NAMESPACE(); // should this be defined at a global level?
        {
            REGISTER_OBJECT_TYPE( "vec2", vec2, asOBJ_VALUE );
            
            REGISTER_OBJECT_BEHAVIOUR( "vec2", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( vec2, ( void ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "vec2", asBEHAVE_CONSTRUCT, "void f( float )", WRAP_CON( vec2, ( float) ) );
            REGISTER_OBJECT_BEHAVIOUR( "vec2", asBEHAVE_CONSTRUCT, "void f( const vec2& in )", WRAP_CON( vec2, ( const vec2& ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "vec2", asBEHAVE_CONSTRUCT, "void f( float, float )", WRAP_CON( vec2, ( float, float ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "vec2", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( vec2 ) );

            REGISTER_OBJECT_PROPERTY( "vec2", "float x", offsetof( vec2, x ) );
            REGISTER_OBJECT_PROPERTY( "vec2", "float y", offsetof( vec2, y ) );

            REGISTER_METHOD_FUNCTION( "vec2", "vec2& opAssign( const vec2& in )", vec2, operator=, ( const vec2& ), vec2& );
            REGISTER_METHOD_FUNCTION( "vec2", "vec2& opAddAssign( const vec2& in )", vec2, operator+=, ( const vec2& ), vec2& );
            REGISTER_METHOD_FUNCTION( "vec2", "vec2& opSubAssign( const vec2& in )", vec2, operator-=, ( const vec2& ), vec2& );
            REGISTER_METHOD_FUNCTION( "vec2", "vec2& opMulAssign( const vec2& in )", vec2, operator*=, ( const vec2& ), vec2& );
            REGISTER_METHOD_FUNCTION( "vec2", "vec2& opDivAssign( const vec2& in )", vec2, operator/=, ( const vec2& ), vec2& );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec2", "bool opEquals( const vec2& in ) const", asFUNCTION( ModuleLib_EqualsVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec2", "int opCmp( const vec2& in ) const", asFUNCTION( ModuleLib_CmpVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec2", "float& opIndex( uint )", asFUNCTION( ModuleLib_IndexVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec2", "const float& opIndex( uint ) const", asFUNCTION( ModuleLib_IndexVec2Generic ), asCALL_GENERIC );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec2", "vec2 opAdd( const vec2& in ) const", asFUNCTION( ModuleLib_AddVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec2", "vec2 opSub( const vec2& in ) const", asFUNCTION( ModuleLib_SubVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec2", "vec2 opDiv( const vec2& in ) const", asFUNCTION( ModuleLib_DivVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec2", "vec2 opMul( const vec2& in ) const", asFUNCTION( ModuleLib_MulVec2Generic ), asCALL_GENERIC );
        }
        {
            REGISTER_OBJECT_TYPE( "vec3", vec3, asOBJ_VALUE );
            REGISTER_OBJECT_BEHAVIOUR( "vec3", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( vec3, ( void ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "vec3", asBEHAVE_CONSTRUCT, "void f( float )", WRAP_CON( vec3, ( float ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "vec3", asBEHAVE_CONSTRUCT, "void f( const vec3& in )", WRAP_CON( vec3, ( const vec3& ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "vec3", asBEHAVE_CONSTRUCT, "void f( float, float, float )", WRAP_CON( vec3, ( float, float, float ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "vec3", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( vec3 ) );
            REGISTER_OBJECT_PROPERTY( "vec3", "float x", offsetof( vec3, x ) );
            REGISTER_OBJECT_PROPERTY( "vec3", "float y", offsetof( vec3, y ) );
            REGISTER_OBJECT_PROPERTY( "vec3", "float z", offsetof( vec3, z ) );

            REGISTER_METHOD_FUNCTION( "vec3", "vec3& opAssign( const vec3& in )", vec3, operator=, ( const vec3& ), vec3& );
            REGISTER_METHOD_FUNCTION( "vec3", "vec3& opAddAssign( const vec3& in )", vec3, operator+=, ( const vec3& ), vec3& );
            REGISTER_METHOD_FUNCTION( "vec3", "vec3& opSubAssign( const vec3& in )", vec3, operator-=, ( const vec3& ), vec3& );
            REGISTER_METHOD_FUNCTION( "vec3", "vec3& opMulAssign( const vec3& in )", vec3, operator*=, ( const vec3& ), vec3& );
            REGISTER_METHOD_FUNCTION( "vec3", "vec3& opDivAssign( const vec3& in )", vec3, operator/=, ( const vec3& ), vec3& );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec3", "bool opEquals( const vec3& in ) const", asFUNCTION( ModuleLib_EqualsVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec3", "int opCmp( const vec3& in ) const", asFUNCTION( ModuleLib_CmpVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec3", "float& opIndex( uint )", asFUNCTION( ModuleLib_IndexVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec3", "const float& opIndex( uint ) const", asFUNCTION( ModuleLib_IndexVec3Generic ), asCALL_GENERIC );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec3", "vec3 opAdd( const vec3& in ) const", asFUNCTION( ModuleLib_AddVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec3", "vec3 opSub( const vec3& in ) const", asFUNCTION( ModuleLib_SubVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec3", "vec3 opDiv( const vec3& in ) const", asFUNCTION( ModuleLib_DivVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec3", "vec3 opMul( const vec3& in ) const", asFUNCTION( ModuleLib_MulVec3Generic ), asCALL_GENERIC );
        }
        {
            REGISTER_OBJECT_TYPE( "vec4", vec4, asOBJ_VALUE );
            REGISTER_OBJECT_BEHAVIOUR( "vec4", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( vec4, ( void ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "vec4", asBEHAVE_CONSTRUCT, "void f( float )", WRAP_CON( vec4, ( float ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "vec4", asBEHAVE_CONSTRUCT, "void f( const vec4& in )", WRAP_CON( vec4, ( const vec4& ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "vec4", asBEHAVE_CONSTRUCT, "void f( float, float, float, float )", WRAP_CON( vec4, ( float, float, float, float ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "vec4", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( vec4 ) );
            REGISTER_OBJECT_PROPERTY( "vec4", "float r", offsetof( vec4, r ) );
            REGISTER_OBJECT_PROPERTY( "vec4", "float g", offsetof( vec4, g ) );
            REGISTER_OBJECT_PROPERTY( "vec4", "float b", offsetof( vec4, b ) );
            REGISTER_OBJECT_PROPERTY( "vec4", "float a", offsetof( vec4, a ) );

            REGISTER_METHOD_FUNCTION( "vec4", "vec4& opAssign( const vec4& in )", vec4, operator=, ( const vec4& ), vec4& );
            REGISTER_METHOD_FUNCTION( "vec4", "vec4& opAddAssign( const vec4& in )", vec4, operator+=, ( const vec4& ), vec4& );
            REGISTER_METHOD_FUNCTION( "vec4", "vec4& opSubAssign( const vec4& in )", vec4, operator-=, ( const vec4& ), vec4& );
            REGISTER_METHOD_FUNCTION( "vec4", "vec4& opMulAssign( const vec4& in )", vec4, operator*=, ( const vec4& ), vec4& );
            REGISTER_METHOD_FUNCTION( "vec4", "vec4& opDivAssign( const vec4& in )", vec4, operator/=, ( const vec4& ), vec4& );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec4", "bool opEquals( const vec4& in ) const", asFUNCTION( ModuleLib_EqualsVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec4", "int opCmp( const vec4& in ) const", asFUNCTION( ModuleLib_CmpVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec4", "float& opIndex( uint )", asFUNCTION( ModuleLib_IndexVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec4", "const float& opIndex( uint ) const", asFUNCTION( ModuleLib_IndexVec4Generic ), asCALL_GENERIC );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec4", "vec4 opAdd( const vec4& in ) const", asFUNCTION( ModuleLib_AddVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec4", "vec4 opSub( const vec4& in ) const", asFUNCTION( ModuleLib_SubVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec4", "vec4 opDiv( const vec4& in ) const", asFUNCTION( ModuleLib_DivVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "vec4", "vec4 opMul( const vec4& in ) const", asFUNCTION( ModuleLib_MulVec4Generic ), asCALL_GENERIC );
        }

        {
            REGISTER_OBJECT_TYPE( "ivec2", ivec2, asOBJ_VALUE );
            
            REGISTER_OBJECT_BEHAVIOUR( "ivec2", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( ivec2, ( void ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "ivec2", asBEHAVE_CONSTRUCT, "void f( int )", WRAP_CON( ivec2, ( int ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "ivec2", asBEHAVE_CONSTRUCT, "void f( const ivec2& in )", WRAP_CON( ivec2, ( const ivec2& ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "ivec2", asBEHAVE_CONSTRUCT, "void f( int, int )", WRAP_CON( ivec2, ( int, int ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "ivec2", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( ivec2 ) );

            REGISTER_OBJECT_PROPERTY( "ivec2", "int x", offsetof( ivec2, x ) );
            REGISTER_OBJECT_PROPERTY( "ivec2", "int y", offsetof( ivec2, y ) );

            REGISTER_METHOD_FUNCTION( "ivec2", "ivec2& opAssign( const ivec2& in )", ivec2, operator=, ( const ivec2& ), ivec2& );
            REGISTER_METHOD_FUNCTION( "ivec2", "ivec2& opAddAssign( const ivec2& in )", ivec2, operator+=, ( const ivec2& ), ivec2& );
            REGISTER_METHOD_FUNCTION( "ivec2", "ivec2& opSubAssign( const ivec2& in )", ivec2, operator-=, ( const ivec2& ), ivec2& );
            REGISTER_METHOD_FUNCTION( "ivec2", "ivec2& opMulAssign( const ivec2& in )", ivec2, operator*=, ( const ivec2& ), ivec2& );
            REGISTER_METHOD_FUNCTION( "ivec2", "ivec2& opDivAssign( const ivec2& in )", ivec2, operator/=, ( const ivec2& ), ivec2& );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec2", "bool opEquals( const ivec2& in ) const", asFUNCTION( ModuleLib_EqualsIVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec2", "int opCmp( const ivec2& in ) const", asFUNCTION( ModuleLib_CmpIVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec2", "int& opIndex( uint )", asFUNCTION( ModuleLib_IndexIVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec2", "const int& opIndex( uint ) const", asFUNCTION( ModuleLib_IndexIVec2Generic ), asCALL_GENERIC );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec2", "ivec2 opAdd( const ivec2& in ) const", asFUNCTION( ModuleLib_AddIVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec2", "ivec2 opSub( const ivec2& in ) const", asFUNCTION( ModuleLib_SubIVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec2", "ivec2 opDiv( const ivec2& in ) const", asFUNCTION( ModuleLib_DivIVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec2", "ivec2 opMul( const ivec2& in ) const", asFUNCTION( ModuleLib_MulIVec2Generic ), asCALL_GENERIC );
        }
        {
            REGISTER_OBJECT_TYPE( "ivec3", ivec3, asOBJ_VALUE );
            REGISTER_OBJECT_BEHAVIOUR( "ivec3", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( ivec3, ( void ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "ivec3", asBEHAVE_CONSTRUCT, "void f( int )", WRAP_CON( ivec3, ( int ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "ivec3", asBEHAVE_CONSTRUCT, "void f( const ivec3& in )", WRAP_CON( ivec3, ( const ivec3& ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "ivec3", asBEHAVE_CONSTRUCT, "void f( int, int, int )", WRAP_CON( ivec3, ( int, int, int ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "ivec3", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( ivec3 ) );
            REGISTER_OBJECT_PROPERTY( "ivec3", "int x", offsetof( ivec3, x ) );
            REGISTER_OBJECT_PROPERTY( "ivec3", "int y", offsetof( ivec3, y ) );
            REGISTER_OBJECT_PROPERTY( "ivec3", "int z", offsetof( ivec3, z ) );

            REGISTER_METHOD_FUNCTION( "ivec3", "ivec3& opAssign( const ivec3& in )", ivec3, operator=, ( const ivec3& ), ivec3& );
            REGISTER_METHOD_FUNCTION( "ivec3", "ivec3& opAddAssign( const ivec3& in )", ivec3, operator+=, ( const ivec3& ), ivec3& );
            REGISTER_METHOD_FUNCTION( "ivec3", "ivec3& opSubAssign( const ivec3& in )", ivec3, operator-=, ( const ivec3& ), ivec3& );
            REGISTER_METHOD_FUNCTION( "ivec3", "ivec3& opMulAssign( const ivec3& in )", ivec3, operator*=, ( const ivec3& ), ivec3& );
            REGISTER_METHOD_FUNCTION( "ivec3", "ivec3& opDivAssign( const ivec3& in )", ivec3, operator/=, ( const ivec3& ), ivec3& );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec3", "bool opEquals( const ivec3& in ) const", asFUNCTION( ModuleLib_EqualsIVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec3", "int opCmp( const ivec3& in ) const", asFUNCTION( ModuleLib_CmpIVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec3", "int& opIndex( uint )", asFUNCTION( ModuleLib_IndexIVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec3", "const int& opIndex( uint ) const", asFUNCTION( ModuleLib_IndexIVec3Generic ), asCALL_GENERIC );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec3", "ivec3 opAdd( const ivec3& in ) const", asFUNCTION( ModuleLib_AddIVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec3", "ivec3 opSub( const ivec3& in ) const", asFUNCTION( ModuleLib_SubIVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec3", "ivec3 opDiv( const ivec3& in ) const", asFUNCTION( ModuleLib_DivIVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec3", "ivec3 opMul( const ivec3& in ) const", asFUNCTION( ModuleLib_MulIVec3Generic ), asCALL_GENERIC );
        }
        {
            REGISTER_OBJECT_TYPE( "ivec4", vec4, asOBJ_VALUE );
            REGISTER_OBJECT_BEHAVIOUR( "ivec4", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( ivec4, ( void ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "ivec4", asBEHAVE_CONSTRUCT, "void f( int )", WRAP_CON( ivec4, ( int ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "ivec4", asBEHAVE_CONSTRUCT, "void f( const ivec4& in )", WRAP_CON( ivec4, ( const ivec4& ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "ivec4", asBEHAVE_CONSTRUCT, "void f( int, int, int, int )", WRAP_CON( ivec4, ( int, int, int, int ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "ivec4", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( ivec4 ) );
            REGISTER_OBJECT_PROPERTY( "ivec4", "int r", offsetof( ivec4, r ) );
            REGISTER_OBJECT_PROPERTY( "ivec4", "int g", offsetof( ivec4, g ) );
            REGISTER_OBJECT_PROPERTY( "ivec4", "int b", offsetof( ivec4, b ) );
            REGISTER_OBJECT_PROPERTY( "ivec4", "int a", offsetof( ivec4, a ) );

            REGISTER_METHOD_FUNCTION( "ivec4", "ivec4& opAssign( const ivec4& in )", ivec4, operator=, ( const ivec4& ), ivec4& );
            REGISTER_METHOD_FUNCTION( "ivec4", "ivec4& opAddAssign( const ivec4& in )", ivec4, operator+=, ( const ivec4& ), ivec4& );
            REGISTER_METHOD_FUNCTION( "ivec4", "ivec4& opSubAssign( const ivec4& in )", ivec4, operator-=, ( const ivec4& ), ivec4& );
            REGISTER_METHOD_FUNCTION( "ivec4", "ivec4& opMulAssign( const ivec4& in )", ivec4, operator*=, ( const ivec4& ), ivec4& );
            REGISTER_METHOD_FUNCTION( "ivec4", "ivec4& opDivAssign( const ivec4& in )", ivec4, operator/=, ( const ivec4& ), ivec4& );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec4", "bool opEquals( const ivec4& in ) const", asFUNCTION( ModuleLib_EqualsIVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec4", "int opCmp( const ivec4& in ) const", asFUNCTION( ModuleLib_CmpIVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec4", "int& opIndex( uint )", asFUNCTION( ModuleLib_IndexIVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec4", "const int& opIndex( uint ) const", asFUNCTION( ModuleLib_IndexIVec4Generic ), asCALL_GENERIC );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec4", "ivec4 opAdd( const ivec4& in ) const", asFUNCTION( ModuleLib_AddIVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec4", "ivec4 opSub( const ivec4& in ) const", asFUNCTION( ModuleLib_SubIVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec4", "ivec4 opDiv( const ivec4& in ) const", asFUNCTION( ModuleLib_DivIVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "ivec4", "ivec4 opMul( const ivec4& in ) const", asFUNCTION( ModuleLib_MulIVec4Generic ), asCALL_GENERIC );
        }

        {
            REGISTER_OBJECT_TYPE( "uvec2", uvec2, asOBJ_VALUE );
            
            REGISTER_OBJECT_BEHAVIOUR( "uvec2", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( uvec2, ( void ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "uvec2", asBEHAVE_CONSTRUCT, "void f( uint )", WRAP_CON( uvec2, ( unsigned ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "uvec2", asBEHAVE_CONSTRUCT, "void f( const uvec2& in )", WRAP_CON( uvec2, ( const uvec2& ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "uvec2", asBEHAVE_CONSTRUCT, "void f( uint, uint )", WRAP_CON( uvec2, ( unsigned, unsigned ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "uvec2", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( uvec2 ) );

            REGISTER_OBJECT_PROPERTY( "uvec2", "uint x", offsetof( uvec2, x ) );
            REGISTER_OBJECT_PROPERTY( "uvec2", "uint y", offsetof( uvec2, y ) );

            REGISTER_METHOD_FUNCTION( "uvec2", "uvec2& opAssign( const uvec2& in )", uvec2, operator=, ( const uvec2& ), uvec2& );
            REGISTER_METHOD_FUNCTION( "uvec2", "uvec2& opAddAssign( const uvec2& in )", uvec2, operator+=, ( const uvec2& ), uvec2& );
            REGISTER_METHOD_FUNCTION( "uvec2", "uvec2& opSubAssign( const uvec2& in )", uvec2, operator-=, ( const uvec2& ), uvec2& );
            REGISTER_METHOD_FUNCTION( "uvec2", "uvec2& opMulAssign( const uvec2& in )", uvec2, operator*=, ( const uvec2& ), uvec2& );
            REGISTER_METHOD_FUNCTION( "uvec2", "uvec2& opDivAssign( const uvec2& in )", uvec2, operator/=, ( const uvec2& ), uvec2& );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec2", "bool opEquals( const uvec2& in ) const", asFUNCTION( ModuleLib_EqualsUVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec2", "int opCmp( const uvec2& in ) const", asFUNCTION( ModuleLib_CmpUVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec2", "uint& opIndex( uint )", asFUNCTION( ModuleLib_IndexUVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec2", "const uint& opIndex( uint ) const", asFUNCTION( ModuleLib_IndexUVec2Generic ), asCALL_GENERIC );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec2", "uvec2 opAdd( const uvec2& in ) const", asFUNCTION( ModuleLib_AddUVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec2", "uvec2 opSub( const uvec2& in ) const", asFUNCTION( ModuleLib_SubUVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec2", "uvec2 opDiv( const uvec2& in ) const", asFUNCTION( ModuleLib_DivUVec2Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec2", "uvec2 opMul( const uvec2& in ) const", asFUNCTION( ModuleLib_MulUVec2Generic ), asCALL_GENERIC );
        }
        {
            REGISTER_OBJECT_TYPE( "uvec3", uvec3, asOBJ_VALUE );
            REGISTER_OBJECT_BEHAVIOUR( "uvec3", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( uvec3, ( void ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "uvec3", asBEHAVE_CONSTRUCT, "void f( uint )", WRAP_CON( uvec3, ( unsigned ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "uvec3", asBEHAVE_CONSTRUCT, "void f( const uvec3& in )", WRAP_CON( uvec3, ( const uvec3& ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "uvec3", asBEHAVE_CONSTRUCT, "void f( uint, uint, uint )", WRAP_CON( uvec3, ( unsigned, unsigned, unsigned ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "uvec3", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( uvec3 ) );
            REGISTER_OBJECT_PROPERTY( "uvec3", "uint x", offsetof( uvec3, x ) );
            REGISTER_OBJECT_PROPERTY( "uvec3", "uint y", offsetof( uvec3, y ) );
            REGISTER_OBJECT_PROPERTY( "uvec3", "uint z", offsetof( uvec3, z ) );

            REGISTER_METHOD_FUNCTION( "uvec3", "uvec3& opAssign( const uvec3& in )", uvec3, operator=, ( const uvec3& ), uvec3& );
            REGISTER_METHOD_FUNCTION( "uvec3", "uvec3& opAddAssign( const uvec3& in )", uvec3, operator+=, ( const uvec3& ), uvec3& );
            REGISTER_METHOD_FUNCTION( "uvec3", "uvec3& opSubAssign( const uvec3& in )", uvec3, operator-=, ( const uvec3& ), uvec3& );
            REGISTER_METHOD_FUNCTION( "uvec3", "uvec3& opMulAssign( const uvec3& in )", uvec3, operator*=, ( const uvec3& ), uvec3& );
            REGISTER_METHOD_FUNCTION( "uvec3", "uvec3& opDivAssign( const uvec3& in )", uvec3, operator/=, ( const uvec3& ), uvec3& );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec3", "bool opEquals( const uvec3& in ) const", asFUNCTION( ModuleLib_EqualsUVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec3", "int opCmp( const uvec3& in ) const", asFUNCTION( ModuleLib_CmpUVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec3", "uint& opIndex( uint )", asFUNCTION( ModuleLib_IndexUVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec3", "const uint& opIndex( uint ) const", asFUNCTION( ModuleLib_IndexUVec3Generic ), asCALL_GENERIC );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec3", "uvec3 opAdd( const uvec3& in ) const", asFUNCTION( ModuleLib_AddUVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec3", "uvec3 opSub( const uvec3& in ) const", asFUNCTION( ModuleLib_SubUVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec3", "uvec3 opDiv( const uvec3& in ) const", asFUNCTION( ModuleLib_DivUVec3Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec3", "uvec3 opMul( const uvec3& in ) const", asFUNCTION( ModuleLib_MulUVec3Generic ), asCALL_GENERIC );
        }
        {
            REGISTER_OBJECT_TYPE( "uvec4", uvec4, asOBJ_VALUE );
            REGISTER_OBJECT_BEHAVIOUR( "uvec4", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( uvec4, ( void ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "uvec4", asBEHAVE_CONSTRUCT, "void f( uint )", WRAP_CON( uvec4, ( unsigned ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "uvec4", asBEHAVE_CONSTRUCT, "void f( const uvec4& in )", WRAP_CON( uvec4, ( const uvec4& ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "uvec4", asBEHAVE_CONSTRUCT, "void f( uint, uint, uint, uint )", WRAP_CON( uvec4, ( unsigned, unsigned, unsigned, unsigned ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "uvec4", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( uvec4 ) );
            REGISTER_OBJECT_PROPERTY( "uvec4", "uint r", offsetof( uvec4, r ) );
            REGISTER_OBJECT_PROPERTY( "uvec4", "uint g", offsetof( uvec4, g ) );
            REGISTER_OBJECT_PROPERTY( "uvec4", "uint b", offsetof( uvec4, b ) );
            REGISTER_OBJECT_PROPERTY( "uvec4", "uint a", offsetof( uvec4, a ) );

            REGISTER_METHOD_FUNCTION( "uvec4", "uvec4& opAssign( const uvec4& in )", uvec4, operator=, ( const uvec4& ), uvec4& );
            REGISTER_METHOD_FUNCTION( "uvec4", "uvec4& opAddAssign( const uvec4& in )", uvec4, operator+=, ( const uvec4& ), uvec4& );
            REGISTER_METHOD_FUNCTION( "uvec4", "uvec4& opSubAssign( const uvec4& in )", uvec4, operator-=, ( const uvec4& ), uvec4& );
            REGISTER_METHOD_FUNCTION( "uvec4", "uvec4& opMulAssign( const uvec4& in )", uvec4, operator*=, ( const uvec4& ), uvec4& );
            REGISTER_METHOD_FUNCTION( "uvec4", "uvec4& opDivAssign( const uvec4& in )", uvec4, operator/=, ( const uvec4& ), uvec4& );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec4", "bool opEquals( const uvec4& in ) const", asFUNCTION( ModuleLib_EqualsUVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec4", "int opCmp( const uvec4& in ) const", asFUNCTION( ModuleLib_CmpUVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec4", "uint& opIndex( uint )", asFUNCTION( ModuleLib_IndexUVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec4", "const uint& opIndex( uint ) const", asFUNCTION( ModuleLib_IndexUVec4Generic ), asCALL_GENERIC );

            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec4", "uvec4 opAdd( const uvec4& in ) const", asFUNCTION( ModuleLib_AddUVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec4", "uvec4 opSub( const uvec4& in ) const", asFUNCTION( ModuleLib_SubUVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec4", "uvec4 opDiv( const uvec4& in ) const", asFUNCTION( ModuleLib_DivUVec4Generic ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterObjectMethod( "uvec4", "uvec4 opMul( const uvec4& in ) const", asFUNCTION( ModuleLib_MulUVec4Generic ), asCALL_GENERIC );
        }
    }

    SET_NAMESPACE( "ImGui" );
    { // ImGui
        #undef REGISTER_GLOBAL_FUNCTION
        #define REGISTER_GLOBAL_FUNCTION( decl, funcPtr, params, returnType ) \
            ValidateFunction( __func__, decl,\
                g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction( decl, WRAP_FN_PR( funcPtr, params, returnType ), asCALL_GENERIC ) )
        
        RESET_NAMESPACE();

        REGISTER_ENUM_TYPE( "ImGuiWindowFlags" );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "NoTitleBar", ImGuiWindowFlags_NoTitleBar );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "NoResize", ImGuiWindowFlags_NoResize );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "NoMouseInputs", ImGuiWindowFlags_NoMouseInputs );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "NoMove", ImGuiWindowFlags_NoMove );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "NoCollapse", ImGuiWindowFlags_NoCollapse );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "NoBringToFrontOnFocus", ImGuiWindowFlags_NoBringToFrontOnFocus );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "NoDecoration", ImGuiWindowFlags_NoDecoration );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "NoBackground", ImGuiWindowFlags_NoBackground );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "NoSavedSettings", ImGuiWindowFlags_NoSavedSettings );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "NoScrollbar", ImGuiWindowFlags_NoScrollbar );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "AlwaysHorizontalScrollbar", ImGuiWindowFlags_AlwaysHorizontalScrollbar );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "AlwaysVerticalScrollbar", ImGuiWindowFlags_AlwaysVerticalScrollbar );
        REGISTER_ENUM_VALUE( "ImGuiWindowFlags", "AlwaysAutoResize", ImGuiWindowFlags_AlwaysAutoResize );

        REGISTER_ENUM_TYPE( "ImGuiTableFlags" );
        REGISTER_ENUM_VALUE( "ImGuiTableFlags", "None", ImGuiTableFlags_None );
        REGISTER_ENUM_VALUE( "ImGuiTableFlags", "Resizable", ImGuiTableFlags_Resizable );
        REGISTER_ENUM_VALUE( "ImGuiTableFlags", "Reorderable", ImGuiTableFlags_Reorderable );

        REGISTER_ENUM_TYPE( "ImGuiInputTextFlags" );
        REGISTER_ENUM_VALUE( "ImGuiInputTextFlags", "None", ImGuiInputTextFlags_None );
        REGISTER_ENUM_VALUE( "ImGuiInputTextFlags", "EnterReturnsTrue", ImGuiInputTextFlags_EnterReturnsTrue );
        REGISTER_ENUM_VALUE( "ImGuiInputTextFlags", "AllowTabInput", ImGuiInputTextFlags_AllowTabInput );
        REGISTER_ENUM_VALUE( "ImGuiInputTextFlags", "CtrlEnterForNewLine", ImGuiInputTextFlags_CtrlEnterForNewLine );

        REGISTER_ENUM_TYPE( "ImGuiDir" );
        REGISTER_ENUM_VALUE( "ImGuiDir", "Left", ImGuiDir_Left );
        REGISTER_ENUM_VALUE( "ImGuiDir", "Right", ImGuiDir_Right );
        REGISTER_ENUM_VALUE( "ImGuiDir", "Down", ImGuiDir_Down );
        REGISTER_ENUM_VALUE( "ImGuiDir", "Up", ImGuiDir_Up );
        REGISTER_ENUM_VALUE( "ImGuiDir", "None", ImGuiDir_None );

        REGISTER_ENUM_TYPE( "ImGuiCol" );
        REGISTER_ENUM_VALUE( "ImGuiCol", "Border", ImGuiCol_Border );
        REGISTER_ENUM_VALUE( "ImGuiCol", "Button", ImGuiCol_Button );
        REGISTER_ENUM_VALUE( "ImGuiCol", "ButtonActive", ImGuiCol_ButtonActive );
        REGISTER_ENUM_VALUE( "ImGuiCol", "ButtonHovered", ImGuiCol_ButtonHovered );
        REGISTER_ENUM_VALUE( "ImGuiCol", "WindowBg", ImGuiCol_WindowBg );
        REGISTER_ENUM_VALUE( "ImGuiCol", "MenuBarBg", ImGuiCol_MenuBarBg );
        REGISTER_ENUM_VALUE( "ImGuiCol", "FrameBg", ImGuiCol_FrameBg );
        REGISTER_ENUM_VALUE( "ImGuiCol", "FrameBgActive", ImGuiCol_FrameBgActive );
        REGISTER_ENUM_VALUE( "ImGuiCol", "FrameBgHovered", ImGuiCol_FrameBgHovered );
        REGISTER_ENUM_VALUE( "ImGuiCol", "CheckMark", ImGuiCol_CheckMark );
        REGISTER_ENUM_VALUE( "ImGuiCol", "PopupBg", ImGuiCol_PopupBg );
        REGISTER_ENUM_VALUE( "ImGuiCol", "ScrollbarBg", ImGuiCol_ScrollbarBg );
        REGISTER_ENUM_VALUE( "ImGuiCol", "ScrollbarGrab", ImGuiCol_ScrollbarGrab );
        REGISTER_ENUM_VALUE( "ImGuiCol", "ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive );
        REGISTER_ENUM_VALUE( "ImGuiCol", "ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered );

        // this isn't currently supported
        CheckASCall( g_pModuleLib->GetScriptEngine()->RegisterFuncdef( "int ImGuiInputTextCalback( ref@ )" ) );

        SET_NAMESPACE( "ImGui" );

        REGISTER_GLOBAL_FUNCTION( "bool ImGui::Begin( const string& in, ref@, ImGuiWindowFlags = ImGuiWindowFlags::None )", ImGui_Begin, ( const string_t *, bool *, ImGuiWindowFlags ), bool );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::End()", ImGui_End, ( void ), void );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::SetWindowSize( const vec2& in )", ImGui_SetWindowSize, ( const vec2 * ), void );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::SetWindowPos( const vec2& in )", ImGui_SetWindowPos, ( const vec2 * ), void );
        REGISTER_GLOBAL_FUNCTION( "bool ImGui::BeginTable( const string& in, int, ImGuiTableFlags = ImGuiTableFlags::None )", ImGui_BeginTable, ( const string_t *, int, ImGuiTableFlags ), bool );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::EndTable()", ImGui_EndTable, ( void ), void );
        REGISTER_GLOBAL_FUNCTION( "bool ImGui::InputText( const string& in, string& out, ImGuiInputTextFlags = ImGuiInputTextFlags::None )", ImGui_InputText, ( const string_t *, string_t *, ImGuiInputTextFlags ), bool );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::TableNextColumn()", ImGui_TableNextColumn, ( void ), void );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::TableNextRow()", ImGui_TableNextRow, ( void ), void );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::PushStyleColor( ImGuiCol, const vec4& in )", ImGui_PushStyleColor_V4, ( ImGuiCol, const vec4 * ), void );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::PushStyleColor( ImGuiCol, const uint32 )", ImGui_PushStyleColor_U32, ( ImGuiCol, const ImU32 ), void );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::PopStyleColor( int = 1 )", ImGui_PopStyleColor, ( int ), void );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::Text( const string& in )", ImGui_Text, ( const string_t * ), void );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::TextColored( const vec4& in, const string& in )", ImGui_TextColored, ( const vec4 *, const string_t * ), void );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::SameLine( float = 0.0f, float = -1.0f )", ImGui::SameLine, ( float, float ), void );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::NewLine()", ImGui::NewLine, ( void ), void );
        REGISTER_GLOBAL_FUNCTION( "bool ImGui::ArrowButton( const string& in, ImGuiDir )", ImGui_ArrowButton, ( const string_t *, ImGuiDir ), bool );
        REGISTER_GLOBAL_FUNCTION( "bool ImGui::RadioButton( const string& in, bool )", ImGui_RadioButton, ( const string_t *, bool ), bool );
        g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction(
            "bool ImGui::SliderInt( const string& in, int& out, int, int, int = 0 )", asFUNCTION( ImGui_SliderInt ),
            asCALL_GENERIC
        );
        g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction(
            "bool ImGui::SliderFloat( const string& in, float& out, float, float, int = 0 )", asFUNCTION( ImGui_SliderFloat ),
            asCALL_GENERIC
        );
        g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction(
            "bool ImGui::SliderVec2( const string& in, vec2& out, float, float, int = 0 )", asFUNCTION( ImGui_SliderFloat2 ),
            asCALL_GENERIC
        );
        g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction(
            "bool ImGui::SliderVec3( const string& in, vec3& out, float, float, int = 0 )", asFUNCTION( ImGui_SliderFloat3 ),
            asCALL_GENERIC
        );
        g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction(
            "bool ImGui::SliderVec4( const string& in, vec4& out, float, float, int = 0 )", asFUNCTION( ImGui_SliderFloat4 ),
            asCALL_GENERIC
        );
        g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction(
            "bool ImGui::SliderAngle( const string& in, float& out, float = -360.0f, float = 360.0f, int = 0 )", asFUNCTION( ImGui_SliderAngle ),
            asCALL_GENERIC
        );
        REGISTER_GLOBAL_FUNCTION( "bool ImGui::ColorEdit3( const string& in, vec3& in, int = 0 )", ImGui_ColorEdit3, ( const string_t *, vec3 *, int ), bool );
        REGISTER_GLOBAL_FUNCTION( "bool ImGui::ColorEdit4( const string& in, vec4& in, int = 0 )", ImGui_ColorEdit4, ( const string_t *, vec4 *, int ), bool );
        REGISTER_GLOBAL_FUNCTION( "bool ImGui::Button( const string& in, const vec2& in = vec2( 0.0f ) )", ImGui_Button, ( const string_t *, const vec2 * ), bool );

        REGISTER_GLOBAL_FUNCTION( "bool ImGui::BeginCombo( const string& in, const string& in )", ImGui_BeginCombo, ( const string_t *, const string_t * ), bool );
        REGISTER_GLOBAL_FUNCTION( "bool ImGui::Selectable( const string& in, bool = false, int = 0, const vec2& in = vec2( 0.0f ) )", ImGui_Selectable, ( const string_t *, bool, int, const vec2 * ), bool );
        REGISTER_GLOBAL_FUNCTION( "void ImGui::EndCombo()", ImGui_EndCombo, ( void ), void );

        #undef REGISTER_GLOBAL_FUNCTION
        #define REGISTER_GLOBAL_FUNCTION( decl, funcPtr ) \
            ValidateFunction( __func__, decl,\
                g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction( decl, asFUNCTION( ModuleLib_##funcPtr ), asCALL_GENERIC ) )
    }
    RESET_NAMESPACE();

    SET_NAMESPACE( "TheNomad" );
	
	SET_NAMESPACE( "TheNomad::Engine" );
	{ // Engine
        
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::CvarRegister( const string& in, const string& in, uint, int64& out, float& out, int& out, int& out )", CvarRegisterGeneric );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::CvarSet( const string& in, const string& in )", CvarSetValueGeneric );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::CvarUpdate( string& out, int64& out, float& out, int& out, const int )", CvarUpdateGeneric );
        REGISTER_GLOBAL_FUNCTION( "int64 TheNomad::Engine::CvarVariableInteger( const string& in )", CvarVariableInteger );
        REGISTER_GLOBAL_FUNCTION( "float TheNomad::Engine::CvarVariableFloat( const string& in )", CvarVariableFloat );
        REGISTER_GLOBAL_FUNCTION( "string TheNomad::Engine::CvarVariableString( const string& in )", CvarVariableString );

        REGISTER_GLOBAL_FUNCTION( "uint TheNomad::Engine::CmdArgc()", CmdArgcGeneric );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::CmdArgvFixed( int8[]& in, uint, uint )", CmdArgvFixedGeneric );
        REGISTER_GLOBAL_FUNCTION( "const string& TheNomad::Engine::CmdArgv( uint )", CmdArgvGeneric );
        REGISTER_GLOBAL_FUNCTION( "const string& TheNomad::Engine::CmdArgs( uint )", CmdArgsGeneric );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::CmdAddCommand( const string& in )", CmdAddCommandGeneric );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::CmdRemoveCommand( const string& in )", CmdRemoveCommandGeneric );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::CmdExecuteCommand( const string& in )", CmdExecuteCommandGeneric );

        REGISTER_ENUM_TYPE( "KeyNum" );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_A", KEY_A );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_B", KEY_B );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_C", KEY_C );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_D", KEY_D );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_E", KEY_E );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_F", KEY_F );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_G", KEY_G );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_H", KEY_H );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_I", KEY_I );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_J", KEY_J );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_K", KEY_K );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_L", KEY_L );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_M", KEY_M );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_N", KEY_N );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_O", KEY_O );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_P", KEY_P );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_Q", KEY_Q );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_R", KEY_R );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_S", KEY_S );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_T", KEY_T );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_U", KEY_U );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_V", KEY_V );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_W", KEY_W );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_X", KEY_X );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_Y", KEY_Y );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_Z", KEY_Z );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_Tab", KEY_TAB );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_Space", KEY_SPACE );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_BackSpace", KEY_BACKSPACE );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_Alt", KEY_ALT );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_UpArrow", KEY_UP );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_LeftArrow", KEY_LEFT );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_DownArrow", KEY_DOWN );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_RightArrow", KEY_RIGHT );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_Up", KEY_UP );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_Left", KEY_LEFT );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_Down", KEY_DOWN );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_Right", KEY_RIGHT );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_BackSlash", KEY_BACKSLASH );
        REGISTER_ENUM_VALUE( "KeyNum", "Key_Slash", KEY_SLASH );

        REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::IsAnyKeyDown()", IsAnyKeyDown );
        REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::IsKeyDown( KeyNum )", KeyIsDown );

        REGISTER_OBJECT_TYPE( "Timer", CTimer, asOBJ_VALUE );
        REGISTER_OBJECT_BEHAVIOUR( "TheNomad::Engine::Timer", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( CTimer, ( void ) ) );
        REGISTER_OBJECT_BEHAVIOUR( "TheNomad::Engine::Timer", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( CTimer ) );

        REGISTER_METHOD_FUNCTION( "TheNomad::Engine::Timer", "void Run()", CTimer, Run, ( void ), void );
        REGISTER_METHOD_FUNCTION( "TheNomad::Engine::Timer", "void Stop()", CTimer, Stop, ( void ), void );
        REGISTER_METHOD_FUNCTION( "TheNomad::Engine::Timer", "int64 ElapsedMilliseconds() const", CTimer, ElapsedMilliseconds_ML, ( void ) const, int64_t );
        REGISTER_METHOD_FUNCTION( "TheNomad::Engine::Timer", "int64 ElapsedSeconds() const", CTimer, ElapsedSeconds_ML, ( void ) const, int64_t );
        REGISTER_METHOD_FUNCTION( "TheNomad::Engine::Timer", "int32 ElapsedMinutes() const", CTimer, ElapsedMinutes_ML, ( void ) const, int32_t );

		{ // SoundSystem
			SET_NAMESPACE( "TheNomad::Engine::SoundSystem" );
			
			// could this be a class, YES, but I won't force it on the modder
			
			REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::SoundSystem::PlaySfx( int )", Snd_PlaySfx );
			REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::SoundSystem::SetLoopingTrack( int )", Snd_SetLoopingTrack );
			REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::SoundSystem::ClearLoopingTrack()", Snd_ClearLoopingTrack );
			REGISTER_GLOBAL_FUNCTION( "int TheNomad::Engine::SoundSystem::RegisterSfx( const string& in )", Snd_RegisterSfx );
			REGISTER_GLOBAL_FUNCTION( "int TheNomad::Engine::SoundSystem::RegisterTrack( const string& in )", Snd_RegisterTrack );

            RESET_NAMESPACE();
		}
		{ // FileSystem
			SET_NAMESPACE( "TheNomad::Engine::FileSystem" );
			
			// could this be a class, YES, but I won't force it on the modder
			
			REGISTER_GLOBAL_FUNCTION( "int TheNomad::Engine::FileSystem::OpenFileRead( const string& in )", OpenFileRead );
			REGISTER_GLOBAL_FUNCTION( "int TheNomad::Engine::FileSystem::OpenFileWrite( const string& in )", OpenFileWrite );
			REGISTER_GLOBAL_FUNCTION( "int TheNomad::Engine::FileSystem::OpenFileAppend( const string& in )", OpenFileAppend );
			REGISTER_GLOBAL_FUNCTION( "uint64 TheNomad::Engine::FileSystem::OpenFile( const string& in, int, int& out )", OpenFile );
			REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::FileSystem::CloseFile( int )", CloseFile );
			REGISTER_GLOBAL_FUNCTION( "uint64 TheNomad::Engine::FileSystem::GetLength( int )", GetFileLength );
			REGISTER_GLOBAL_FUNCTION( "uint64 TheNomad::Engine::FileSystem::GetPosition( int )", GetFilePosition );
            REGISTER_GLOBAL_FUNCTION( "uint64 TheNomad::Engine::FileSystem::SetPosition( int, uint64, uint )", FileSetPosition );
			REGISTER_GLOBAL_FUNCTION( "uint64 TheNomad::Engine::FileSystem::LoadFile( const string& in, array<int8>& out )", LoadFile );
            REGISTER_GLOBAL_FUNCTION( "uint64 TheNomad::Engine::FileSystem::LoadFile( const string& in, string& out )", LoadFileString );

            {
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteInt8( char, int )", WriteInt8 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteInt16( int16, int )", WriteInt16 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteInt32( int32, int )", WriteInt32 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteInt64( int64, int )", WriteInt64 );

                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteUInt8( char, int )", WriteUInt8 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteUInt16( uint16, int )", WriteUInt16 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteUInt32( uint32, int )", WriteUInt32 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteUInt64( uint64, int )", WriteUInt64 );

                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteChar( char, int )", WriteInt8 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteShort( int16, int )", WriteInt16 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteInt( int32, int )", WriteInt32 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteLong( int64, int )", WriteInt64 );

                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteByte( char, int )", WriteUInt8 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteUShort( uint16, int )", WriteUInt16 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteUInt( uint32, int )", WriteUInt32 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteULong( uint64, int )", WriteUInt64 );

                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::WriteString( const string& in, int )", WriteString );
            }
            {
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadInt8( char, int )", ReadInt8 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadInt16( int16, int )", ReadInt16 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadInt32( int32, int )", ReadInt32 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadInt64( int64, int )", ReadInt64 );

                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadUInt8( char, int )", ReadUInt8 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadUInt16( uint16, int  )", ReadUInt16 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadUInt32( uint32, int  )", ReadUInt32 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadUInt64( uint64, int  )", ReadUInt64 );

                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadChar( char, int )", ReadInt8 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadShort( int16, int )", ReadInt16 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadInt( int32, int )", ReadInt32 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadLong( int64, int )", ReadInt64 );

                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadByte( char, int )", ReadUInt8 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadUShort( uint16, int )", ReadUInt16 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadUInt( uint32, int )", ReadUInt32 );
                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadULong( uint64, int )", ReadUInt64 );

                REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Engine::FileSystem::ReadString( string& out, int )", ReadString );
            }

            RESET_NAMESPACE();
		}
        { // Renderer
            SET_NAMESPACE( "TheNomad::Engine::Renderer" );

            REGISTER_OBJECT_TYPE( "GPUConfig", CModuleGPUConfig, asOBJ_VALUE );

            REGISTER_OBJECT_BEHAVIOUR( "TheNomad::Engine::Renderer::GPUConfig", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( CModuleGPUConfig, ( void ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "TheNomad::Engine::Renderer::GPUConfig", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( CModuleGPUConfig ) );

            REGISTER_METHOD_FUNCTION( "TheNomad::Engine::Renderer::GPUConfig", "TheNomad::Engine::Renderer::GPUConfig& opAssign( const TheNomad::Engine::Renderer::GPUConfig& in )", CModuleGPUConfig, operator=, ( const CModuleGPUConfig& ), CModuleGPUConfig& );

            REGISTER_OBJECT_PROPERTY( "TheNomad::Engine::Renderer::GPUConfig", "string vendor", offsetof( CModuleGPUConfig, vendorString ) );
            REGISTER_OBJECT_PROPERTY( "TheNomad::Engine::Renderer::GPUConfig", "string renderer", offsetof( CModuleGPUConfig, rendererString ) );
            REGISTER_OBJECT_PROPERTY( "TheNomad::Engine::Renderer::GPUConfig", "string extensions", offsetof( CModuleGPUConfig, extensionsString ) );
            REGISTER_OBJECT_PROPERTY( "TheNomad::Engine::Renderer::GPUConfig", "string version", offsetof( CModuleGPUConfig, versionString ) );
            REGISTER_OBJECT_PROPERTY( "TheNomad::Engine::Renderer::GPUConfig", "string shaderVersion", offsetof( CModuleGPUConfig, shaderVersionString ) );
            REGISTER_OBJECT_PROPERTY( "TheNomad::Engine::Renderer::GPUConfig", "uint32 screenWidth", offsetof( gpuConfig_t, vidWidth ) );
            REGISTER_OBJECT_PROPERTY( "TheNomad::Engine::Renderer::GPUConfig", "uint32 screenHeight", offsetof( gpuConfig_t, vidHeight ) );
            REGISTER_OBJECT_PROPERTY( "TheNomad::Engine::Renderer::GPUConfig", "bool isFullscreen", offsetof( gpuConfig_t, isFullscreen ) );

            REGISTER_OBJECT_TYPE( "PolyVert", CModulePolyVert, asOBJ_VALUE );

            REGISTER_OBJECT_BEHAVIOUR( "TheNomad::Engine::Renderer::PolyVert", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( CModulePolyVert, ( void ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "TheNomad::Engine::Renderer::PolyVert", asBEHAVE_CONSTRUCT,
                "void f( const TheNomad::Engine::Renderer::PolyVert& in )", WRAP_CON( CModulePolyVert, ( const CModulePolyVert& ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "TheNomad::Engine::Renderer::PolyVert", asBEHAVE_CONSTRUCT,
                "void f( const vec3& in, const vec3& in, const vec2& in, uint32 )", WRAP_CON( CModulePolyVert, ( const vec3&, const vec3&, const vec2&, const color4ub_t& ) ) );
            REGISTER_OBJECT_BEHAVIOUR( "TheNomad::Engine::Renderer::PolyVert", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( CModulePolyVert ) );
            
            REGISTER_METHOD_FUNCTION( "TheNomad::Engine::Renderer::PolyVert", "TheNomad::Engine::Renderer::PolyVert& opAssign( const TheNomad::Engine::Renderer::PolyVert& in )",
                CModulePolyVert, operator=, ( const CModulePolyVert& ), CModulePolyVert& );
            REGISTER_METHOD_FUNCTION( "TheNomad::Engine::Renderer::PolyVert", "void Set( const TheNomad::Engine::Renderer::PolyVert& in )",
                CModulePolyVert, Set, ( const CModulePolyVert& ), void );
            REGISTER_METHOD_FUNCTION( "TheNomad::Engine::Renderer::PolyVert", "void Set( const vec3& in, const vec3& in, const vec2& in, uint32 )",
                CModulePolyVert, Set, ( const vec3&, const vec3&, const vec2&, const color4ub_t& ), void );
            
            REGISTER_OBJECT_PROPERTY( "TheNomad::Engine::Renderer::PolyVert", "vec3 xyz", offsetof( CModulePolyVert, m_Origin ) );
            REGISTER_OBJECT_PROPERTY( "TheNomad::Engine::Renderer::PolyVert", "vec3 worldPos", offsetof( CModulePolyVert, m_WorldPos ) );
            REGISTER_OBJECT_PROPERTY( "TheNomad::Engine::Renderer::PolyVert", "vec2 uv", offsetof( CModulePolyVert, m_TexCoords ) );
            REGISTER_OBJECT_PROPERTY( "TheNomad::Engine::Renderer::PolyVert", "uint32 color", offsetof( CModulePolyVert, m_Color ) );

            REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::Renderer::ClearScene()", ClearScene );
            REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::Renderer::RenderScene( uint, uint, uint, uint, uint, uint )", RenderScene );
//            REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::Renderer::RE_AddEntityToScene( int, vec3, uint,  )", AddEntityToScene );
            REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::Renderer::AddPolyToScene( int, const array<TheNomad::Engine::Renderer::PolyVert>& in )", AddPolyToScene );
            REGISTER_GLOBAL_FUNCTION( "void TheNomad::Engine::Renderer::AddSpriteToScene( const vec3& in, int, int )", AddSpriteToScene );
            REGISTER_GLOBAL_FUNCTION( "int TheNomad::Engine::Renderer::RegisterShader( const string& in )", RegisterShader );
            REGISTER_GLOBAL_FUNCTION( "int TheNomad::Engine::Renderer::RegisterSpriteSheet( const string& in, uint, uint, uint, uint )", RegisterSpriteSheet );

            RESET_NAMESPACE();
        }
	}
    
	SET_NAMESPACE( "TheNomad::GameSystem" );
	{
        REGISTER_OBJECT_TYPE( "BBox", CModuleBoundBox, asOBJ_VALUE );
        REGISTER_OBJECT_BEHAVIOUR( "TheNomad::GameSystem::BBox", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( CModuleBoundBox, ( void ) ) );
        REGISTER_OBJECT_BEHAVIOUR( "TheNomad::GameSystem::BBox", asBEHAVE_CONSTRUCT, "void f( float, float, const vec3& in )",
            WRAP_CON( CModuleBoundBox, ( float, float, const glm::vec3& ) ) );
        REGISTER_OBJECT_BEHAVIOUR( "TheNomad::GameSystem::BBox", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( CModuleBoundBox ) );
        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::BBox", "float m_nWidth", offsetof( CModuleBoundBox, width ) );
        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::BBox", "float m_nHeight", offsetof( CModuleBoundBox, height ) );
        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::BBox", "vec3 m_Mins", offsetof( CModuleBoundBox, mins ) );
        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::BBox", "vec3 m_Maxs", offsetof( CModuleBoundBox, maxs ) );
        REGISTER_METHOD_FUNCTION( "TheNomad::GameSystem::BBox", "TheNomad::GameSystem::BBox& opAssign( const TheNomad::GameSystem::BBox& in )",
            CModuleBoundBox, operator=, ( const CModuleBoundBox& ), CModuleBoundBox& );
        REGISTER_METHOD_FUNCTION( "TheNomad::GameSystem::BBox", "void MakeBounds( const vec3& in )",
            CModuleBoundBox, MakeBounds, ( const glm::vec3& ), void );

        REGISTER_OBJECT_TYPE( "LinkEntity", CModuleLinkEntity, asOBJ_VALUE );
        REGISTER_OBJECT_BEHAVIOUR( "TheNomad::GameSystem::LinkEntity", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( CModuleLinkEntity, ( void ) ) );
        REGISTER_OBJECT_BEHAVIOUR( "TheNomad::GameSystem::LinkEntity", asBEHAVE_CONSTRUCT, "void f( const vec3& in, const BBox& in, uint, uint )", WRAP_CON( CModuleLinkEntity, ( const glm::vec3&, const CModuleBoundBox&, uint32_t, uint32_t ) ) );
        REGISTER_OBJECT_BEHAVIOUR( "TheNomad::GameSystem::LinkEntity", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( CModuleLinkEntity ) );
        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::LinkEntity", "vec3 m_Origin", offsetof( CModuleLinkEntity, m_Origin ) );
        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::LinkEntity", "uint32 m_nEntityId", offsetof( CModuleLinkEntity, m_nEntityId ) );
        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::LinkEntity", "uint32 m_nEntityType", offsetof( CModuleLinkEntity, m_nEntityType ) );
        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::LinkEntity", "uint32 m_nEntityNumber", offsetof( CModuleLinkEntity, m_nEntityNumber ) );
        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::LinkEntity", "BBox m_Bounds", offsetof( CModuleLinkEntity, m_Bounds ) );
        
        REGISTER_METHOD_FUNCTION( "TheNomad::GameSystem::LinkEntity", "TheNomad::GameSystem::LinkEntity& opAssign( const TheNomad::GameSystem::LinkEntity& in )", CModuleLinkEntity, operator=, ( const CModuleLinkEntity& ), CModuleLinkEntity& );
        REGISTER_METHOD_FUNCTION( "TheNomad::GameSystem::LinkEntity", "void SetOrigin( const vec3& in )", CModuleLinkEntity,
            SetOrigin, ( const glm::vec3& ), void );
        REGISTER_METHOD_FUNCTION( "TheNomad::GameSystem::LinkEntity", "void SetBounds( const BBox& in )", CModuleLinkEntity,
            SetBounds, ( const CModuleBoundBox& ), void );
        REGISTER_METHOD_FUNCTION( "TheNomad::GameSystem::LinkEntity", "const vec3& GetOrigin( void ) const", CModuleLinkEntity,
            GetOrigin, ( void ), const glm::vec3& );
        REGISTER_METHOD_FUNCTION( "TheNomad::GameSystem::LinkEntity", "const BBox& GetBounds( void ) const", CModuleLinkEntity,
            GetBounds, ( void ), const CModuleBoundBox& );
        REGISTER_METHOD_FUNCTION( "TheNomad::GameSystem::LinkEntity", "void Update()", CModuleLinkEntity, Update, ( void ), void );
        
//        REGISTER_OBJECT_TYPE( "RayCast", CModuleRayCast, asOBJ_VALUE );
//        REGISTER_OBJECT_BEHAVIOUR( "TheNomad::GameSystem::RayCast", asBEHAVE_CONSTRUCT, "void f()", WRAP_CON( CModuleRayCast, ( void ) ) );
//        REGISTER_OBJECT_BEHAVIOUR( "TheNomad::GameSystem::RayCast", asBEHAVE_DESTRUCT, "void f()", WRAP_DES( CModuleRayCast ) );
//        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::RayCast", "vec3 m_Start", offsetof( ray_t, start ) );
//        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::RayCast", "vec3 m_End", offsetof( ray_t, end ) );
//        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::RayCast", "vec3 m_Origin", offsetof( ray_t, origin ) );
//        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::RayCast", "uint32 m_nEntityNumber", offsetof( ray_t, entityNumber ) );
//        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::RayCast", "float m_nLength", offsetof( ray_t, length ) );
//        REGISTER_OBJECT_PROPERTY( "TheNomad::GameSystem::RayCast", "float m_nAngle", offsetof( ray_t, angle ) );
        
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::GetString( const string& in, string& out )", GetString );

		REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::BeginSaveSection( const string& in )", BeginSaveSection );
		REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::EndSaveSection()", EndSaveSection );
        REGISTER_GLOBAL_FUNCTION( "int TheNomad::GameSystem::FindSaveSection( const string& in )", FindSaveSection );

        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveInt8( const string& in, int8 )", SaveInt8 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveInt16( const string& in, int16 )", SaveInt16 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveInt32( const string& in, int32 )", SaveInt32 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveInt64( const string& in, int64 )", SaveInt64 );

        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveUInt8( const string& in, uint8 )", SaveUInt8 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveUInt16( const string& in, uint16 )", SaveUInt16 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveUInt32( const string& in, uint32 )", SaveUInt32 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveUInt64( const string& in, uint64 )", SaveUInt64 );

        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveChar( const string& in, int8 )", SaveInt8 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveShort( const string& in, int16 )", SaveInt16 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveInt( const string& in, int32 )", SaveInt32 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveLong( const string& in, int64 )", SaveInt64 );

        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveByte( const string& in, uint8 )", SaveUInt8 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveUShort( const string& in, uint16 )", SaveUInt16 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveUInt( const string& in, uint32 )", SaveUInt32 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveULong( const string& in, uint64 )", SaveUInt64 );

        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveFloat( const string& in, float )", SaveFloat );
        
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveArray( const string& in, const array<float>& in )", SaveArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveArray( const string& in, const array<int8>& in )", SaveArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveArray( const string& in, const array<int16>& in )", SaveArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveArray( const string& in, const array<int32>& in )", SaveArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveArray( const string& in, const array<int64>& in )", SaveArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveArray( const string& in, const array<uint8>& in )", SaveArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveArray( const string& in, const array<uint16>& in )", SaveArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveArray( const string& in, const array<uint32>& in )", SaveArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveArray( const string& in, const array<uint64>& in )", SaveArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveArray( const string& in, const array<string>& in )", SaveArray );

        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveString( const string& in, const string& in )", SaveString );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveVec2( const string& in, const vec2& in )", SaveVec2 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveVec3( const string& in, const vec3& in )", SaveVec3 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SaveVec4( const string& in, const vec4& in )", SaveVec4 );

        REGISTER_GLOBAL_FUNCTION( "int8 TheNomad::GameSystem::LoadInt8( const string& in, int )", LoadInt8 );
        REGISTER_GLOBAL_FUNCTION( "int16 TheNomad::GameSystem::LoadInt16( const string& in, int )", LoadInt16 );
        REGISTER_GLOBAL_FUNCTION( "int32 TheNomad::GameSystem::LoadInt32( const string& in, int )", LoadInt32 );
        REGISTER_GLOBAL_FUNCTION( "int64 TheNomad::GameSystem::LoadInt64( const string& in, int )", LoadInt64 );

        REGISTER_GLOBAL_FUNCTION( "uint8 TheNomad::GameSystem::LoadUInt8( const string& in, int )", LoadUInt8 );
        REGISTER_GLOBAL_FUNCTION( "uint16 TheNomad::GameSystem::LoadUInt16( const string& in, int )", LoadUInt16 );
        REGISTER_GLOBAL_FUNCTION( "uint32 TheNomad::GameSystem::LoadUInt32( const string& in, int )", LoadUInt32 );
        REGISTER_GLOBAL_FUNCTION( "uint64 TheNomad::GameSystem::LoadUInt64( const string& in, int )", LoadUInt64 );

        REGISTER_GLOBAL_FUNCTION( "int8 TheNomad::GameSystem::LoadChar( const string& in, int )", LoadInt8 );
        REGISTER_GLOBAL_FUNCTION( "int16 TheNomad::GameSystem::LoadShort( const string& in, int )", LoadInt16 );
        REGISTER_GLOBAL_FUNCTION( "int32 TheNomad::GameSystem::LoadInt( const string& in, int )", LoadInt32 );
        REGISTER_GLOBAL_FUNCTION( "int64 TheNomad::GameSystem::LoadLong( const string& in, int )", LoadInt64 );

        REGISTER_GLOBAL_FUNCTION( "uint8 TheNomad::GameSystem::LoadByte( const string& in, int )", LoadUInt8 );
        REGISTER_GLOBAL_FUNCTION( "uint16 TheNomad::GameSystem::LoadUShort( const string& in, int )", LoadUInt16 );
        REGISTER_GLOBAL_FUNCTION( "uint32 TheNomad::GameSystem::LoadUInt( const string& in, int )", LoadUInt32 );
        REGISTER_GLOBAL_FUNCTION( "uint64 TheNomad::GameSystem::LoadULong( const string& in, int )", LoadUInt64 );

        REGISTER_GLOBAL_FUNCTION( "float TheNomad::GameSystem::LoadFloat( const string& in, int )", LoadFloat );
        
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadArray( const string& in, array<float>& out, int )", LoadArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadArray( const string& in, array<int8>& out, int )", LoadArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadArray( const string& in, array<int16>& out, int )", LoadArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadArray( const string& in, array<int32>& out, int )", LoadArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadArray( const string& in, array<int64>& out, int )", LoadArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadArray( const string& in, array<uint8>& out, int )", LoadArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadArray( const string& in, array<uint16>& out, int )", LoadArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadArray( const string& in, array<uint32>& out, int )", LoadArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadArray( const string& in, array<uint64>& out, int )", LoadArray );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadArray( const string& in, array<string>& out, int )", LoadArray );

        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadString( const string& in, string& out, int )", LoadString );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadVec2( const string& in, vec2& out, int )", LoadVec2 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadVec3( const string& in, vec3& out, int )", LoadVec3 );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::LoadVec4( const string& in, vec4& out, int )", LoadVec4 );

        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::GetGPUGameConfig( TheNomad::Engine::Renderer::GPUConfig& out )", GetGPUConfig );

        
		// these are here because if they change in the map editor or the engine, it'll break sgame
		REGISTER_ENUM_TYPE( "EntityType" );
		REGISTER_ENUM_VALUE( "EntityType", "Playr", ET_PLAYR );
		REGISTER_ENUM_VALUE( "EntityType", "Mob", ET_MOB );
		REGISTER_ENUM_VALUE( "EntityType", "Bot", ET_BOT );
		REGISTER_ENUM_VALUE( "EntityType", "Item", ET_ITEM );
		REGISTER_ENUM_VALUE( "EntityType", "Wall", ET_WALL );
		REGISTER_ENUM_VALUE( "EntityType", "Weapon", ET_WEAPON );
        REGISTER_ENUM_VALUE( "EntityType", "NumEntityTypes", NUMENTITYTYPES );

        REGISTER_ENUM_TYPE( "GameDifficulty" );
        REGISTER_ENUM_VALUE( "GameDifficulty", "VeryEasy", DIF_NOOB );
        REGISTER_ENUM_VALUE( "GameDifficulty", "Easy", DIF_RECRUIT );
        REGISTER_ENUM_VALUE( "GameDifficulty", "Normal", DIF_MERC );
        REGISTER_ENUM_VALUE( "GameDifficulty", "Hard", DIF_NOMAD );
        REGISTER_ENUM_VALUE( "GameDifficulty", "VeryHard", DIF_BLACKDEATH );
        REGISTER_ENUM_VALUE( "GameDifficulty", "TryYourBest", DIF_MINORINCONVENIECE );
        REGISTER_ENUM_VALUE( "GameDifficulty", "NumDifficulties", NUMDIFS );
		
		REGISTER_ENUM_TYPE( "DirType" );
		REGISTER_ENUM_VALUE( "DirType", "North", DIR_NORTH );
		REGISTER_ENUM_VALUE( "DirType", "NorthEast", DIR_NORTH_EAST );
		REGISTER_ENUM_VALUE( "DirType", "East", DIR_EAST );
		REGISTER_ENUM_VALUE( "DirType", "SouthEast", DIR_SOUTH_EAST );
		REGISTER_ENUM_VALUE( "DirType", "South", DIR_SOUTH );
		REGISTER_ENUM_VALUE( "DirType", "SouthWest", DIR_SOUTH_WEST );
		REGISTER_ENUM_VALUE( "DirType", "West", DIR_WEST );
		REGISTER_ENUM_VALUE( "DirType", "NorthWest", DIR_NORTH_WEST );
		REGISTER_ENUM_VALUE( "DirType", "Inside", DIR_NULL );
        REGISTER_ENUM_VALUE( "DirType", "NumDirs", NUMDIRS );

        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::CastRay( ref@ )", CastRay );
        REGISTER_GLOBAL_FUNCTION( "bool TheNomad::GameSystem::CheckWallHit( const vec3& in, TheNomad::GameSystem::DirType )", CheckWallHit );
		
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::GetCheckpointData( uvec3& out, uint )", GetCheckpointData );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::GetSpawnData( uvec3& out, uint& out, uint& out, uint )", GetSpawnData );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::GetTileData( uint32[]& out )", GetTileData );
        REGISTER_GLOBAL_FUNCTION( "void TheNomad::GameSystem::SetActiveMap( int, uint& out, uint& out, uint& out, float[]& in, "
            "TheNomad::GameSystem::LinkEntity& in )", SetActiveMap );
        REGISTER_GLOBAL_FUNCTION( "int LoadMap( const string& in )", LoadMap );
    }

    SET_NAMESPACE( "TheNomad" );
    { // Util
        SET_NAMESPACE( "TheNomad::Util" );

        REGISTER_GLOBAL_FUNCTION( "void TheNomad::Util::GetModuleList( array<string>& out )", GetModuleList );
        REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Util::IsModuleActive( const string& in )", IsModuleActive );

        REGISTER_GLOBAL_FUNCTION( "int TheNomad::Util::StrICmp( const string& in, const string& in )", StrICmp );
        REGISTER_GLOBAL_FUNCTION( "int TheNomad::Util::StrCmp( const string& in, const string& in )", StrCmp );
        REGISTER_GLOBAL_FUNCTION( "int TheNomad::Util::StringToInt( const string& in )", StringToInt );
        REGISTER_GLOBAL_FUNCTION( "uint TheNomad::Util::StringToUInt( const string& in )", StringToUInt );
        REGISTER_GLOBAL_FUNCTION( "float TheNomad::Util::StringToFloat( const string& in )", StringToFloat );
        REGISTER_GLOBAL_FUNCTION( "int TheNomad::Util::StringToInt( const int8[]& in )", StringBufferToInt );
        REGISTER_GLOBAL_FUNCTION( "uint TheNomad::Util::StringToUInt( const int8[]& in )", StringBufferToUInt );
        REGISTER_GLOBAL_FUNCTION( "float TheNomad::Util::StringToFloat( const int8[]& in )", StringBufferToFloat );
        g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction(
            "float TheNomad::Util::Distance( const vec2& in, const vec2& in )", WRAP_FN_PR( disBetweenOBJ, ( const vec2&, const vec2& ), float ), asCALL_GENERIC );
        g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction(
            "float TheNomad::Util::Distance( const ivec2& in, const ivec2& in )", WRAP_FN_PR( disBetweenOBJ, ( const glm::ivec2&, const glm::ivec2& ), int ), asCALL_GENERIC );
        g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction(
            "float TheNomad::Util::Distance( const vec3& in, const vec3& in )", WRAP_FN_PR( disBetweenOBJ, ( const vec3&, const vec3& ), float ), asCALL_GENERIC );
        g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction(
            "float TheNomad::Util::Distance( const uvec3& in, const uvec3& in )", WRAP_FN_PR( disBetweenOBJ, ( const uvec3_t, const uvec3_t ), unsigned ), asCALL_GENERIC );
            g_pModuleLib->GetScriptEngine()->RegisterGlobalFunction(
            "float TheNomad::Util::Distance( const ivec3& in, const ivec3& in )", WRAP_FN_PR( disBetweenOBJ, ( const ivec3_t, const ivec3_t ), int ), asCALL_GENERIC );
        REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Util::BoundsIntersect( const TheNomad::GameSystem::BBox& in, const TheNomad::GameSystem::BBox& in )", BoundsIntersect );
        REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Util::BoundsIntersectPoint( const TheNomad::GameSystem::BBox& in, const vec3& in )", BoundsIntersectPoint );
        REGISTER_GLOBAL_FUNCTION( "bool TheNomad::Util::BoundsIntersectSphere( const TheNomad::GameSystem::BBox& in, const vec3& in, float )", BoundsIntersectSphere );
//        REGISTER_GLOBAL_FUNCTION( "string TheNomad::Util::SkipPath( const string& in )", SkipPath );
//        REGISTER_GLOBAL_FUNCTION( "string TheNomad::Util::GetExtension( const string& in )", GetExtension );
//        REGISTER_GLOBAL_FUNCTION( "string TheNomad::Util::DefaultExtension( const string& in, const string& in )", DefaultExtension );

        RESET_NAMESPACE();
    }

    RESET_NAMESPACE();
	
	//
	// misc & global funcdefs
	//
	
	REGISTER_GLOBAL_FUNCTION( "void ConsolePrint( const string& in )", ConsolePrint );
    REGISTER_GLOBAL_FUNCTION( "void ConsoleWarning( const string& in )", ConsoleWarning );
    REGISTER_GLOBAL_FUNCTION( "void GameError( const string& in )", GameError );
}