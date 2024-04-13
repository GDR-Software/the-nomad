#include "../engine/n_shared.h"
#include "../game/g_game.h"
#include "ui_public.hpp"
#include "ui_menu.h"
#include "ui_lib.h"

qboolean m_entersound;

qboolean UI_MenuOption( const char *label )
{
	qboolean retn;

    ImGui::TableNextColumn();
    ImGui::TextUnformatted( label );
    ImGui::TableNextColumn();
    ImGui::SameLine();

    retn = ImGui::ArrowButton( label, ImGuiDir_Right );
	if ( retn ) {
		Snd_PlaySfx( ui->sfx_select );
	}

	return retn;
}

qboolean UI_MenuTitle( const char *label, float fontScale )
{
	ImVec2 cursorPos;
	renderSceneRef_t refdef;

	memset( &refdef, 0, sizeof( refdef ) );
	refdef.x = 0;
	refdef.y = 0;
	refdef.width = ui->gpuConfig.vidWidth;
	refdef.height = ui->gpuConfig.vidHeight;
	refdef.time = 0;
	refdef.flags = RSF_NOWORLDMODEL | RSF_ORTHO_TYPE_SCREENSPACE;

	FontCache()->SetActiveFont( AlegreyaSC );

	ImGui::PushStyleColor( ImGuiCol_Text, colorRed );
    ImGui::SetWindowFontScale( fontScale * ui->scale );
    ImGui::TextUnformatted( label );
    ImGui::SetWindowFontScale( 1.0f * ui->scale );
	ImGui::PopStyleColor();

	cursorPos = ImGui::GetCursorScreenPos();

//	ImGui::SetWindowFontScale( 1.5f * scale );
	ImGui::SetCursorScreenPos( ImVec2( 16 * ui->scale, 680 * ui->scale ) );
	if ( ui->menusp > 1 ) {
		ImGui::Image( (ImTextureID)(uintptr_t)( ui->backHovered ? ui->back_1 : ui->back_0 ), ImVec2( 256 * ui->scale, 72 * ui->scale ) );
		ui->backHovered = ImGui::IsItemHovered( ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_DelayNone );
		if ( ImGui::IsItemClicked() ) {
			Snd_PlaySfx( ui->sfx_back );
			return true;
		}
	}
	ImGui::SetCursorScreenPos( cursorPos );

	ImGui::SetWindowFontScale( 1.0f * ui->scale );

    return false;
}

void UI_EscapeMenuToggle( void )
{
    if ( Key_IsDown( KEY_ESCAPE ) ) {
        if ( ui->escapeToggle ) {
            ui->escapeToggle = qfalse;
			UI_PopMenu();

			Snd_PlaySfx( ui->sfx_back );
        }
    }
    else {
        ui->escapeToggle = qtrue;
    }
}

void UI_PushMenu( menuframework_t *menu )
{
	int i;

    // avoid stacking menus invoked by hotkeys
    for ( i = 0; i < ui->menusp; i++ ) {
        if ( ui->stack[i] == menu ) {
            ui->menusp = i;
            break;
        }
    }

    if ( i == ui->menusp ) {
        if ( ui->menusp >= MAX_MENU_DEPTH ) {
            N_Error( ERR_DROP, "UI_PushMenu: menu stack overflow" );
        }
        ui->stack[ui->menusp++] = menu;
    }

    ui->activemenu = menu;

    // default cursor position
//    menu->cursor = 0;
//    menu->cursor_prev = 0;

    Key_SetCatcher( KEYCATCH_UI );
}

void UI_PopMenu( void )
{
    ui->menusp--;

    if ( ui->menusp < 0 ) {
        N_Error( ERR_DROP, "UI_PopMenu: menu stack underflow" );
    }

    if ( ui->menusp ) {
        ui->activemenu = ui->stack[ui->menusp - 1];
    }
    else {
        UI_ForceMenuOff();
    }
}

void UI_ForceMenuOff( void )
{
    ui->menusp = 0;
    ui->activemenu = NULL;

    Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_UI );
    Key_ClearStates();
}

/*
=================
UI_LerpColor
=================
*/
void UI_LerpColor( vec4_t a, vec4_t b, vec4_t c, float t )
{
	int i;

	// lerp and clamp each component
	for (i=0; i<4; i++)
	{
		c[i] = a[i] + t*(b[i]-a[i]);
		if (c[i] < 0)
			c[i] = 0;
		else if (c[i] > 1.0)
			c[i] = 1.0;
	}
}

qboolean UI_IsFullscreen( void ) {
	if ( ui->activemenu && ( Key_GetCatcher() & KEYCATCH_UI ) ) {
		return ui->activemenu->fullscreen;
	}

	return qfalse;
}

void UI_SetActiveMenu( uiMenu_t menu )
{
	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	Menu_Cache();

	ui->menustate = menu;

	switch ( menu ) {
	case UI_MENU_NONE:
		Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_UI );
		Key_ClearStates();
		Cvar_Set( "g_paused", "0" );
		UI_ForceMenuOff();
		break;
	case UI_MENU_PAUSE:
		Cvar_Set( "g_paused", "1" );
		Key_SetCatcher( KEYCATCH_UI );
		UI_PauseMenu();
		break;
	case UI_MENU_MAIN:
		UI_MainMenu();
		break;
	case UI_MENU_DEMO:
		UI_DemoMenu();
		break;
	default:
#ifdef _NOMAD_DEBUG
	    Con_Printf("UI_SetActiveMenu: bad enum %lu\n", menu );
#endif
        break;
	};
}


/*
================
UI_AdjustFrom1024

Adjusted for resolution and screen aspect ratio
================
*/
void UI_AdjustFrom1024( float *x, float *y, float *w, float *h )
{
	// expect valid pointers
	*x *= ui->scale + ui->bias;
	*y *= ui->scale;
	*w *= ui->scale;
	*h *= ui->scale;
}

void UI_DrawNamedPic( float x, float y, float width, float height, const char *picname )
{
	nhandle_t hShader;

	hShader = re.RegisterShader( picname );
	UI_AdjustFrom1024( &x, &y, &width, &height );
	re.DrawImage( x, y, width, height, 0, 0, 1, 1, hShader );
}

void UI_DrawHandlePic( float x, float y, float w, float h, nhandle_t hShader )
{
	float	s0;
	float	s1;
	float	t0;
	float	t1;

	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}
	
	UI_AdjustFrom1024( &x, &y, &w, &h );
	re.DrawImage( x, y, w, h, s0, t0, s1, t1, hShader );
}

/*
================
UI_FillRect

Coordinates are 1024*768 virtual values
=================
*/
void UI_FillRect( float x, float y, float width, float height, const float *color )
{
	re.SetColor( color );

	UI_AdjustFrom1024( &x, &y, &width, &height );
	re.DrawImage( x, y, width, height, 0, 0, 0, 0, ui->whiteShader );

	re.SetColor( NULL );
}

/*
================
UI_DrawRect

Coordinates are 1024*768 virtual values
=================
*/
void UI_DrawRect( float x, float y, float width, float height, const float *color )
{
	re.SetColor( color );

	UI_AdjustFrom1024( &x, &y, &width, &height );

	re.DrawImage( x, y, width, 1, 0, 0, 0, 0, ui->whiteShader );
	re.DrawImage( x, y, 1, height, 0, 0, 0, 0, ui->whiteShader );
	re.DrawImage( x, y + height - 1, width, 1, 0, 0, 0, 0, ui->whiteShader );
	re.DrawImage( x + width - 1, y, 1, height, 0, 0, 0, 0, ui->whiteShader );

	re.SetColor( NULL );
}

void UI_SetColor( const float *rgba ) {
	re.SetColor( rgba );
}

void UI_DrawTextBox( int x, int y, int width, int lines )
{
//	FillRect( x + BIGCHAR_WIDTH/2, y + BIGCHAR_HEIGHT/2, ( width + 1 ) * BIGCHAR_WIDTH, ( lines + 1 ) * BIGCHAR_HEIGHT, colorBlack );
//	DrawRect( x + BIGCHAR_WIDTH/2, y + BIGCHAR_HEIGHT/2, ( width + 1 ) * BIGCHAR_WIDTH, ( lines + 1 ) * BIGCHAR_HEIGHT, colorWhite );
}

