#include "../game/g_game.h"
#include "ui_public.hpp"
#include "ui_menu.h"
#include "ui_lib.h"
#include "ui_window.h"
#include "ui_string_manager.h"
#include "../game/g_archive.h"

typedef struct {

    // new game data
    char name[MAX_NPATH];
    gamedif_t diff;
    uint64_t hardestIndex;

	const stringHash_t *title;
    const stringHash_t *newGame;
    const stringHash_t *newGameSaveNamePrompt;
    const stringHash_t *newGameBegin;
    const stringHash_t *loadGame;
    const stringHash_t *loadGameTitle;
    const stringHash_t *playMission;

    char **hardestStrings;
    int32_t numHardestStrings;

    uint64_t currentSave;

    menuframework_t menu;

    const stringHash_t *difficultyDescriptions[NUMDIFS];
} singleplayer_t;

extern ImFont *RobotoMono;
extern ImFont *PressStart2P;
static singleplayer_t sp;

typedef struct {
    const char *name;
    const char *tooltip;
} dif_t;

static dif_t difficultyTable[NUMDIFS];

void SinglePlayerMenu_Draw( void )
{
    ImVec2 mousePos;
    uint64_t i;
    float font_scale;
    qboolean allGood = qtrue;
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_CollapsingHeader
                                        | ImGuiTreeNodeFlags_Framed;

    switch ( ui->GetState() ) {
    case STATE_SINGLEPLAYER: {
        ui->EscapeMenuToggle( STATE_MAIN );
        if ( ui->GetState() != STATE_SINGLEPLAYER ) {
            break;
        } else if ( ui->Menu_Title( sp.title->value ) ) {
            ui->SetState( STATE_MAIN );
            break;
        }
        mousePos = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos( ImVec2( mousePos.x, mousePos.y + 10 ) );
        

        FontCache()->SetActiveFont( PressStart2P );
        ImGui::BeginTable( "##SinglePlayerOptions", 2 );
        if ( ui->Menu_Option( sp.newGame->value ) ) {
            ui->SetState( STATE_NEWGAME );
            sp.hardestIndex = rand() % sp.numHardestStrings;
        }
        ImGui::TableNextRow();
        if ( ui->Menu_Option( sp.loadGame->value ) ) {
            ui->SetState( STATE_LOADGAME );
        }
        ImGui::TableNextRow();
        if (ui->Menu_Option( va( "%s COMING SOON! :)", sp.playMission->value ) ) ) {
        	// play any mission found inside the current mods loaded
//               ui->SetState( STATE_PLAYMISSION );
        }
        ImGui::EndTable();
        break; }
    case STATE_NEWGAME: {

        ImGui::SetWindowSize( ImVec2( (float)ui->GetConfig().vidWidth * 0.75f, ImGui::GetWindowSize().y ) );

        ui->EscapeMenuToggle( STATE_SINGLEPLAYER );
        const char *difName;
        if (ui->GetState() != STATE_NEWGAME) {
            // reset to 0
            memset( sp.name, 0, sizeof(sp.name) );
            sp.diff = DIF_NOOB;
            sp.hardestIndex = 0;
            break;
        }
        else if (ui->Menu_Title( sp.newGame->value )) {
            // reset to 0
            memset( sp.name, 0, sizeof(sp.name) );
            sp.diff = DIF_NOOB;
            sp.hardestIndex = 0;
            ui->SetState( STATE_SINGLEPLAYER );
            break;
        }

        mousePos = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos( ImVec2( mousePos.x, mousePos.y + 10 ) );
        if (sp.diff == DIF_HARDEST) {
            difName = sp.hardestStrings[ sp.hardestIndex ];
        } else {
            difName = difficultyTable[ sp.diff ].name;
        }
        ImGui::BeginTable( "##SinglePlayerBeginNewGame", 2 );
        {
            FontCache()->SetActiveFont( PressStart2P );
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( sp.newGameSaveNamePrompt->value );
            ImGui::TableNextColumn();
            if ( ImGui::InputText( "##SaveNameInput", sp.name, sizeof( sp.name ) - 1, ImGuiInputTextFlags_EscapeClearsAll
                | ImGuiInputTextFlags_EnterReturnsTrue ) )
            {
                ui->PlaySelected();
            }
            
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( "Difficulty" ); // NOTE: perhaps call this "Plot Armor Amount"?
            ImGui::TableNextColumn();
            if (ImGui::BeginMenu( va("%s", difName) )) {
                for (i = 0; i < NUMDIFS; i++) {
                    if (i != DIF_HARDEST) {
                        if ( ImGui::MenuItem( difficultyTable[ i ].name ) ) {
                            sp.diff = (gamedif_t)i;
                            ui->PlaySelected();
                        }
                    }
                    else {
                        if ( ImGui::MenuItem( sp.hardestStrings[ sp.hardestIndex ] ) ) {
                            sp.diff = (gamedif_t)i;
                            ui->PlaySelected();
                        }
                    }
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndTable();

        ImGui::NewLine();

        if ( ImGui::Button( sp.newGameBegin->value ) ) {
            // make sure it's an absolute path
            N_strncpyz( sp.name, COM_SkipPath( sp.name ), sizeof( sp.name ) );

            // make sure its a unique name, so we don't get filename collisions
            for ( i = 0; i < sp.numSaves; i++ ) {
                if ( N_strcmp( sp.name, sp.saveList[i].name ) == 0 ) {
                    Sys_MessageBox( "Save Name Issue",
                        "Sorry, but it looks like your save file name is either too long or already exists\n"
                        "if its too long, it must be less than or equal to %lu characters in length, please\n"
                        "shorten your save's name.\n"
                        "if it already exists, then please rename the save slot.\n"
                        "\n"
                        "Sorry for the inconvenience. :)\n"
                        "\n"
                        "Your Resident Fiend,\n"
                        "Noah Van Til\n", false );
                    allGood = qfalse;
                }
            }

            if ( allGood ) {
                memset( sp.name, 0, sizeof( sp.name ) );
                ui->PlaySelected();
                ui->SetState( STATE_NONE );
                ui->SetActiveMenu( UI_MENU_NONE );
                gi.state = GS_LEVEL;
                Key_SetCatcher( KEYCATCH_UI );

                Cvar_Set( "sgame_SaveName", sp.name );
                Cvar_SetIntegerValue( "g_levelIndex", 0 );

                gi.state = GS_LEVEL;
                Cvar_Set( "mapname", *gi.mapCache.mapList );
                g_pModuleLib->ModuleCall( sgvm, ModuleOnLevelStart, 0 ); // start a new game
            }
        }

        ImGui::NewLine();
        ImGui::NewLine();

        FontCache()->SetActiveFont( RobotoMono );

        font_scale = ImGui::GetFont()->Scale;
        ImGui::SetWindowFontScale( font_scale * 3.75f );
        ImGui::TextUnformatted( "Difficulty Description" );
        ImGui::SetWindowFontScale( font_scale * 2.75f );
        ImGui::TextWrapped( "%s", difficultyTable[(int32_t)sp.diff].tooltip );
        break; }
    case STATE_LOADGAME: {
        ui->EscapeMenuToggle( STATE_SINGLEPLAYER );
        if ( ui->GetState() != STATE_LOADGAME ) {
            break;
        } else if ( ui->Menu_Title( sp.loadGame->value, 2.25f ) ) {
            ui->SetState( STATE_SINGLEPLAYER );
            break;
        }
        mousePos = ImGui::GetCursorScreenPos();
        font_scale = ImGui::GetFont()->Scale;
        const ImVec2& windowSize = ImGui::GetWindowSize();
        ImGui::SetWindowSize( ImVec2( ui->GetConfig().vidWidth * 0.75f, windowSize.y ) );
        ImGui::SetCursorScreenPos( ImVec2( mousePos.x, mousePos.y + 10 ) );
        if ( sp.numSaves ) {
            {
                ImGui::Begin( "##ModList", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                    | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse );
                ImGui::SetWindowPos( ImVec2( ui->GetConfig().vidWidth * 0.75f, 64 * ui->scale ) );
                ImGui::SetWindowSize( ImVec2( ui->GetConfig().vidWidth * 0.25f, ui->GetConfig().vidHeight - 10 ) );

                ImGui::SeparatorText( "Loaded Modules" );
                for ( uint64_t m = 0; m < sp.saveList[sp.currentSave].gd.numMods; m++ ) {
                    if ( !sp.saveList[sp.currentSave].modsLoaded[m] ) {
                        ImGui::PushStyleColor( ImGuiCol_Text, colorRed );
                        ImGui::PushStyleColor( ImGuiCol_TextDisabled, colorRed );
                        ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, colorRed );
                    } else {
                        ImGui::PushStyleColor( ImGuiCol_Text, colorGreen );
                        ImGui::PushStyleColor( ImGuiCol_TextDisabled, colorGreen );
                        ImGui::PushStyleColor( ImGuiCol_TextSelectedBg, colorGreen );
                    }

                    ImGui::Text( "%s v%i.%i.%i", sp.saveList[sp.currentSave].gd.modList[m].name,
                        sp.saveList[sp.currentSave].gd.modList[m].nVersionMajor,
                        sp.saveList[sp.currentSave].gd.modList[m].nVersionUpdate,
                        sp.saveList[sp.currentSave].gd.modList[m].nVersionPatch );
                    ImGui::PopStyleColor( 3 );

                    if ( ImGui::IsItemHovered() && !sp.saveList[sp.currentSave].modsLoaded[m] ) {
                        ImGui::SetTooltip(
                                    "This module either hasn't been loaded or failed to load, check"
                                    "the console/logfile for more detailed information" );
                    }
                }

                ImGui::End();
            }

            FontCache()->SetActiveFont( PressStart2P );
            for (i = 0; i < sp.numSaves; i++) {
                if ( ImGui::TreeNodeEx( (void *)(uintptr_t)sp.saveList[i].name, treeNodeFlags, sp.saveList[i].name ) ) {
                    if ( ImGui::IsMouseClicked( ImGuiMouseButton_Left ) ) {
                    	ui->PlaySelected();
                    }
                    sp.currentSave = i;
                    if ( ImGui::IsItemClicked( ImGuiMouseButton_Left ) && ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) ) {
                        ui->PlaySelected();
                        Cvar_Set( "sgame_SaveName", sp.saveList[i].name );
                        gi.state = GS_LEVEL;
                        g_pModuleLib->ModuleCall( sgvm, ModuleOnLoadGame, 0 );
                    }

                    ImGui::SetWindowFontScale( font_scale * 0.75f );

                    ImGui::BeginTable( va( "Save Slots##%lu", i ), 3 );
                    {
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted( "Creation Time" );

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted( "Last Save Time" );

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted( "Difficulty" );

                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted( sp.saveList[i].creationTime ); // creation time

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted( sp.saveList[i].modificationTime ); // last used time

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted( difficultyTable[ sp.saveList[i].gd.dif ].name );
                    }
                    ImGui::EndTable();

                    ImGui::SetWindowFontScale( font_scale );
                    ImGui::TreePop();
                }
            }
        }
        else {
            ImGui::TextUnformatted( "No Saves" );
        }
        break; }
    };

    ImGui::End();
}

char **parse_csv( const char *line );
int32_t count_fields( const char *line );

void SinglePlayerMenu_Cache( void )
{
    saveinfo_t *info;
    const char **fileList;
    uint64_t i, j;
    const char *path;
    const stringHash_t *hardest;
    struct tm *fileTime;

    memset( &sp, 0, sizeof( sp ) );
    
    //
    // init strings
    //
    difficultyTable[DIF_NOOB].name = strManager->ValueForKey( "SP_DIFF_VERY_EASY" )->value;
    difficultyTable[DIF_NOOB].tooltip = strManager->ValueForKey( "SP_DIFF_0_DESC" )->value;

    difficultyTable[DIF_RECRUIT].name = strManager->ValueForKey( "SP_DIFF_EASY" )->value;
    difficultyTable[DIF_RECRUIT].tooltip = strManager->ValueForKey( "SP_DIFF_1_DESC" )->value;

    difficultyTable[DIF_MERC].name = strManager->ValueForKey( "SP_DIFF_MEDIUM" )->value;
    difficultyTable[DIF_MERC].tooltip = strManager->ValueForKey( "SP_DIFF_2_DESC" )->value;

    difficultyTable[DIF_NOMAD].name = strManager->ValueForKey( "SP_DIFF_HARD" )->value;
    difficultyTable[DIF_NOMAD].tooltip = strManager->ValueForKey( "SP_DIFF_3_DESC" )->value;

    difficultyTable[DIF_BLACKDEATH].name = strManager->ValueForKey( "SP_DIFF_VERY_HARD" )->value;
    difficultyTable[DIF_BLACKDEATH].tooltip = strManager->ValueForKey( "SP_DIFF_4_DESC" )->value;

    difficultyTable[DIF_MINORINCONVENIECE].tooltip = "PAIN."; // no changing this one, because that's the most accurate description

	sp.title = strManager->ValueForKey( "SP_MENU_TITLE" );	
	sp.newGameBegin = strManager->ValueForKey( "SP_BEGIN_NEWGAME" );
	sp.loadGameTitle = strManager->ValueForKey( "SP_LOADGAME_TITLE" );
    sp.newGame = strManager->ValueForKey( "SP_NEWGAME" );
    sp.loadGame = strManager->ValueForKey( "SP_LOADGAME" );
    sp.playMission = strManager->ValueForKey( "SP_PLAY_MISSION" );
    sp.newGameSaveNamePrompt = strManager->ValueForKey( "SP_SAVE_NAME_PROMPT" );
    
    hardest = strManager->ValueForKey( "SP_DIFF_THE_MEMES" );
    sp.hardestStrings = parse_csv( hardest->value );
    sp.numHardestStrings = count_fields( hardest->value );
}

void UI_SinglePlayerMenu( void )
{
    UI_PushMenu( &sp.menu );
    Key_SetCatcher( KEYCATCH_UI );
}

//
// a small csv parser for c, credits to semitrivial for this
// https://github.com/semitrivial/csv_parser.git
//

void free_csv_line( char **parsed ) {
    char **ptr;

    for ( ptr = parsed; *ptr; ptr++ ) {
        Z_Free( *ptr );
    }

    Z_Free( parsed );
}

int32_t count_fields( const char *line ) {
    const char *ptr;
    int32_t cnt, fQuote;

    for ( cnt = 1, fQuote = 0, ptr = line; *ptr; ptr++ ) {
        if ( fQuote ) {
            if ( *ptr == '\"' ) {
                fQuote = 0;
            }
            continue;
        }

        switch ( *ptr ) {
        case '\"':
            fQuote = 1;
            continue;
        case ',':
            cnt++;
            continue;
        default:
            continue;
        };
    }

    if ( fQuote ) {
        return -1;
    }

    return cnt;
}

static char *CopyUIString( const char *str ) {
    char *out;
    uint64_t len;

    len = strlen( str ) + 1;
    out = (char *)Z_Malloc( len, TAG_GAME );
    N_strncpyz( out, str, len );

    return out;
}

/*
 *  Given a string containing no linebreaks, or containing line breaks
 *  which are escaped by "double quotes", extract a NULL-terminated
 *  array of strings, one for every cell in the row.
 */
char **parse_csv( const char *line ) {
    char **buf, **bptr, *tmp, *tptr;
    const char *ptr;
    int32_t fieldcnt, fQuote, fEnd;

    fieldcnt = count_fields( line );

    if ( fieldcnt == -1 ) {
        return NULL;
    }

    buf = (char **)Z_Malloc( sizeof( char * ) * ( fieldcnt + 1 ), TAG_GAME );
    tmp = (char *)Hunk_AllocateTempMemory( strlen( line ) + 1 );

    bptr = buf;

    for ( ptr = line, fQuote = 0, *tmp = '\0', tptr = tmp, fEnd = 0; ; ptr++ ) {
        if ( fQuote ) {
            if ( !*ptr ) {
                break;
            }

            if ( *ptr == '\"' ) {
                if ( ptr[1] == '\"' ) {
                    *tptr++ = '\"';
                    ptr++;
                    continue;
                }
                fQuote = 0;
            }
            else {
                *tptr++ = *ptr;
            }

            continue;
        }

        switch ( *ptr ) {
        case '\"':
            fQuote = 1;
            continue;
        case '\0':
            fEnd = 1;
        case ',':
            *tptr = '\0';
            *bptr = CopyUIString( tmp );

            if ( !*bptr ) {
                for ( bptr--; bptr >= buf; bptr-- ) {
                    Z_Free( *bptr );
                }
                Z_Free( buf );
                Hunk_FreeTempMemory( tmp );

                return NULL;
            }

            bptr++;
            tptr = tmp;

            if ( fEnd ) {
                break;
            }
            continue;
        default:
            *tptr++ = *ptr;
            continue;
        };

        if ( fEnd ) {
            break;
        }
    }

    *bptr = NULL;
    Hunk_FreeTempMemory( tmp );
    return buf;
}