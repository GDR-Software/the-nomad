#include "ui_lib.h"

typedef enum {
    HELP_PARRY,

    NUMHELPSTRINGS
} helpstate_t;

#define MAX_DAILY_TIPS 1024

typedef struct
{
    CUIMenu menu;
    
    uint32_t numDailyTips;
    char *dailyTips[MAX_DAILY_TIPS];
    const char *tipOfTheDay;
    qboolean helpMenu;
    helpstate_t helpstate;
    float oldVolume;

    qboolean exitToMM;
    qboolean settingsMenu;

    const stringHash_t *title;
    const stringHash_t *help;
    const stringHash_t *resume;
    const stringHash_t *settings;
    const stringHash_t *checkpoint;
    const stringHash_t *exitToMainMenu;
} pausemenu_t;

#define PAUSEMENU_VOLUME_CAP 2.0f

static const char *dailyTips[] = {
    "You can parry anything that's a projectile, that includes bullets, flying corpses, blades, etc.",
    "The grappling hook deals a little bit of damage every time it hooks onto an enemy",
};

static const char *helpStrings[NUMHELPSTRINGS][2] = {
    {
        "Tutorial 1: Parrying",

        "Parrying is a mechanic that can help you a lot in fights"
    },
};

// PAUSE. REWIND. PLAY.
static pausemenu_t menu;

static void PauseMenu_Help( void )
{
    switch (ui->GetState()) {
    case STATE_HELP:
        ui->EscapeMenuToggle( STATE_PAUSE );
        if (ui->GetState() != STATE_HELP) {
            menu.helpMenu = qfalse;
            break;
        }
        else if (ui->Menu_Title( "HELP" )) {
            menu.helpMenu = qfalse;
            break;
        }
        if (ui->Menu_Option( "Tutorial 1: How To Parry" )) {
            ui->SetState( STATE_HELP_SHOW );
            menu.helpstate = HELP_PARRY;
        }
        break;
    case STATE_HELP_SHOW:
        ui->EscapeMenuToggle( STATE_HELP );
        if (ui->GetState() != STATE_HELP_SHOW) {
            break;
        }
        else if (ui->Menu_Title( helpStrings[menu.helpstate][0] )) {
            ui->SetState( STATE_HELP );
            break;
        }

        ImGui::NewLine();
        ImGui::TextUnformatted( helpStrings[menu.helpstate][1] );
        break;
    };
}

static void PauseMenu_DrawTitle( void ) {
    const float font_scale = ImGui::GetFont()->Scale;

    ImGui::SetWindowFontScale( font_scale * 3.75f * ui->scale );
    ImGui::TextUnformatted( menu.title->value );
    ImGui::SetWindowFontScale( font_scale * 1.5f * ui->scale );
}


static void PauseMenu_Draw( void )
{
    const int windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize
                            | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

    ImGui::Begin("PauseMenu", NULL, windowFlags);
    ImGui::SetWindowSize( ImVec2( (float)ui->GetConfig().vidWidth, (float)ui->GetConfig().vidHeight ) );
    ImGui::SetWindowPos( ImVec2( 0, 0 ) );

    if ( menu.helpMenu ) {
        PauseMenu_Help();
        return;
    } else if ( menu.settingsMenu ) {
        // we don't want to be drawing everything else behind the settings menu
        // otherwise its very hard to see the options
        ui->SetActiveMenu( UI_MENU_MAIN );
        ui->SetState( STATE_SETTINGS );
        return;
    }

    ui->EscapeMenuToggle( STATE_NONE );
    if ( ui->GetState() == STATE_NONE ) {
        ui->SetActiveMenu( UI_MENU_NONE );
        return;
    }

    PauseMenu_DrawTitle();

    ImGui::BeginTable( " ", 2 );
    {
        if (ui->Menu_Option( menu.resume->value )) {
            ui->SetState( STATE_NONE );
            ui->SetActiveMenu( UI_MENU_NONE );
        }
        ImGui::TableNextRow();
        if (ui->Menu_Option( menu.help->value )) {
            menu.helpMenu = qtrue;
        }
        ImGui::TableNextRow();
        if (ui->Menu_Option( menu.checkpoint->value )) {
            // rewind the checkpoint
            Cbuf_ExecuteText( EXEC_APPEND, "sgame.rewind_to_last_checkpoint\n" );
        }
        ImGui::TableNextRow();
        if (ui->Menu_Option( menu.settings->value )) {
            menu.settingsMenu = qtrue;
        }
        ImGui::TableNextRow();
        if (ui->Menu_Option( menu.exitToMainMenu->value )) {
            ui->SetState( STATE_MAIN );
            ui->SetActiveMenu( UI_MENU_MAIN );
            gi.mapLoaded = qfalse;
            g_pModuleLib->ModuleCall( sgvm, ModuleOnLevelEnd, 0 );
            Cbuf_ExecuteText( EXEC_APPEND, "unloadworld\n" );
        }
    }
    ImGui::EndTable();

    ImGui::End();
}

static void PauseMenu_LoadDailyTips( void )
{
    union {
        void *v;
        char *b;
    } f;
    uint32_t i;
    const char **text, *tok;
    const char *text_p;

    FS_LoadFile( "dailytips.txt", &f.v );
    if ( !f.v ) {
        N_Error( ERR_FATAL, "PauseMenu_Cache: failed to load daily tips file" );
    }

    text_p = f.b;
    text = &text_p;

    while ( 1 ) {
        tok = COM_ParseExt( text, qfalse );

        if ( !tok[0] ) {
            break;
        } else if ( menu.numDailyTips >= MAX_DAILY_TIPS ) {
            Con_Printf( COLOR_YELLOW "WARNING: too many daily tips\n" );
            break;
        }

        menu.dailyTips[menu.numDailyTips] = (char *)Hunk_Alloc( strlen( tok ) + 1, h_low );
        strcpy( menu.dailyTips[menu.numDailyTips], tok );
        menu.numDailyTips++;
    }

    FS_FreeFile( f.v );
}

void PauseMenu_Cache( void )
{
    memset( &menu, 0, sizeof(menu) );

    menu.menu.Draw = PauseMenu_Draw;

    menu.oldVolume = Cvar_VariableFloat( "snd_musicvol" );
    Cvar_Set( "snd_musicvol", va( "%f", PAUSEMENU_VOLUME_CAP ) );

    menu.title = strManager->ValueForKey( "MENU_PAUSE_TITLE" );
    menu.checkpoint = strManager->ValueForKey( "MENU_PAUSE_CHECKPOINT" );
    menu.help = strManager->ValueForKey( "MENU_PAUSE_HELP" );
    menu.resume = strManager->ValueForKey( "MENU_PAUSE_RESUME" );
    menu.settings = strManager->ValueForKey( "MENU_PAUSE_SETTINGS" );
    menu.exitToMainMenu = strManager->ValueForKey( "MENU_PAUSE_ETMM" );

    ui->PushMenu( &menu.menu );
}

void UI_PauseMenu( void )
{
    // force as top level menu
    ui->ForceMenuOff();
    Key_SetCatcher( KEYCATCH_UI );

    PauseMenu_Cache();
}
