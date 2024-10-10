/*
===========================================================================
Copyright (C) 2023-2024 GDR Games

This file is part of The Nomad source code.

The Nomad source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

The Nomad source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../game/g_game.h"
#include "ui_public.hpp"
#include "ui_menu.h"
#include "ui_lib.h"
#include "ui_window.h"
#include "ui_string_manager.h"
#include "../rendercommon/imgui_impl_opengl3.h"
#include "../rendercommon/imgui_internal.h"
#include "../game/imgui_memory_editor.h"
//#include "RobotoMono-Bold.h"
#define FPS_FRAMES 60

uiGlobals_t *ui;
CUIFontCache *g_pFontCache;

ImFont *AlegreyaSC;
ImFont *PressStart2P;
ImFont *RobotoMono;

cvar_t *ui_language;
cvar_t *ui_cpuString;
cvar_t *ui_printStrings;
cvar_t *ui_active;
cvar_t *ui_diagnostics;
cvar_t *r_gpuDiagnostics;
cvar_t *ui_debugOverlay;
cvar_t *ui_maxLangStrings;
cvar_t *ui_menuStyle;
dif_t difficultyTable[NUMDIFS];

static cvar_t *com_drawFPS;

/*
=================
UI_Cache
=================
*/
static void UI_Cache_f( void ) {
	Con_Printf( "Caching ui resources...\n" );

	MainMenu_Cache();
	ModsMenu_Cache();
	DemoMenu_Cache();
	PlayMenu_Cache();
	PauseMenu_Cache();
	CreditsMenu_Cache();
	ConfirmMenu_Cache();
	DataBaseMenu_Cache();
	SettingsMenu_Cache();
}

CUIFontCache::CUIFontCache( void ) {
	union {
		void *v;
		char *b;
	} f;
	uint64_t nLength;
	const char **text;
	const char *tok, *text_p;
	float scale;
	char name[MAX_NPATH];

	Con_Printf( "Initializing font cache...\n" );

	memset( m_FontList, 0, sizeof( m_FontList ) );
	m_pCurrentFont = NULL;

	nLength = FS_LoadFile( "fonts/font_config.txt", &f.v );
	if ( !nLength || !f.v ) {
		N_Error( ERR_FATAL, "CUIFontCache::Init: failed to load fonts/font_config.txt" );
	}

	text_p = f.b;
	text = (const char **)&text_p;

	COM_BeginParseSession( "fonts/font_config.txt" );

	tok = COM_ParseExt( text, qtrue );
	if ( tok[0] != '{' ) {
		COM_ParseError( "expected '{' at beginning of file" );
		FS_FreeFile( f.v );
		return;
	}
	while ( 1 ) {
		tok = COM_ParseExt( text, qtrue );
		if ( !tok[0] ) {
			COM_ParseError( "unexpected end of file" );
			break;
		}
		if ( tok[0] == '}' ) {
			break;
		}

		scale = 1.0f;

		if ( tok[0] == '{' ) {
			while ( 1 ) {
				tok = COM_ParseExt( text, qtrue );
				if ( !tok[0] ) {
					COM_ParseError( "unexpected end of font defintion" );
					FS_FreeFile( f.v );
					return;
				}
				if ( tok[0] == '}' ) {
					break;
				}
				if ( !N_stricmp( tok, "name" ) ) {
					tok = COM_ParseExt( text, qfalse );
					if ( !tok[0] ) {
						COM_ParseError( "missing parameter for 'name'" );
						FS_FreeFile( f.v );
						return;
					}
					N_strncpyz( name, tok, sizeof( name ) - 1 );
				}
				else if ( !N_stricmp( tok, "scale" ) ) {
					tok = COM_ParseExt( text, qfalse );
					if ( !tok[0] ) {
						COM_ParseError( "missing parameter for 'scale'" );
						FS_FreeFile( f.v );
						return;
					}
					scale = atof( tok );
				}
				else {
					COM_ParseWarning( "unrecognized token '%s'", tok );
				}
			}
			if ( !AddFontToCache( name, "", scale ) ) {
				N_Error( ERR_FATAL, "CUIFontCache::Init: failed to load font data for 'fonts/%s/%s.ttf'", name, name );
			}
		}
	}

	FS_FreeFile( f.v );
}

void CUIFontCache::SetActiveFont( ImFont *font )
{
	if ( !ImGui::GetIO().Fonts->IsBuilt() ) {
		Finalize();
	}

	if ( !ImGui::GetFont()->ContainerAtlas ) {
		return;
	}
	if ( m_pCurrentFont ) {
		ImGui::PopFont();
	}
	m_pCurrentFont = font;
	ImGui::PushFont( font );
}

void CUIFontCache::SetActiveFont( nhandle_t hFont )
{
	if ( !ImGui::GetFont()->ContainerAtlas ) {
		return;
	}
	if ( !ImGui::GetIO().Fonts->IsBuilt() ) {
		Finalize();
	}

	if ( hFont == FS_INVALID_HANDLE || !m_FontList[ hFont ] ) {
		return;
	}

	if ( m_pCurrentFont ) {
		ImGui::PopFont();
	}

	m_pCurrentFont = m_FontList[ hFont ]->m_pFont;

	ImGui::PushFont( m_pCurrentFont );
}

uiFont_t *CUIFontCache::GetFont( const char *fileName ) {
	return m_FontList[ Com_GenerateHashValue( fileName, MAX_UI_FONTS ) ];
}

void CUIFontCache::ClearCache( void ) {
	if ( ImGui::GetCurrentContext() && ImGui::GetIO().Fonts ) {
		ImGui::GetIO().Fonts->Clear();
	}
	memset( m_FontList, 0, sizeof( m_FontList ) );
	m_pCurrentFont = NULL;

	RobotoMono = NULL;
	PressStart2P = NULL;
	AlegreyaSC = NULL;
}

void CUIFontCache::Finalize( void ) {
	ImGui::GetIO().Fonts->Build();
	ImGui_ImplOpenGL3_CreateFontsTexture();
}

nhandle_t CUIFontCache::RegisterFont( const char *filename, const char *variant, float scale ) {
	uint64_t hash;
	char rpath[MAX_NPATH];
	char hashpath[MAX_NPATH];

	COM_StripExtension( filename, rpath, sizeof( rpath ) );
	if ( rpath[ strlen( rpath ) - 1 ] == '.' ) {
		rpath[ strlen( rpath) - 1 ] = 0;
	}
	Com_snprintf( hashpath, sizeof( hashpath ) - 1, "%s", rpath );

	hash = Com_GenerateHashValue( hashpath, MAX_UI_FONTS );
	AddFontToCache( filename, variant, scale );

	return hash;
}

ImFont *CUIFontCache::AddFontToCache( const char *filename, const char *variant, float scale )
{
	uiFont_t *font;
	uint64_t size;
	uint64_t hash;
	ImFontConfig config;
	const char *path;
	union {
		void *v;
		char *b;
	} f;
	char rpath[MAX_NPATH];
	char hashpath[MAX_NPATH];

	COM_StripExtension( filename, rpath, sizeof( rpath ) );
	if ( rpath[ strlen( rpath ) - 1 ] == '.' ) {
		rpath[ strlen( rpath) - 1 ] = 0;
	}

	Com_snprintf( hashpath, sizeof( hashpath ) - 1, "%s", rpath );

	path = va( "fonts/%s.ttf", hashpath );
	hash = Com_GenerateHashValue( hashpath, MAX_UI_FONTS );

	//
	// see if we already have the font in the cache
	//
	for ( font = m_FontList[hash]; font; font = font->m_pNext ) {
		if ( !N_stricmp( font->m_szName, hashpath ) ) {
			return font->m_pFont; // its already been loaded
		}
	}

	Con_Printf( "CUIFontCache: loading font '%s'...\n", path );

	if ( strlen( hashpath ) >= MAX_NPATH ) {
		N_Error( ERR_DROP, "CUIFontCache::AddFontToCache: name '%s' is too long", hashpath );
	}

	size = FS_LoadFile( path, &f.v );
	if ( !size || !f.v ) {
		N_Error( ERR_DROP, "CUIFontCache::AddFontToCache: failed to load font file '%s'", path );
	}

	font = (uiFont_t *)Hunk_Alloc( sizeof( *font ), h_low );

	font->m_pNext = m_FontList[hash];
	m_FontList[hash] = font;

	config.FontDataOwnedByAtlas = false;
	config.GlyphExtraSpacing.x = 0.0f;

	N_strncpyz( font->m_szName, hashpath, sizeof( font->m_szName ) );
	font->m_nFileSize = size;
	font->m_pFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF( f.v, size, 16.0f * scale, &config );

	FS_FreeFile( f.v );

	return font->m_pFont;
}

void CUIFontCache::ListFonts_f( void ) {
	uint64_t memSize, i;
	uint64_t numFonts;
	const uiFont_t *font;

	Con_Printf( "---------- Font Cache Info ----------\n" );

	numFonts = 0;
	memSize = 0;
	for ( i = 0; i < MAX_UI_FONTS; i++ ) {
		font = g_pFontCache->m_FontList[i];

		if ( !font ) {
			continue;
		}

		Con_Printf( "[%s]\n", font->m_szName );
		Con_Printf( "File Size: %lu\n", font->m_nFileSize );

		memSize += font->m_nFileSize;
		numFonts++;
	}

	Con_Printf( "\n" );
	Con_Printf( "%-8lu total bytes in font cache\n", memSize );
	Con_Printf( "%-8lu total fonts in cache\n", numFonts );
}

const char *UI_LangToString( int32_t lang )
{
	switch ((language_t)lang) {
	case LANGUAGE_ENGLISH:
		return "english";
	default:
		break;
	};
	return "Invalid";
}

static void UI_RegisterCvars( void )
{
	ui_language = Cvar_Get( "ui_language", "english", CVAR_LATCH | CVAR_SAVE );
	Cvar_SetDescription( ui_language,
							"Sets the game's language: american_english, british_english, spanish, german\n"
							"Currently only english is supported, but I'm looking for some translators :)"
				   		);

	ui_cpuString = Cvar_Get( "sys_cpuString", "detect", CVAR_PROTECTED | CVAR_ROM | CVAR_NORESTART );

	ui_printStrings = Cvar_Get( "ui_printStrings", "1", CVAR_LATCH | CVAR_SAVE | CVAR_PRIVATE );
	Cvar_CheckRange( ui_printStrings, "0", "1", CVT_INT );
	Cvar_SetDescription( ui_printStrings, "Print value strings set by the language ui file" );

#ifdef _NOMAD_DEBUG
	ui_debugOverlay = Cvar_Get( "ui_debugOverlay", "1", CVAR_SAVE );
#else
	ui_debugOverlay = Cvar_Get( "ui_debugOverlay", "0", CVAR_SAVE );
#endif
	Cvar_SetDescription( ui_debugOverlay, "Draws an overlay of various debugging statistics." );

	ui_active = Cvar_Get( "g_paused", "1", CVAR_TEMP );

#ifdef _NOMAD_DEBUG
	r_gpuDiagnostics = Cvar_Get( "r_gpuDiagnostics", "1", CVAR_LATCH | CVAR_SAVE );
#else
	r_gpuDiagnostics = Cvar_Get( "r_gpuDiagnostics", "0", CVAR_LATCH | CVAR_SAVE );
#endif

	com_drawFPS = Cvar_Get( "com_drawFPS", "0", CVAR_SAVE );
	Cvar_SetDescription( com_drawFPS, "Toggles displaying the average amount of frames drawn per second." );

#ifdef _NOMAD_DEBUG
	ui_diagnostics = Cvar_Get( "ui_diagnostics", "3", CVAR_PROTECTED | CVAR_SAVE );
#else
	ui_diagnostics = Cvar_Get( "ui_diagnostics", "0", CVAR_PROTECTED | CVAR_SAVE );
#endif
	Cvar_SetDescription( ui_diagnostics, "Displays various engine performance diagnostics:\n"
											" 0 - disabled\n"
											" 1 - display gpu memory usage\n"
											" 2 - display cpu memory usage\n"
											" 3 - SHOW ME EVERYTHING!!!!" );
	
	ui_maxLangStrings = Cvar_Get( "ui_maxLangStrings", "528", CVAR_TEMP | CVAR_LATCH );
	Cvar_CheckRange( ui_maxLangStrings, "528", "8192", CVT_INT );

	ui_menuStyle = Cvar_Get( "ui_menuStyle", "0", CVAR_SAVE );
	Cvar_CheckRange( ui_menuStyle, "0", "5", CVT_INT );
	Cvar_SetDescription( ui_menuStyle, "Sets the ui's generate layout." );
}

extern "C" void UI_Shutdown( void )
{
	if ( ui ) {
		ui->activemenu = NULL;
		memset( ui->stack, 0, sizeof( ui->stack ) );
		ui->menusp = 0;
		ui->uiAllocated = qfalse;
	}

	if ( strManager ) {
		strManager->Shutdown();
		strManager = NULL;
	}

	if ( FontCache() ) {
		FontCache()->ClearCache();
	}

	Cmd_RemoveCommand( "ui.cache" );
	Cmd_RemoveCommand( "ui.fontinfo" );
	Cmd_RemoveCommand( "togglepausemenu" );
	Cmd_RemoveCommand( "ui.reload_savefiles" );
}

// FIXME: call UI_Shutdown instead
void G_ShutdownUI( void ) {
	UI_Shutdown();
}

/*
* UI_GetHashString: an sgame interface for the string manager
*/
extern "C" void UI_GetHashString( const char *name, char *value ) {
	const stringHash_t *hash;

	hash = strManager->ValueForKey( name );

	N_strncpyz( value, hash->value, MAX_STRING_CHARS );
}

static void UI_PauseMenu_f( void ) {
	if ( gi.state != GS_LEVEL || !gi.mapLoaded ) {
		return;
	}
	UI_SetActiveMenu( UI_MENU_PAUSE );
}

static int32_t previousTimes[FPS_FRAMES];

extern "C" void UI_DrawFPS( void )
{
	if ( !com_drawFPS->i ) {
		return;
	}

	static int32_t index;
	static int32_t previous;
	int32_t t, frameTime;
	int32_t total, i;
	int32_t fps;
	extern ImFont *RobotoMono;

	if ( RobotoMono ) {
		FontCache()->SetActiveFont( RobotoMono );
	}

	fps = 0;

	t = Sys_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0; i < FPS_FRAMES; i++ ) {
			total += previousTimes[i];
		}
		if ( total == 0 ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;
	} else {
		fps = previous;
	}

	ImGui::Begin( "DrawFPS##UI", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar
										| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMouseInputs
										| ImGuiWindowFlags_NoBackground );
	ImGui::SetWindowPos( ImVec2( 900 * ui->scale + ui->bias, 8 * ui->scale ) );
	ImGui::SetWindowFontScale( 1.5f * ui->scale );
	ImGui::Text( "%i", fps );
	ImGui::End();
}

void UI_EscapeMenuToggle( void )
{
	if ( ( Key_IsDown( KEY_ESCAPE ) || Key_IsDown( KEY_PAD0_B ) && !ImGui::IsAnyItemActive() ) && ui->menusp > 1 ) {
		if ( !ui->escapeToggle ) {
			ui->escapeToggle = qtrue;
			UI_PopMenu();
			Snd_PlaySfx( ui->sfx_back );
		}
	} else {
		ui->escapeToggle = qfalse;
	}
}

extern "C" void UI_Init( void )
{
	Con_Printf( "UI_Init: initializing UI...\n" );

	// register cvars
	UI_RegisterCvars();

	// init the library
	ui = (uiGlobals_t *)Hunk_Alloc( sizeof( *ui ), h_high );

	// init the string manager
	strManager = (CUIStringManager *)Hunk_Alloc( sizeof( *strManager ), h_high );
	strManager->Init();
	// load the language string file
	strManager->LoadLanguage( ui_language->s );
	if ( !strManager->NumLangsLoaded() ) {
		N_Error( ERR_DROP, "UI_Init: no language loaded" );
	}

	//
	// init strings
	//
	difficultyTable[ DIF_EASY ].name = strManager->ValueForKey( "SP_DIFF_EASY" )->value;
	difficultyTable[ DIF_EASY ].tooltip = strManager->ValueForKey( "SP_DIFF_0_DESC" )->value;

	difficultyTable[ DIF_NORMAL ].name = strManager->ValueForKey( "SP_DIFF_NORMAL" )->value;
	difficultyTable[ DIF_NORMAL ].tooltip = strManager->ValueForKey( "SP_DIFF_1_DESC" )->value;

	difficultyTable[ DIF_HARD ].name = strManager->ValueForKey( "SP_DIFF_HARD" )->value;
	difficultyTable[ DIF_HARD ].tooltip = strManager->ValueForKey( "SP_DIFF_2_DESC" )->value;

	difficultyTable[ DIF_VERY_HARD ].name = strManager->ValueForKey( "SP_DIFF_VERY_HARD" )->value;
	difficultyTable[ DIF_VERY_HARD ].tooltip = strManager->ValueForKey( "SP_DIFF_3_DESC" )->value;

	difficultyTable[ DIF_INSANE ].name = strManager->ValueForKey( "SP_DIFF_INSANE" )->value;
	difficultyTable[ DIF_INSANE ].tooltip = strManager->ValueForKey( "SP_DIFF_4_DESC" )->value;

	difficultyTable[ DIF_MEME ].tooltip = "PAIN."; // no changing this one, because that's the most accurate description

	// cache redundant calulations
	re.GetConfig( &ui->gpuConfig );

	// for 640x480 virtualized screen
	ui->scale = ui->gpuConfig.vidHeight * ( 1.0f / 768.0f );
	if ( ui->gpuConfig.vidWidth * 1024.0f > ui->gpuConfig.vidHeight * 768.0f ) {
		// wide screen
		ui->bias = 0.5f * ( ui->gpuConfig.vidWidth - ( ui->gpuConfig.vidHeight * ( 1024.0f / 768.0f ) ) );
	}
	else {
		// no wide screen
		ui->bias = 0.0f;
	}

	// initialize the menu system
	Menu_Cache();

	ui->activemenu = NULL;
	ui->menusp     = 0;

	ui->uiAllocated = qfalse;

	UI_Cache_f();
	UI_SetActiveMenu( UI_MENU_MAIN );

	ui->uiAllocated = qtrue;

	// are we running a demo?
	if ( FS_FOpenFileRead( "demokey.txt", NULL ) > 0 ) {
		ui->demoVersion = qtrue;
	} else {
		ui->demoVersion = qfalse;
	}

	memset( previousTimes, 0, sizeof( previousTimes ) );

	// add commands
	Cmd_AddCommand( "ui.cache", UI_Cache_f );
	Cmd_AddCommand( "ui.fontinfo", CUIFontCache::ListFonts_f );
	Cmd_AddCommand( "togglepausemenu", UI_PauseMenu_f );
	Cmd_AddCommand( "ui.reload_savefiles", UI_ReloadSaveFiles_f );

//	ImGuiIO& io = ImGui::GetIO();
//	ImFontConfig config;
//
//	memset( &config, 0, sizeof( config ) );
//	config.FontDataOwnedByAtlas = false;
//
//	ImFont *font = io.Fonts->AddFontFromMemoryTTF( (void *)g_RobotoMono_Bold, sizeof( g_RobotoMono_Bold ), 16.0f, &config );
//	io.FontDefault = font;
}

void Menu_Cache( void )
{
	ui->whiteShader = re.RegisterShader( "white" );
	ui->back_0 = re.RegisterShader( "menu/backbutton0" );
	ui->back_1 = re.RegisterShader( "menu/backbutton1" );

	ui->sfx_select = Snd_RegisterSfx( "event:/sfx/menu/select_item" );
	ui->sfx_back = Snd_RegisterSfx( "event:/sfx/menu/back" );

	ui->controller_start = re.RegisterShader( "menu/xbox_start" );
	ui->controller_back = re.RegisterShader( "menu/xbox_back" );
	ui->controller_a = re.RegisterShader( "menu/xbox_button_a" );
	ui->controller_b = re.RegisterShader( "menu/xbox_button_b" );
	ui->controller_x = re.RegisterShader( "menu/xbox_button_x" );
	ui->controller_y = re.RegisterShader( "menu/xbox_button_y" );
	ui->controller_dpad_down = re.RegisterShader( "menu/dpad_down" );
	ui->controller_dpad_up = re.RegisterShader( "menu/dpad_up" );
	ui->controller_dpad_left = re.RegisterShader( "menu/dpad_left" );
	ui->controller_dpad_right = re.RegisterShader( "menu/dpad_right" );
	ui->controller_left_button = re.RegisterShader( "menu/left_button" );
	ui->controller_right_button = re.RegisterShader( "menu/right_button" );
	ui->controller_left_trigger = re.RegisterShader( "menu/left_trigger" );
	ui->controller_right_trigger = re.RegisterShader( "menu/right_trigger" );

	// cache the textures
	// for some reason, we need to load these before the backdrop
	// if we want the lower resolution textures to load properly
	re.RegisterShader( "menu/save_0" );
	re.RegisterShader( "menu/save_1" );
	re.RegisterShader( "menu/load_0" );
	re.RegisterShader( "menu/load_1" );
	re.RegisterShader( "menu/reset_0" );
	re.RegisterShader( "menu/reset_1" );
	re.RegisterShader( "menu/accept_0" );
	re.RegisterShader( "menu/accept_1" );
	re.RegisterShader( "menu/play_0" );
	re.RegisterShader( "menu/play_1" );
	re.RegisterShader( "menu/tales_around_the_campfire" );

	ui->backdrop = re.RegisterShader( "menu/mainbackdrop" );

	// IT MUST BE THERE!
	if ( !FS_LoadFile( "textures/coconut.jpg", NULL ) || ui->backdrop == FS_INVALID_HANDLE ) {
		N_Error( ERR_FATAL, "YOU DARE DEFY THE WILL OF THE GODS!?!?!?!?!?" );
	}
}

/*
=================
UI_Refresh
=================
*/

extern "C" void UI_ShowDemoMenu( void )
{
	UI_SetActiveMenu( UI_MENU_DEMO );
}

extern "C" void UI_DrawMenuBackground( void )
{
	refdef_t refdef;

	memset( &refdef, 0, sizeof( refdef ) );
	refdef.x = 0;
	refdef.y = 0;
	refdef.width = ui->gpuConfig.vidWidth;
	refdef.height = ui->gpuConfig.vidHeight;
	refdef.time = Sys_Milliseconds();
	refdef.flags = RSF_NOWORLDMODEL | RSF_ORTHO_TYPE_SCREENSPACE;

	//
	// draw the background
	//
	re.ClearScene();
	re.SetColor( colorWhite );
	re.DrawImage( 0, 0, refdef.width, refdef.height, 0, 0, 1, 1, ui->menubackShader );
	re.RenderScene( &refdef );
}

extern "C" void UI_AddJoystickKeyEvents( void )
{
	ImGuiIO& io = ImGui::GetIO();

	io.AddKeyEvent( ImGuiKey_Escape, Key_IsDown( KEY_PAD0_BACK ) );
	io.AddKeyEvent( ImGuiKey_Enter, Key_IsDown( KEY_PAD0_START ) );

	io.AddKeyEvent( ImGuiKey_DownArrow, Key_IsDown( KEY_PAD0_LEFTSTICK_DOWN ) );
	io.AddKeyEvent( ImGuiKey_UpArrow, Key_IsDown( KEY_PAD0_LEFTSTICK_UP ) );
	io.AddKeyEvent( ImGuiKey_LeftArrow, Key_IsDown( KEY_PAD0_LEFTSTICK_LEFT ) );
	io.AddKeyEvent( ImGuiKey_RightArrow, Key_IsDown( KEY_PAD0_LEFTSTICK_RIGHT ) );
}

static void UI_DrawDebugOverlay( void )
{
	if ( ui_debugOverlay->i == 1 ) {
	}
}

extern "C" void UI_Refresh( int32_t realtime )
{
	extern cvar_t *in_joystick;
	static qboolean windowFocus = qfalse;

	ui->realtime = realtime;
	ui->frametime = ui->frametime - realtime;

	UI_DrawFPS();

	{
		refdef_t refdef;

		memset( &refdef, 0, sizeof( refdef ) );
		refdef.x = 0;
		refdef.y = 0;
		refdef.width = ui->gpuConfig.vidWidth;
		refdef.height = ui->gpuConfig.vidHeight;
		refdef.time = 0;
		refdef.flags = RSF_ORTHO_TYPE_SCREENSPACE | RSF_NOWORLDMODEL;
	}

	if ( !( Key_GetCatcher() & KEYCATCH_UI ) ) {
		return;
	}

	if ( ui->activemenu && ui->activemenu->fullscreen ) {
		UI_DrawMenuBackground();
	}

	if ( ui_debugOverlay->i && ui->menustate != UI_MENU_SPLASH ) {
		UI_DrawDebugOverlay();
	}

	if ( in_joystick->i ) {
		UI_AddJoystickKeyEvents();
	}

	if ( ui->activemenu ) {
		if ( ui->activemenu->track != FS_INVALID_HANDLE ) {
			Snd_AddLoopingTrack( ui->activemenu->track );
		}

		if ( ui->activemenu->draw ) {
			ui->activemenu->draw();
		} else {
			Menu_Draw( ui->activemenu );
		}
	}
/*
	// draw cursor
//	ui->SetColor( NULL );
//	ui->DrawHandlePic( ui->GetCursorX() - 16, ui->GetCursorY() - 16, 32, 32, cursor);

#ifdef _NOMAD_DEBUG
	if (ui->IsDebug()) {
		// cursor coordinates
		ui->DrawString( 0, 0, va("(%d,%d)", ui->GetCursorX(), ui->GetCursorY()), UI_LEFT|UI_SMALLFONT, color_red );
	}
#endif

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound) {
		Snd_PlaySfx( menu_in_sound );
		m_entersound = qfalse;
	}
	*/
}

/*
#define TRACE_FRAMES 60

typedef struct {
	double cpuFrames[TRACE_FRAMES];
	uint32_t gpuTimes[TRACE_FRAMES];
	uint32_t gpuSamples[TRACE_FRAMES];
	uint32_t gpuPrimitives[TRACE_FRAMES];

	uint64_t virtualHeapUsed;
	uint64_t physicalHeapUsed;
	uint64_t stackMemoryUsed;

	uint32_t gpuTimeMin;
	uint32_t gpuTimeMax;
	uint32_t gpuTimeAvg;
	int32_t gpuTimeIndex;
	uint32_t gpuTimePrevious;

	qboolean cpuNewMin;
	qboolean cpuNewMax;
	double cpuMin;
	double cpuMax;
	double cpuAvg;
	int32_t cpuIndex;
	double cpuPrevious;
} sys_stats_t;


// NOTE: got all the win32 code from stack overflow, don't mess with it!!!!!
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <pdh.h>

static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
static DWORD dwNumProcessors;
static HANDLE self;
#endif

#ifdef __unix__
#include <unistd.h>
#include <sys/resource.h>
#include <sys/times.h>

static FILE *cpuInfo;

static int32_t numProcessors;
static clock_t lastCPU, lastSysCPU, lastUserCPU;
#endif

CallBeforeMain(Sys_InitCPUMonitor)
{
#ifdef _WIN32
	SYSTEM_INFO sysInfo{};
	FILETIME ftime, fsys, fuser;
	
	GetSystemInfo( &sysInfo );
	dwNumProcessors = sysInfo.dwNumberOfProcessors;
	
	GetSystemTimeAsFileTime( &ftime );
	memcpy( &lastCPU, &ftime, sizeof(FILETIME) );
	
	self = GetCurrentProcess();
	GetProcessTimes( self, &ftime, &ftime, &fsys, &fuser );
	memcpy( &lastSysCPU, &fsys, sizeof(FILETIME) );
	memcpy( &lastUserCPU, &fuser, sizeof(FILETIME) );
#elif defined(__unix__)
	struct tms timeSample;
	char line[128];

	cpuInfo = fopen( "/proc/cpuinfo", "r" );
	if ( !cpuInfo ) {
		Sys_Error( "Sys_InitCPUMonitor: failed to open /proc/cpuinfo in readonly mode!" );
	}
	
	numProcessors = 0;
	lastCPU = times( &timeSample );
	lastSysCPU = timeSample.tms_stime;
	lastUserCPU = timeSample.tms_utime;
	
	while (fgets( line, sizeof(line), cpuInfo )) {
		if (strncmp( line, "processor", 9 ) == 0) {
			numProcessors++;
		}
	}

	fclose( cpuInfo );
#endif
}

static double Sys_GetCPUUsage( void )
{
#ifdef _WIN32
	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;
	double percent;
	
	GetSystemTimeAsFileTime( &ftime );
	memcpy( &now, &ftime, sizeof(FILETIME) );
	
	GetProcessTimes( self, &ftime, &ftime, &fsys, &fuser );
	memcpy( &sys, &fsys, sizeof(FILETIME) );
	memcpy( &user, &fuser, sizeof(FILETIME) );
	
	percent = (sys.QuadPart - lastSysCPU.QuadPart) + (user.QuadPart - lastUserCPU.QuadPart);
	percent /= (now.QuadPart - lastCPU.QuadPart);
	percent /= dwNumProcessors;
	
	lastCPU = now;
	lastUserCPU = user;
	lastSysCPU = sys;
	
	return percent * 100;
#elif defined(__APPLE__)
#elif defined(__unix__)
	struct tms timeSample;
	clock_t now;
	double percent;
	
	now = times( &timeSample );
	if (now <= lastCPU || timeSample.tms_stime < lastSysCPU || timeSample.tms_utime < lastUserCPU) {
		// overflow detection, just skip this value
		percent = -1.0f;
	}
	else {
		percent = (timeSample.tms_stime - lastSysCPU) + (timeSample.tms_utime - lastUserCPU);
		percent /= (now - lastCPU);
		percent /= numProcessors;
		percent *= 100.0f;
	}
	
	lastCPU = now;
	lastSysCPU = timeSample.tms_stime;
	lastUserCPU = timeSample.tms_utime;
	
	return percent;
#endif
}

#ifdef __unix__
static bool Posix_GetProcessMemoryUsage( uint64_t *virtualMem, uint64_t *physicalMem )
{
	char line[1024];
	int64_t unused;
	FILE *self;

	self = fopen( "/proc/self/status", "r" );
	if ( !self ) {
		N_Error( ERR_FATAL, "Posix_GetProcessMemoryUsage: failed to open /proc/self/status" );
	}

	while ( fscanf( self, "%1023s", line ) > 0 ) {
		if ( strstr( line, "VmRSS:" ) ) {
			fscanf( self, " %lu", physicalMem );
		} else if ( strstr( line, "VmSize:" ) ) {
			fscanf( self, " %lu", virtualMem );
		}
	}

	*physicalMem *= 1024;
	*virtualMem *= 1024;

	fclose( self );
	
	return true;
}
#endif

static void Sys_GetMemoryUsage( sys_stats_t *usage )
{
#ifdef _WIN32
	PROCESS_MEMORY_COUNTERS_EX pmc{};
	MEMORYSTATUSEX memInfo{};
	PDH_HQUERY cpuQuery;
	PDH_HCOUNTER cpuTotal;
	GetProcessMemoryInfo( GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc) );
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx( &memInfo );
	
	usage->virtualHeapUsed = pmc.PrivateUsage;
	usage->physicalHeapUsed = pmc.WorkingSetSize;
#elif defined(__APPLE__)
#elif defined(__unix__)
	Posix_GetProcessMemoryUsage( &usage->virtualHeapUsed, &usage->physicalHeapUsed );
#endif
}

static sys_stats_t *stats;

static void Sys_GetCPUStats( void )
{
	const double cpuTime = Sys_GetCPUUsage();
	double total, cpu;
	
	if ( cpuTime < stats->cpuMin ) {
		stats->cpuMin = cpuTime;
		stats->cpuNewMin = qtrue;
	} else if ( cpuTime > stats->cpuMax ) {
		stats->cpuMax = cpuTime;
		stats->cpuNewMax = qtrue;
	}
	
	stats->cpuPrevious = cpuTime;
	
	stats->cpuFrames[stats->cpuIndex % TRACE_FRAMES] = cpuTime;
	stats->cpuIndex++;
	
	if (stats->cpuIndex > TRACE_FRAMES) {
		// average multiple frames of cpu usage to smooth changes out
		for (uint32_t i = 0; i < TRACE_FRAMES; i++) {
			stats->cpuAvg += stats->cpuFrames[i];
		}
		
		stats->cpuAvg /= TRACE_FRAMES;
	}
}

static void Sys_DrawMemoryUsage( void )
{
	ImGui::SeparatorText( "Memory Usage/Stats" );
	ImGui::Text( "Blocks Currently Allocated: %i", SDL_GetNumAllocations() );
	ImGui::Text( "Total Virtual Memory Used: %lu", stats->virtualHeapUsed );	
	ImGui::Text( "Total Physical Memory Used: %lu", stats->physicalHeapUsed );
	ImGui::Text( "Total Stack Memory Remaining: %lu", Sys_StackMemoryRemaining() );
}

static void Sys_GPUStatFrame( uint32_t stat, uint32_t *min, uint32_t *max, uint32_t *avg, uint32_t *prev, uint32_t *frames, int32_t *index )
{
	if ( stat < *min ) {
		*min = stat;
	} else if ( stat > *max ) {
		*max = stat;
	}

	*prev = stat;

	frames[*index % TRACE_FRAMES] = stat;
	(*index)++;

	if ( *index > TRACE_FRAMES ) {
		// average multiple frames of stats to smooth changes out
		for ( uint32_t i = 0; i < TRACE_FRAMES; i++ ) {
			*avg += frames[i];
		}

		*avg /= TRACE_FRAMES;
	}
}

static void Sys_DrawGPUStats( void )
{
	uint32_t time, samples, primitives;

	re.GetGPUFrameStats( &time, &samples, &primitives );

	Sys_GPUStatFrame( time, &stats->gpuTimeMin, &stats->gpuTimeMax, &stats->gpuTimeAvg, &stats->gpuTimePrevious, stats->gpuTimes,
		&stats->gpuTimeIndex );

	ImGui::SeparatorText( "GPU Frame Statistics" );
	ImGui::Text( "Time Elapsed (Average): %u", stats->gpuTimeAvg );
	ImGui::Text( "Samples Passed: %u", samples );
	ImGui::Text( "Primitives Generated: %u", primitives );
}

static void Sys_DrawCPUUsage( void )
{
	ImGui::SeparatorText( "CPU Usage" );
	ImGui::Text( "Number of CPU Cores: %i", SDL_GetCPUCount() );

	ImGui::BeginTable( " ", 4 );
	{
		ImGui::TableNextColumn();
		ImGui::TextUnformatted( "average" );
		ImGui::TableNextColumn();
		ImGui::TextUnformatted( "min" );
		ImGui::TableNextColumn();
		ImGui::TextUnformatted( "max" );
		ImGui::TableNextColumn();
		ImGui::TextUnformatted( "last" );

		ImGui::TableNextRow();

		ImGui::TableNextColumn();
		ImGui::Text( "%03.4f", stats->cpuAvg );
		ImGui::TableNextColumn();

		if ( stats->cpuNewMin ) {
			ImGui::TextColored( ImVec4( g_color_table[ ColorIndex( S_COLOR_GREEN ) ] ), "%03.4f", stats->cpuMin );
			stats->cpuNewMin = qfalse;
		} else {
			ImGui::Text( "%03.4f", stats->cpuMin );
		}

		ImGui::TableNextColumn();

		if ( stats->cpuNewMax ) {
			ImGui::TextColored( ImVec4( g_color_table[ ColorIndex( S_COLOR_RED ) ] ), "%03.4f", stats->cpuMax );
			stats->cpuNewMax = qfalse;
		} else {
			ImGui::Text( "%03.4f", stats->cpuMax );
		}
		
		ImGui::TableNextColumn();
		ImGui::Text( "%03.4f", stats->cpuPrevious );
	}
	ImGui::EndTable();
}

void Sys_DisplayEngineStats( void )
{
	const int windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground
						| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoTitleBar;
	extern ImFont *RobotoMono;
	ImVec2 windowPos;

	if ( !ui_diagnostics->i ) {
		return;
	}

	if ( ui->GetState() == STATE_CREDITS || ui->GetState() == STATE_LEGAL || ImGui::IsWindowCollapsed() ) {
		// pay respects, don't block the words

		// if its in the legal section, just don't draw it

		// if we're collapsing, we'll segfault when drawing CPU usage
		return;
	}
	
	ImGui::Begin( "Engine Diagnostics", NULL, windowFlags );

	windowPos.x = 730 * ui->scale + ui->bias;
	windowPos.y = 16 * ui->scale;
	ImGui::SetWindowPos( windowPos );

	if ( RobotoMono ) {
		FontCache()->SetActiveFont( RobotoMono );
	}
	const float fontScale = ImGui::GetFont()->Scale;
	ImGui::SetWindowFontScale( ( ImGui::GetFont()->Scale * 0.75f ) * ui->scale );

	if ( !stats ) {
		stats = (sys_stats_t *)Hunk_Alloc( sizeof(sys_stats_t), h_low );
	}

	// draw the cpu usage chart
	if ( ui_diagnostics->i == 1 ) {
		Sys_GetCPUStats();
		Sys_DrawCPUUsage();
		return;
	}
	// draw memory statistics
	else if ( ui_diagnostics->i == 2 ) {
		Sys_GetMemoryUsage( stats );

		Sys_DrawMemoryUsage();
		return;
	}

	//
	// fetch the data
	//

	Sys_GetCPUStats();
	Sys_GetMemoryUsage( stats );
	
	//
	// draw EVERYTHING
	//
	UI_DrawFPS();

	ImGui::Text( "Frame Number: %lu", com_frameNumber );

	Sys_DrawCPUUsage();
	Sys_DrawMemoryUsage();
	Sys_DrawGPUStats();

	ImGui::SeparatorText( "Computer Information" );
	
	ImGui::Text( "%ix%i", gi.gpuConfig.vidWidth, gi.gpuConfig.vidHeight );
	ImGui::Text( "%s", ui->GetConfig().version_string );
	ImGui::Text( "%s", ui->GetConfig().vendor_string );
	ImGui::Text( "%s", ui->GetConfig().renderer_string );
	ImGui::Text( "%s", ui_cpuString->s );

	ImGui::SetWindowFontScale( fontScale );

	ImGui::End();
}
*/
