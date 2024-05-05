#include "../game/g_game.h"
#include "ui_public.hpp"
#include "ui_menu.h"
#include "ui_lib.h"
#include "ui_window.h"
#include "ui_string_manager.h"
#include "../rendercommon/imgui_impl_opengl3.h"
#include "../rendercommon/imgui_internal.h"
#include "../game/imgui_memory_editor.h"
#include "../rendercommon/implot.h"
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
    SettingsMenu_Cache();
	ModsMenu_Cache();
	SinglePlayerMenu_Cache();
	DemoMenu_Cache();
	LoadGameMenu_Cache();
	NewGameMenu_Cache();
	PauseMenu_Cache();
	CreditsMenu_Cache();
}

CUIFontCache::CUIFontCache( void ) {
    union {
        void *v;
        char *b;
    } f;
    uint64_t nLength;
	nlohmann::json data;

	Con_Printf( "Initializing font cache...\n" );

	memset( m_FontList, 0, sizeof( m_FontList ) );
	m_pCurrentFont = NULL;

    nLength = FS_LoadFile( "fonts/font_config.json", &f.v );
    if ( !nLength || !f.v ) {
        N_Error( ERR_FATAL, "CUIFontCache::Init: failed to load fonts/font_config.json" );
    }
	try {
		data = nlohmann::json::parse( f.b, f.b + nLength );
	} catch ( const nlohmann::json::exception& e ) {
		N_Error( ERR_FATAL,
			"CUIFontCache::Init: failed to parse fonts/font_config.json, nlohmann::json::exception thrown ->\n"
			"  id: %i\n"
			"  what: %s\n"
		, e.id, e.what() );
	}
	if ( !data.contains( "FontList" ) ) {
		N_Error( ERR_FATAL, "CUIFontCache::Init: font_config missing FontList object array" );
	}

	for ( const auto& it : data.at( "FontList"  ) ) {
		if ( !it.contains( "Name" ) ) {
			N_Error( ERR_FATAL, "CUIFontCache::Init: font config object missing variable 'Name'" );
		}
		const nlohmann::json::string_t& name = it.at( "Name" );

		if ( !it.contains( "Scale" ) ) {
			N_Error( ERR_FATAL, "CUIFontCache::Init: font_config object missing variable 'Scale'" );
		}
		const float scale = it.at( "Scale" );

		if ( !it.contains( "Variants" ) ) {
			N_Error( ERR_FATAL, "CUIFontCache::Init: font_config object missing object array 'Variants'" );
		}

		for ( const auto& var : it.at( "Variants" ) ) {
			const nlohmann::json::string_t& variant = var.get<nlohmann::json::string_t>();
			if ( !AddFontToCache( name.c_str(), variant.c_str(), scale ) ) {
				N_Error( ERR_FATAL, "CUIFontCache::Init: failed to load font data for 'fonts/%s/%s-%s.ttf'", name.c_str(), name.c_str(),
					variant.c_str() );
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

	if ( m_pCurrentFont ) {
		ImGui::PopFont();
	}

	m_pCurrentFont = font;

	ImGui::PushFont( font );
}

void CUIFontCache::SetActiveFont( nhandle_t hFont )
{
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
	Com_snprintf( hashpath, sizeof( hashpath ) - 1, "%s-%s", rpath, variant );

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

	Com_snprintf( hashpath, sizeof( hashpath ) - 1, "%s-%s", rpath, variant );

	path = va( "fonts/%s/%s.ttf", rpath, hashpath );
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
	ui_debugOverlay = Cvar_Get( "ui_debugOverlay", "1", 0 );
#else
	ui_debugOverlay = Cvar_Get( "ui_debugOverlay", "0", 0 );
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
											" 3 - SHOW ME EVERYTHING!!!!\n" );
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

	if ( ui_debugOverlay && ui_debugOverlay->i ) {
		ImPlot::DestroyContext();
	}

    Cmd_RemoveCommand( "ui.cache" );
	Cmd_RemoveCommand( "ui.fontinfo" );
	Cmd_RemoveCommand( "togglepausemenu" );
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
    ImGui::SetWindowPos( ImVec2( 1000 * ui->scale + ui->bias, 8 * ui->scale ) );
    ImGui::SetWindowFontScale( 1.5f * ui->scale );
    ImGui::Text( "%ifps", fps );
    ImGui::End();
}

static uint32_t gpu_BackEndPreviousTimes[FPS_FRAMES];
static int32_t gpu_BackEndFrameTimeIndex;
static uint32_t gpu_BackEndFrameTimePrevious;

static uint32_t gpu_FrontEndPreviousTimes[FPS_FRAMES];
static int32_t gpu_FrontEndFrameTimeIndex;
static uint32_t gpu_FrontEndFrameTimePrevious;

extern "C" uint32_t GPU_CalcFrameTime( uint64_t gpuTime, uint32_t *times, uint32_t *previous, int32_t *index )
{
	uint32_t frameTime;
	uint32_t total, i;
	uint32_t realTime;

	frameTime = gpuTime - *previous;
	*previous = gpuTime;

	times[ *index % FPS_FRAMES ] = frameTime;
	(*index)++;
	if ( *index > FPS_FRAMES ) {
        // average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0; i < FPS_FRAMES; i++ ) {
			total += times[i];
		}
		if ( total == 0 ) {
			total = 1;
		}
		realTime = 1000.0f * FPS_FRAMES / total;
    } else {
		realTime = *previous;
	}

	return realTime;
}

static void UI_DrawDebugOverlay( void )
{
	uint64_t i;

	if ( ImGui::Begin( "Input Debug##DebugOverlayInputSystem" ) ) {
		extern cvar_t *in_joystick;

		if ( in_joystick->i ) {
			ImGui::TextUnformatted( "Joystick: Enabled" );
		} else {
			ImGui::TextUnformatted( "Joystick: Disabled" );
		}

		ImGui::Text( "Input Mode: %s", in_mode->i == 0 ? "Keyboard & Mouse" : "GamePad/Controller" );

		ImGui::End();
	}

	if ( ImGui::Begin( "GPU Debug##DebugOverlayGPUDriver" ) ) {
		char *p;
		char str[256];
		uint32_t gpu_FrameTime, gpu_FrameSamples, gpu_FramePrimitives;
		uint64_t frameTime;
		static uint32_t frameTimes[60];
		for ( i = 0; i < arraylen( frameTimes ); i++ ) {
			frameTimes[i] = i;
		}

		ImGui::Text( "Vendor: %s", ui->gpuConfig.vendor_string );
		ImGui::Text( "Version: %s", ui->gpuConfig.version_string );
		ImGui::Text( "GPU Driver: %s", ui->gpuConfig.renderer_string );
		ImGui::Text( "GPU Renderer: %s", g_renderer->s );
		if ( !N_stricmp( g_renderer->s, "opengl" ) || !N_stricmp( g_renderer->s, "vulkan" ) ) {
			ImGui::Text( "GLSL Version: %s", ui->gpuConfig.shader_version_str );
		} else if ( !N_stricmp( g_renderer->s, "d3d11" ) ) {
			ImGui::Text( "HLSL Version: %s", ui->gpuConfig.shader_version_str );
		}

		re.GetGPUFrameStats( &gpu_FrameTime, &gpu_FrameSamples, &gpu_FramePrimitives );
		frameTime = GPU_CalcFrameTime( time_backend, gpu_BackEndPreviousTimes, &gpu_BackEndFrameTimePrevious, &gpu_BackEndFrameTimeIndex );
		frameTime = GPU_CalcFrameTime( time_frontend, gpu_FrontEndPreviousTimes, &gpu_FrontEndFrameTimePrevious,
			&gpu_FrontEndFrameTimeIndex );

		ImGui::Text( "Frame Primitives: %u", gpu_FramePrimitives );
		ImGui::Text( "Frame Samples: %u", gpu_FrameSamples );
		ImGui::Text( "Frame Time: %u", gpu_FrameTime );
		ImGui::Text( "BackEnd Frame Time: %lu", time_backend );
		ImGui::Text( "FrontEnd Frame Time: %lu", time_frontend );

		if ( ImPlot::BeginPlot( "GPU Profiler" ) ) {
			ImPlot::SetupAxes( "Time (ms)", "Frame Time (MS)" );
			ImPlot::SetupAxesLimits( 0.0f, 1000.0f, 0.0f, 60.0f );
			ImPlot::PushStyleVar( ImPlotStyleVar_FillAlpha, 0.25f );
			ImPlot::PlotShaded( "Renderer FrontEnd Time", frameTimes, gpu_FrontEndPreviousTimes, FPS_FRAMES );
			ImPlot::PlotShaded( "Renderer BackeEnd Time", frameTimes, gpu_BackEndPreviousTimes, FPS_FRAMES );
			ImPlot::PopStyleVar();
			ImPlot::EndPlot();
		}

		ImGui::TextUnformatted( "Extensions Used:" );
		if ( Cvar_VariableInteger( "r_arb_shader_storage_buffer_object" ) ) {
			ImGui::PushStyleColor( ImGuiCol_Text, colorGreen );
			ImGui::TextUnformatted( "  GL_ARB_shader_storage_buffer_object" );
			ImGui::PopStyleColor();
		} else {
			ImGui::PushStyleColor( ImGuiCol_Text, colorRed );
			ImGui::TextUnformatted( "  GL_ARB_shader_storage_buffer_object" );
			ImGui::PopStyleColor();
		}
		if ( Cvar_VariableInteger( "r_arb_map_buffer_range" ) ) {
			ImGui::PushStyleColor( ImGuiCol_Text, colorGreen );
			ImGui::TextUnformatted( "  GL_ARB_map_buffer_range" );
			ImGui::PopStyleColor();
		} else {
			ImGui::PushStyleColor( ImGuiCol_Text, colorGreen );
			ImGui::TextUnformatted( "  GL_ARB_map_buffer_range" );
			ImGui::PopStyleColor();
		}
		if ( Cvar_VariableInteger( "r_arb_framebuffer_object" ) ) {
			ImGui::PushStyleColor( ImGuiCol_Text, colorGreen );
			ImGui::TextUnformatted( "  GL_ARB_framebuffer_object" );
			ImGui::PopStyleColor();
		} else {
			ImGui::PushStyleColor( ImGuiCol_Text, colorRed );
			ImGui::TextUnformatted( "  GL_ARB_framebuffer_object" );
			ImGui::PopStyleColor();
		}
		if ( Cvar_VariableInteger( "r_arb_vertex_buffer_object" ) ) {
			ImGui::PushStyleColor( ImGuiCol_Text, colorGreen );
			ImGui::TextUnformatted( "  GL_ARB_vertex_buffer_object" );
			ImGui::PopStyleColor();
		} else {
			ImGui::PushStyleColor( ImGuiCol_Text, colorRed );
			ImGui::TextUnformatted( "  GL_ARB_vertex_buffer_object" );
			ImGui::PopStyleColor();
		}
		if ( Cvar_VariableInteger( "r_arb_vertex_array_object" ) ) {
			ImGui::PushStyleColor( ImGuiCol_Text, colorGreen );
			ImGui::TextUnformatted( "  GL_ARB_vertex_array_object" );
			ImGui::PopStyleColor();
		} else {
			ImGui::PushStyleColor( ImGuiCol_Text, colorRed );
			ImGui::TextUnformatted( "  GL_ARB_vertex_array_object" );
			ImGui::PopStyleColor();
		}
		if ( Cvar_VariableInteger( "r_arb_sync" ) ) {
			ImGui::PushStyleColor( ImGuiCol_Text, colorGreen );
			ImGui::TextUnformatted( "  GL_ARB_sync" );
			ImGui::PopStyleColor();
		} else {
			ImGui::PushStyleColor( ImGuiCol_Text, colorRed );
			ImGui::TextUnformatted( "  GL_ARB_sync" );
			ImGui::PopStyleColor();
		}
		if ( Cvar_VariableInteger( "r_arb_texture_float" ) ) {
			ImGui::PushStyleColor( ImGuiCol_Text, colorGreen );
			ImGui::TextUnformatted( "  GL_ARB_texture_float" );
			ImGui::PopStyleColor();
		} else {
			ImGui::PushStyleColor( ImGuiCol_Text, colorRed );
			ImGui::TextUnformatted( "  GL_ARB_texture_float" );
			ImGui::PopStyleColor();
		}
		if ( Cvar_VariableInteger( "r_arb_texture_filter_anisotropic" ) ) {
			ImGui::PushStyleColor( ImGuiCol_Text, colorGreen );
			ImGui::TextUnformatted( "  GL_ARB_texture_filter_anisotropic" );
			ImGui::PopStyleColor();
		} else {
			ImGui::PushStyleColor( ImGuiCol_Text, colorRed );
			ImGui::TextUnformatted( "  GL_ARB_texture_filter_anisotropic" );
			ImGui::PopStyleColor();
		}

		ImGui::TextUnformatted( "Extensions Found:" );
		p = str;
		for ( i = 0; i < sizeof( ui->gpuConfig.extensions_string ); i++ ) {
			if ( !ui->gpuConfig.extensions_string[i] ) {
				break;
			} else if ( ui->gpuConfig.extensions_string[i] == ' ' ) {
				*p = '\0';
				ImGui::Text( "  %s", str );
				p = str;
			} else {
				*p++ = ui->gpuConfig.extensions_string[i];
			}
		}
		ImGui::End();
	}

	if ( ImGui::Begin( "System Debug##DebugOverlaySystemData" ) ) {
		uint64_t virtMem, physMem, peakVirt, peakPhys;
		double cpuFreq;
		const char *cpuFreqSuffix;
		static int32_t frameTimes[60];
		for ( i = 0; i < arraylen( frameTimes ); i++ ) {
			frameTimes[i] = i;
		}

		Sys_GetRAMUsage( &virtMem, &physMem, &peakVirt, &peakPhys );
		cpuFreq = Sys_CalculateCPUFreq();

		ImGui::Text( "CPU Processor: %s", ui_cpuString->s );
		ImGui::Text( "CPU Cores: %i", SDL_GetCPUCount() );
		ImGui::Text( "CPU CacheLine: %lu", com_cacheLine );

		cpuFreqSuffix = "Hz";
		if ( cpuFreq > 1000.0f ) {
			cpuFreq /= 1000;
			cpuFreqSuffix = "KHz";
		}
		if ( cpuFreq > 1000.0f ) {
			cpuFreq /= 1000.0f;
			cpuFreqSuffix = "MHz";
		}
		if ( cpuFreq > 1000.0f ) {
			cpuFreq /= 1000.0f;
			cpuFreqSuffix = "GHz";
		}
		ImGui::Text( "CPU: %lf %s", cpuFreq, cpuFreqSuffix );

		ImGui::Text( "Stack Memory: %lu", Sys_StackMemoryRemaining() );
		ImGui::Text( "Current Virtual Memory: %lu", virtMem );
		ImGui::Text( "Current Physical Memory: %lu", physMem );
		ImGui::Text( "Peak Virtual Memory: %lu", peakVirt );
		ImGui::Text( "Peak Physical Memory: %lu", peakPhys );

		if ( ImPlot::BeginPlot( "Profiler" ) ) {
			ImPlot::SetupAxes( "Time (ms)", "Frame Time (Engine)" );
			ImPlot::SetupAxesLimits( 0.0f, 1000.0f, 0.0f, 60.0f );
			ImPlot::PushStyleVar( ImPlotStyleVar_FillAlpha, 0.25f );
			ImPlot::PlotShaded( "Frame Time", frameTimes, previousTimes, FPS_FRAMES );
			ImPlot::PopStyleVar();
			ImPlot::EndPlot();
		}
		
		ImGui::End();
	}

	if ( ImGui::Begin( "ModuleLib Debug##DebugOverlayModuleLib" ) ) {
		const UtlVector<CModuleInfo *>& loadList = g_pModuleLib->GetLoadList();

		ImGui::Text( "ModuleCount: %lu", loadList.size() );
		for ( i = 0; i < loadList.size(); i++ ) {
			ImGui::Text( "  [%lu]: %s v%i.%i.%i", i, loadList[i]->m_szName, loadList[i]->m_nModVersionMajor,
				loadList[i]->m_nModVersionUpdate, loadList[i]->m_nModVersionPatch );
		}
		ImGui::End();
	}
}

void UI_EscapeMenuToggle( void )
{
    if ( ( Key_IsDown( KEY_ESCAPE ) || Key_IsDown( KEY_PAD0_B ) ) &&  ui->menusp > 1 ) {
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
    strManager->LoadFile( va( "scripts/ui_strings_%s.txt", ui_language->s ) );
    if ( !strManager->NumLangsLoaded() ) {
        N_Error( ERR_DROP, "UI_Init: no language loaded" );
    }

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

	// cache redundant calulations
	re.GetConfig( &ui->gpuConfig );

	// for 640x480 virtualized screen
	ui->scale = ui->gpuConfig.vidHeight * ( 1.0f / 768.0f );
	if ( ui->gpuConfig.vidWidth * 768.0f > ui->gpuConfig.vidHeight * 1024.0f ) {
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

	if ( ui_debugOverlay->i ) {
		ImPlot::CreateContext();
	}

    // add commands
    Cmd_AddCommand( "ui.cache", UI_Cache_f );
	Cmd_AddCommand( "ui.fontinfo", CUIFontCache::ListFonts_f );
	Cmd_AddCommand( "togglepausemenu", UI_PauseMenu_f );
}

void Menu_Cache( void )
{
    ui->whiteShader = re.RegisterShader( "white" );
	ui->back_0 = re.RegisterShader( "menu/backbutton0" );
	ui->back_1 = re.RegisterShader( "menu/backbutton1" );
	ui->sfx_select = Snd_RegisterSfx( "sfx/menu1.wav" );
	ui->sfx_back = Snd_RegisterSfx( "sfx/menu3.wav" );
	ui->sfx_null = Snd_RegisterSfx( "sfx/menu4.wav" );

	ui->backdrop = re.RegisterShader( "menu/mainbackdrop" );

    // IT MUST BE THERE!
    if ( !FS_LoadFile( "textures/coconut.jpg", NULL ) || ui->backdrop == FS_INVALID_HANDLE ) {
        N_Error( ERR_FATAL, "Menu_Cache: could not load coconut.jpg" );
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
	refdef.flags = RSF_NOWORLDMODEL | RSF_ORTHO_TYPE_SCREENSPACE;

	//
	// draw the background
	//
    re.ClearScene();
    re.DrawImage( 0, 0, refdef.width, refdef.height, 0, 0, 1, 1, ui->menubackShader );
    re.RenderScene( &refdef );
}

extern "C" void UI_AddJoystickKeyEvents( void )
{
	ImGuiIO& io = ImGui::GetIO();
	
	io.AddKeyEvent( ImGuiKey_DownArrow, Key_IsDown( KEY_PAD0_LEFTSTICK_DOWN ) );
	io.AddKeyEvent( ImGuiKey_UpArrow, Key_IsDown( KEY_PAD0_LEFTSTICK_UP ) );
	io.AddKeyEvent( ImGuiKey_LeftArrow, Key_IsDown( KEY_PAD0_LEFTSTICK_LEFT ) );
	io.AddKeyEvent( ImGuiKey_RightArrow, Key_IsDown( KEY_PAD0_LEFTSTICK_RIGHT ) );
}

extern "C" void UI_Refresh( int32_t realtime )
{
	extern cvar_t *in_joystick;

	ui->realtime = realtime;
	ui->frametime = ui->frametime - realtime;

	UI_DrawFPS();

//	if ( !ui_active->i ) {
//		UI_EscapeMenuToggle();
//		if ( ui->menustate != UI_MENU_NONE ) {
//			UI_SetActiveMenu( UI_MENU_PAUSE );
//		}
//		return;
//	}

	{
        refdef_t refdef;

        memset( &refdef, 0, sizeof( refdef ) );
        refdef.x = 0;
        refdef.y = 0;
        refdef.width = ui->gpuConfig.vidWidth;
        refdef.height = ui->gpuConfig.vidHeight;
        refdef.time = 0;
        refdef.flags = RSF_ORTHO_TYPE_SCREENSPACE | RSF_NOWORLDMODEL;

        re.ClearScene();
        re.DrawImage( 0, 0, refdef.width, refdef.height, 0, 0, 1, 1, ui->backdrop );
		re.RenderScene( &refdef );
    }

	if ( !( Key_GetCatcher() & KEYCATCH_UI ) ) {
		return;
	}

	if ( ui_debugOverlay->i ) {
		UI_DrawDebugOverlay();
	}

	if ( in_joystick->i ) {
		UI_AddJoystickKeyEvents();
	}

	if ( ui->activemenu ) {
		if ( ui->activemenu->fullscreen ) {
			UI_DrawMenuBackground();
		}
		if ( ui->activemenu->track != FS_INVALID_HANDLE ) {
			Snd_SetLoopingTrack( ui->activemenu->track );
		}

		if ( !( Key_GetCatcher() & KEYCATCH_CONSOLE ) ) {
			if ( ui->activemenu->draw ) {
				ui->activemenu->draw();
			} else {
				Menu_Draw( ui->activemenu );
			}
		}

//		if( ui->GetFirstDraw() ) {
//			ui->MouseEvent( 0, 0 );
//			ui->SetFirstDraw( qfalse );
//		}
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
