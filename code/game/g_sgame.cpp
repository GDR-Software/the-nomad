#include "g_game.h"
#include "g_world.h"
#include "../rendercommon/r_public.h"
#include "../engine/vm_local.h"

void G_ShutdownSGame( void )
{
    Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_SGAME );

    if ( !sgvm ) {
        return;
    }
    
    sgvm = NULL;
    FS_VM_CloseFiles( H_SGAME );
}

void G_InitSGame( void )
{
    PROFILE_FUNCTION();

    vmInterpret_t interpret;
    CTimer timer;

    timer.Run();
    interpret = (vmInterpret_t)Cvar_VariableInteger( "vm_sgame" );
    
    sgvm = g_pModuleLib->GetModule( "nomadmain" );
    if ( !sgvm ) {
        N_Error( ERR_DROP, "G_InitSGame: failed to load sgame module" );
    }

    // run a quick initialization
    g_pModuleLib->ModuleCall( sgvm, ModuleInit, 0 );

    timer.Stop();
    Con_Printf( "G_InitSGame: %5.5lf milliseconds\n", (double)timer.ElapsedMilliseconds().count() );

    // have the renderer touch all its images, so they are present
    // on the card even if the driver does deferred loading
//    re.EndRegistration();

    // make sure everything is paged in
    if ( !Sys_LowPhysicalMemory() ) {
        Com_TouchMemory();
    }

    // do not allow vid_restart for the first time
    gi.lastVidRestart = Sys_Milliseconds();

    // set state to active
    gi.state = GS_MENU;
}

/*
* G_SGameCommand: see if the current console command is claimed by the sgame
*/
qboolean G_SGameCommand( void )
{
    qboolean bRes;

    if ( !sgvm || gi.state != GS_LEVEL ) {
        return qfalse;
    }

    bRes = g_pModuleLib->ModuleCall( sgvm, ModuleCommandLine, 0 );

    return bRes;
}


