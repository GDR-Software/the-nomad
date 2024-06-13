#include "ui_lib.h"

#define ID_CONFIRM_NO  0
#define ID_CONFIRM_YES 1

typedef struct {
	menuframework_t menu;
	
	menutext_t yes;
	menutext_t no;
	
	const char *question;
	void (*draw)( void );
	void (*action)( qboolean result );
	
	const char **lines;
} confirmMenu_t;

static confirmMenu_t *s_confirm;

static void ConfirmMenu_Event( void *ptr, int event )
{
	qboolean result;
	
	if ( event != EVENT_ACTIVATED ) {
		return;
	}
	
	UI_PopMenu();
	
	if ( ( (menucommon_t *)ptr )->id == ID_CONFIRM_NO ) {
		result = qfalse;
	} else {
		result = qtrue;
	}
	
	if ( s_confirm->action ) {
		s_confirm->action( result );
	}
}

static void MessageMenu_Draw( void )
{
	int flags;
	int i;
	
	ImGui::Begin( "##ConfirmMenuMainMenu", NULL, s_confirm->menu.flags );
	ImGui::SetWindowFocus();
	ImGui::SetWindowPos( ImVec2( s_confirm->menu.x, s_confirm->menu.y ) );
	ImGui::SetWindowSize( ImVec2( s_confirm->menu.width, s_confirm->menu.height ) );

	UI_EscapeMenuToggle();
	if ( UI_MenuTitle( s_confirm->menu.name, s_confirm->menu.titleFontScale ) ) {
		UI_PopMenu();
		Snd_PlaySfx( ui->sfx_back );

		ImGui::End();
		return;
	}

	for ( i = 0; s_confirm->lines[i]; i++ ) {
		ImGui::TextUnformatted( s_confirm->lines[i] );
	}

	if ( s_confirm->draw ) {
		s_confirm->draw();
	}

	ImGui::End();
}

static void ConfirmMenu_Draw( void )
{
	Menu_Draw( &s_confirm->menu );

    if ( Key_IsDown( KEY_ENTER ) || Key_IsDown( KEY_Y ) ) {
        ConfirmMenu_Event( &s_confirm->yes, EVENT_ACTIVATED );
    }
    if ( Key_IsDown( KEY_ESCAPE ) || Key_IsDown( KEY_N ) ) {
        ConfirmMenu_Event( &s_confirm->no, EVENT_ACTIVATED );
    }
	
	if ( s_confirm->draw ) {
		s_confirm->draw();
	}
}

void ConfirmMenu_Cache( void )
{
	if ( !ui->uiAllocated ) {
		s_confirm = (confirmMenu_t *)Hunk_Alloc( sizeof( *s_confirm ), h_high );
	}
	memset( s_confirm, 0, sizeof( *s_confirm ) );
}

void UI_ConfirmMenu( const char *question, void (*draw)( void ), void (*action)( qboolean result ) )
{
	int n1, n2, n3;
	int l1, l2, l3;

	ConfirmMenu_Cache();

	n1 = ImGui::CalcTextSize( "YES/NO" ).x;
	n2 = ImGui::CalcTextSize( "YES" ).x + 3;
	n3 = ImGui::CalcTextSize( "/" ).x + 3;
	l1 = 528 - ( n1 / 2 );
	l2 = l1 + n2;
	l3 = l2 + n3;
	
	s_confirm->question = question;
	s_confirm->draw = draw;
	s_confirm->action = action;

	s_confirm->menu.draw = ConfirmMenu_Draw;

	if ( gi.state == GS_LEVEL ) {
		s_confirm->menu.fullscreen = qfalse;
	} else {
		s_confirm->menu.fullscreen = qtrue;
	}

	s_confirm->yes.generic.type = MTYPE_TEXT;
	s_confirm->yes.generic.flags = QMF_HIGHLIGHT_IF_FOCUS;
	s_confirm->yes.generic.eventcallback = ConfirmMenu_Event;
	s_confirm->yes.generic.id = ID_CONFIRM_YES;
	s_confirm->yes.text = "YES";
	s_confirm->yes.color = color_red;

	s_confirm->no.generic.type = MTYPE_TEXT;
	s_confirm->no.generic.flags = QMF_HIGHLIGHT_IF_FOCUS;
	s_confirm->no.generic.eventcallback = ConfirmMenu_Event;
	s_confirm->no.generic.id = ID_CONFIRM_NO;
	s_confirm->no.text = "NO";
	s_confirm->no.color = color_red;
	
	Menu_AddItem( &s_confirm->menu, &s_confirm->yes );

	UI_PushMenu( &s_confirm->menu );
}

/*
=================
UI_Message
hacked over from Confirm stuff
=================
*/
void UI_Message( const char **lines )
{
	int length;

	length = 528 - ( ImGui::CalcTextSize( "OK" ).x / 2 );

	ConfirmMenu_Cache();

	s_confirm->lines = lines;

	s_confirm->menu.name = "";
	s_confirm->menu.fullscreen = qtrue;
	s_confirm->menu.width = 640 * ui->scale;
	s_confirm->menu.height = 480 * ui->scale;
	s_confirm->menu.x = length * ui->scale;
	s_confirm->menu.y = 268 * ui->scale;
	s_confirm->menu.draw = MessageMenu_Draw;

	if ( gi.state == GS_LEVEL ) {
		s_confirm->menu.fullscreen = qfalse;
	} else {
		s_confirm->menu.fullscreen = qtrue;
	}
	
	s_confirm->yes.generic.type = MTYPE_TEXT;
	s_confirm->yes.generic.flags = QMF_HIGHLIGHT_IF_FOCUS | QMF_SAMELINE_NEXT;
	s_confirm->yes.generic.eventcallback = ConfirmMenu_Event;
	s_confirm->yes.generic.id = ID_CONFIRM_YES;
	s_confirm->yes.text = "OK";
	s_confirm->yes.color = color_red;
	
	Menu_AddItem( &s_confirm->menu, &s_confirm->yes );
	Menu_AddItem( &s_confirm->menu, &s_confirm->no );
	
	UI_PushMenu( &s_confirm->menu );
}
