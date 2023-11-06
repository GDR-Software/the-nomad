#include "../engine/n_shared.h"
#include "g_game.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include "../rendercommon/imstb_rectpack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../rendercommon/imstb_truetype.h"
#include "../rendercommon/imgui.h"
#include "../rendercommon/r_public.h"


#define  DEFAULT_CONSOLE_WIDTH 78
#define  MAX_CONSOLE_WIDTH 120

#define  NUM_CON_TIMES  4

#define  CON_TEXTSIZE   65536

uint32_t bigchar_width;
uint32_t bigchar_height;
uint32_t smallchar_width;
uint32_t smallchar_height;

typedef struct {
	qboolean	initialized;

	int16_t text[CON_TEXTSIZE];
	uint32_t bytesUsed;
	uint32_t current;		// line where next message will be printed
	uint32_t x;				// offset in current line for next print
	uint32_t display;		// bottom of console displays this line

	uint32_t 	linewidth;		// characters across screen
	uint32_t 	totallines;		// total lines in console scrollback

	float	xadjust;		// for wide aspect screens

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	uint32_t vislines;		// in scanlines

	uint32_t times[NUM_CON_TIMES];	// gi.realtime time the line was generated
								// for transparent notify lines
	vec4_t	color;

	uint32_t viswidth;
	uint32_t vispage;		

	qboolean newline;
} console_t;

console_t con;

file_t logfile = FS_INVALID_HANDLE;
field_t g_consoleField;
cvar_t *con_conspeed;
cvar_t *con_autoclear;
cvar_t *con_notifytime;
cvar_t *con_scale;
cvar_t *con_color;
cvar_t *con_noprint;
cvar_t *g_conXOffset;

uint32_t g_console_field_width;

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f( void ) {
	// Can't toggle the console when it's the only thing available
    if ( Key_GetCatcher() == KEYCATCH_CONSOLE ) {
		return;
	}

	if ( con_autoclear->i ) {
		Field_Clear( &g_consoleField );
	}

	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify();

	if (Key_GetCatcher() & KEYCATCH_CONSOLE) {
		Key_SetCatcher(Key_GetCatcher() & ~KEYCATCH_CONSOLE);
		Con_DPrintf("Toggle Console OFF\n");
	}
	else {
		Key_SetCatcher(Key_GetCatcher() | KEYCATCH_CONSOLE);
		Con_DPrintf("Toggle Console ON\n");
	}
}


/*
================
Con_Clear_f
================
*/
static void Con_Clear_f( void ) {
	memset(con.text, 0, sizeof(con.text));
	con.bytesUsed = 0;
}

						
/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
static void Con_Dump_f( void )
{
	uint32_t i;
	file_t	f;
	uint32_t bufferlen;
	char	*buffer;
	char	filename[ MAX_OSPATH ];
	const char *ext;

	if ( Cmd_Argc() != 2 )
	{
		Con_Printf( "usage: condump <filename>\n" );
		return;
	}

	N_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".txt" );

	if ( !FS_AllowedExtension( filename, qfalse, &ext ) ) {
		Con_Printf( "%s: Invalid filename extension '%s'.\n", __func__, ext );
		return;
	}

	f = FS_FOpenWrite( filename );
	if ( f == FS_INVALID_HANDLE )
	{
		Con_Printf( "ERROR: couldn't open %s.\n", filename );
		return;
	}

	Con_Printf( "Dumped console text to %s.\n", filename );
	
	bufferlen = sizeof(con.text);
	buffer = (char *)Hunk_AllocateTempMemory( bufferlen );

	// write the remaining lines
	buffer[ bufferlen - 1 ] = '\0';

	for ( i = 0; i < bufferlen ; i++ )  {
		buffer[i] = con.text[i] & 0xff;

		if (buffer[i] == 0) {
			FS_Write( buffer, strlen( buffer ), f );
			break;
		}
	}

	Hunk_FreeTempMemory( buffer );
	FS_FClose( f );
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	uint32_t i;
	
	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		con.times[i] = 0;
	}
}


/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize( void )
{
	uint32_t i, j, width, oldwidth, oldtotallines, oldcurrent, numlines, numchars;
	char tbuf[CON_TEXTSIZE], *src, *dst;
	static uint32_t old_width, old_vispage;
	uint32_t vispage;
	float	scale;

	if ( con.viswidth == gi.gpuConfig.vidWidth && !con_scale->modified ) {
		return;
	}

	scale = con_scale->f;

	con.viswidth = gi.gpuConfig.vidWidth;

	smallchar_width = SMALLCHAR_WIDTH * scale * gi.con_factor;
	smallchar_height = SMALLCHAR_HEIGHT * scale * gi.con_factor;
	bigchar_width = BIGCHAR_WIDTH * scale * gi.con_factor;
	bigchar_height = BIGCHAR_HEIGHT * scale * gi.con_factor;

	if (!smallchar_width) {
		smallchar_width = SMALLCHAR_WIDTH;
	}
	if (!smallchar_height) {
		smallchar_height = SMALLCHAR_HEIGHT;
	}

#if 0
	if ( gi.gpuConfig.vidWidth == 0 ) // video hasn't been initialized yet
	{
		g_console_field_width = DEFAULT_CONSOLE_WIDTH;
		width = DEFAULT_CONSOLE_WIDTH * scale;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		con.vispage = 4;

		Con_Clear_f();
	}
	else
	{
		width = ((gi.gpuConfig.vidWidth / smallchar_width) - 2);

		g_console_field_width = width;
		g_consoleField.widthInChars = g_console_field_width;

		if ( width > MAX_CONSOLE_WIDTH )
			width = MAX_CONSOLE_WIDTH;

		vispage = gi.gpuConfig.vidHeight / ( smallchar_height * 2 ) - 1;

		if ( old_vispage == vispage && old_width == width )
			return;

		oldwidth = con.linewidth;
		oldtotallines = con.totallines;
		oldcurrent = con.current;

		con.linewidth = 1024;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		con.vispage = vispage;

		old_vispage = vispage;
		old_width = width;

		numchars = oldwidth;
		if ( numchars > con.linewidth )
			numchars = con.linewidth;

		if ( oldcurrent > oldtotallines )
			numlines = oldtotallines;	
		else
			numlines = oldcurrent + 1;	

		if ( numlines > con.totallines )
			numlines = con.totallines;

		memcpy( tbuf, con.text, CON_TEXTSIZE * sizeof( short ) );

		for ( i = 0; i < CON_TEXTSIZE; i++ ) 
			con.text[i] = (ColorIndex(S_COLOR_WHITE)<<8) | ' ';

		for ( i = 0; i < numlines; i++ )
		{
			src = &tbuf[ ((oldcurrent - i + oldtotallines) % oldtotallines) * oldwidth ];
			dst = &con.text[ (numlines - 1 - i) * con.linewidth ];
			for ( j = 0; j < numchars; j++ )
				*dst++ = *src++;
		}

		Con_ClearNotify();

		con.current = numlines - 1;
	}
#endif

	con.display = con.current;

	con_scale->modified = qfalse;
}


/*
==================
Cmd_CompleteTxtName
==================
*/
static void Cmd_CompleteTxtName(const char *args, uint32_t argNum ) {
	if ( argNum == 2 ) {
		Field_CompleteFilename( "", "txt", qfalse, FS_MATCH_EXTERN );
	}
}


/*
================
Con_Init
================
*/
void Con_Init( void ) 
{
	con_notifytime = Cvar_Get( "con_notifytime", "3", 0 );
	Cvar_SetDescription( con_notifytime, "Defines how long messages (from players or the system) are on the screen (in seconds)." );
	con_conspeed = Cvar_Get( "scr_conspeed", "3", 0 );
	Cvar_SetDescription( con_conspeed, "Console opening/closing scroll speed." );
	con_autoclear = Cvar_Get("con_autoclear", "1", CVAR_ARCHIVE_ND);
	Cvar_SetDescription( con_autoclear, "Enable/disable clearing console input text when console is closed." );
	con_scale = Cvar_Get( "con_scale", "1", CVAR_ARCHIVE_ND );
	Cvar_CheckRange( con_scale, "0.5", "8", CVT_FLOAT );
	Cvar_SetDescription( con_scale, "Console font size scale." );
	con_noprint = Cvar_Get( "con_noprint", "0", CVAR_LATCH );
	Cvar_CheckRange( con_noprint, "0", "1", CVT_INT );
	Cvar_SetDescription( con_noprint, "Toggles logging to ingame console." );

	g_conXOffset = Cvar_Get ("g_conXOffset", "0", 0);
	Cvar_SetDescription( g_conXOffset, "Console notifications X-offset." );
	con_color = Cvar_Get( "con_color", "", 0 );
	Cvar_SetDescription( con_color, "Console background color, set as R G B A values from 0-255, use with \\sets to save in config." );

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;

	memset(&con, 0, sizeof(con));

	Cmd_AddCommand( "clear", Con_Clear_f );
	Cmd_AddCommand( "condump", Con_Dump_f );
	Cmd_SetCommandCompletionFunc( "condump", Cmd_CompleteTxtName );
	Cmd_AddCommand( "toggleconsole", Con_ToggleConsole_f );
}


/*
================
Con_Shutdown
================
*/
void Con_Shutdown( void )
{
	Cmd_RemoveCommand( "clear" );
	Cmd_RemoveCommand( "condump" );
	Cmd_RemoveCommand( "toggleconsole" );
}


/*
===============
Con_Fixup
===============
*/
#if 0
static void Con_Fixup( void ) 
{
	uint32_t filled;

	if ( con.current >= con.totallines ) {
		filled = con.totallines;
	} else {
		filled = con.current + 1;
	}

	if ( filled <= con.vispage ) {
		con.display = con.current;
	} else if ( con.current - con.display > filled - con.vispage ) {
		con.display = con.current - filled + con.vispage;
	} else if ( con.display > con.current ) {
		con.display = con.current;
	}
}


/*
===============
Con_Linefeed

Move to newline only when we _really_ need this
===============
*/
static void Con_NewLine( void )
{
	int16_t *s;
	uint32_t i;

	// follow last line
	if ( con.display == con.current )
		con.display++;
	con.current++;

	s = &con.text[ ( con.current % con.totallines ) * con.linewidth ];
	for ( i = 0; i < con.linewidth ; i++ ) 
		*s++ = (ColorIndex(S_COLOR_WHITE)<<8) | ' ';

	con.x = 0;
}


/*
===============
Con_Linefeed
===============
*/
static void Con_Linefeed( qboolean skipnotify )
{
	// mark time for transparent overlay
	if ( con.current >= 0 )	{
		if ( skipnotify )
			con.times[ con.current % NUM_CON_TIMES ] = 0;
		else
			con.times[ con.current % NUM_CON_TIMES ] = gi.realtime;
	}

	if ( con.newline ) {
		Con_NewLine();
	} else {
		con.newline = qtrue;
		con.x = 0;
	}

	Con_Fixup();
}
#endif


/*
================
G_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void G_ConsolePrint( const char *txt ) {
	qboolean skipnotify = qfalse;
	uint64_t len;
	int colorIndex;
	int c;

	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !N_strncmp( txt, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		txt += 12;
	}

	// for some demos we don't want to ever show anything on the console
	if ( con_noprint && con_noprint->i ) {
		return;
	}
	
	if ( !con.initialized ) {
		static cvar_t null_cvar = { 0 };
		con.color[0] =
		con.color[1] =
		con.color[2] =
		con.color[3] = 1.0f;
		con.viswidth = -9999;
		gi.con_factor = 1.0f;
		con_scale = &null_cvar;
		con_scale->f = 1.0f;
		con_scale->modified = qtrue;
		//Con_CheckResize();
		con.initialized = qtrue;
	}

	while ((c = *txt) != 0) {
		if (Q_IsColorString( txt) && *(txt+1) != '\n') {
			colorIndex = ColorIndexFromChar(*(txt+1));
			txt += 2;
			continue;
		}

		txt++;

		if (con.bytesUsed + 1 >= CON_TEXTSIZE) {
			Con_Clear_f();
		}

		// display character and advance
		con.text[con.bytesUsed] = (colorIndex << 8) | (c & 255);
		con.bytesUsed++;
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


static int Con_TextCallback( ImGuiInputTextCallbackData *data )
{
	if (Key_IsDown(KEY_LCTRL) && Key_IsDown(KEY_A)) {
		data->SelectAll();
	}
	if (Key_IsDown(KEY_DELETE)) {
		if (data->HasSelection()) {
			data->DeleteChars(data->SelectionStart, data->SelectionEnd - data->SelectionStart);
		}
		else {
			data->DeleteChars(data->SelectionEnd, 1);
		}
		g_consoleField.cursor = data->CursorPos;
	}
	// paste function
	if (Key_IsDown(KEY_LCTRL) && Key_IsDown(KEY_V)) {
		char *buf = Sys_GetClipboardData();
		int length = strlen(buf);

		if (buf) {
			if (length > sizeof(g_consoleField.buffer)) {
				length = sizeof(g_consoleField.buffer);
			}
			if (data->HasSelection()) {
				memcpy(&data->Buf[data->SelectionStart], buf, length);
				data->SelectionEnd = data->SelectionStart;
			}
			else {
				if (data->CursorPos + length > sizeof(g_consoleField.buffer)) {
					length = sizeof(g_consoleField.buffer) - data->CursorPos;
					memcpy(&data->Buf[data->CursorPos], buf, length);
					data->CursorPos = length;
				}
			}
			Z_Free(buf);

			g_consoleField.cursor = data->CursorPos;
		}
	}

	// ctrl-L clears screen
	if ( Key_IsDown(KEY_L) && Key_IsDown(KEY_LCTRL) ) {
		Cbuf_AddText( "clear\n" );
		return 1;
	}

	// command completion

	if (Key_IsDown(KEY_TAB)) {
		Field_AutoComplete(&g_consoleField);
		if (g_consoleField.cursor != data->CursorPos) {
			N_strncpyz(data->Buf, g_consoleField.buffer, sizeof(g_consoleField.buffer));
			data->CursorPos = strlen(g_consoleField.buffer);
		}
		return 1;
	}

	// command history (ctrl-p ctrl-n for unix style)

	if ( (Key_IsDown(KEY_WHEEL_UP) && Key_IsDown(KEY_LSHIFT) ) || ( Key_IsDown(KEY_UP) ) ||
		 ( ( Key_IsDown(KEY_P) ) && Key_IsDown(KEY_LCTRL) ) ) {
		if (data->HasSelection()) {
			data->ClearSelection();
			g_consoleField.cursor = data->CursorPos;
		}
		if (Con_HistoryGetPrev( &g_consoleField )) {
			memcpy(data->Buf, g_consoleField.buffer, sizeof(g_consoleField.buffer));
			data->CursorPos = g_consoleField.cursor;
		}
		g_consoleField.widthInChars = g_console_field_width;
	}

	if ( (Key_IsDown(KEY_WHEEL_DOWN) && Key_IsDown(KEY_LSHIFT)) || ( Key_IsDown(KEY_DOWN) ) ||
		 ( ( Key_IsDown(KEY_N) ) && keys[KEY_LCTRL].down ) ) {
		if (data->HasSelection()) {
			data->ClearSelection();
			g_consoleField.cursor = data->CursorPos;
		}
		else {
			if (Con_HistoryGetNext(&g_consoleField)) {
				memcpy(data->Buf, g_consoleField.buffer, sizeof(g_consoleField.buffer));
				data->CursorPos = g_consoleField.cursor;
			}
		}
		g_consoleField.widthInChars = g_console_field_width;
	}

	return 1;
}

/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
static void Con_DrawInput( void ) {
	uint32_t y;

	if ( !(Key_GetCatcher( ) & KEYCATCH_CONSOLE ) ) {
		return;
	}

	y = con.vislines - ( smallchar_height * 2 );

	ImGui::NewLine();
	ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( con.color ) );
	ImGui::TextUnformatted( "] " );
	ImGui::PopStyleColor();
	ImGui::SameLine();

	if (ImGui::InputText("", g_consoleField.buffer, sizeof(g_consoleField.buffer),
	ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_EnterReturnsTrue,
	Con_TextCallback)) {
		
		// enter finishes the line
		// if not in the game explicitly prepend a slash if needed
		if ( gi.state == GS_LEVEL
			&& g_consoleField.buffer[0] != '\0'
			&& g_consoleField.buffer[0] != '\\'
			&& g_consoleField.buffer[0] != '/' ) {
			char	temp[MAX_EDIT_LINE-1];

			N_strncpyz( temp, g_consoleField.buffer, sizeof( temp ) );
			Com_snprintf( g_consoleField.buffer, sizeof( g_consoleField.buffer ), "\\%s", temp );
			g_consoleField.cursor++;
		}

		Con_Printf( "]%s\n", g_consoleField.buffer );

		// leading slash is an explicit command
		if ( g_consoleField.buffer[0] == '\\' || g_consoleField.buffer[0] == '/' ) {
			Cbuf_AddText( g_consoleField.buffer+1 );	// valid command
			Cbuf_AddText( "\n" );
		} else {
			// other text will be chat messages
			if ( !g_consoleField.buffer[0] ) {
				return;	// empty lines just scroll the console without adding to history
			} else {
//				Cbuf_AddText( "cmd say " );
//				Cbuf_AddText( g_consoleField.buffer );
//				Cbuf_AddText( "\n" );
			}
		}

		// copy line to history buffer
		Con_SaveField( &g_consoleField );

		Field_Clear( &g_consoleField );
		g_consoleField.widthInChars = g_console_field_width;

		if ( gi.state == GS_MENU ) {
			SCR_UpdateScreen ();	// force an update, because the command
		}							// may take some time
	}

//	Field_Draw( &g_consoleField, con.xadjust + 2 * smallchar_width, y,
//		SCREEN_WIDTH - 3 * smallchar_width, qtrue, qtrue );
}


/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
static void Con_DrawSolidConsole( float frac )
{
	static float conColorValue[4] = { 0.0, 0.0, 0.0, 0.0 };
	// for cvar value change tracking
	static char  conColorString[ MAX_CVAR_VALUE ] = { '\0' };
	int currentColorIndex, colorIndex;
	qboolean customColor = qfalse;
	uint32_t i, x;
	uint32_t lines, row, rows;
	char s[2];
	qboolean usedColor = qfalse;
	int16_t *text;
	char buf[ MAX_CVAR_VALUE ], *v[4];

	lines = gi.gpuConfig.vidHeight * 0.5f;

	ImGui::Begin("Command Console", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
	ImGui::SetWindowPos({ 0.0f, 0.0f });
	ImGui::SetWindowSize({ gi.gpuConfig.vidWidth, gi.gpuConfig.vidHeight * 0.5f });
	ImGui::SetWindowFontScale( 1.0f );
	ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, ImVec2( 1.0f, 1.0f ));

	// custom console background color
	if ( con_color->s[0] ) {
		// track changes
		if ( strcmp( con_color->s, conColorString ) )  {
			N_strncpyz( conColorString, con_color->s, sizeof( conColorString ) );
			N_strncpyz( buf, con_color->s, sizeof( buf ) );
			Com_Split( buf, v, 4, ' ' );
			for ( i = 0; i < 4 ; i++ ) {
				conColorValue[ i ] = N_atof( v[ i ] ) / 255.0f;
				if ( conColorValue[ i ] > 1.0f ) {
					conColorValue[ i ] = 1.0f;
				} else if ( conColorValue[ i ] < 0.0f ) {
					conColorValue[ i ] = 0.0f;
				}
			}
		}
		ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( conColorValue ) );
		customColor = qtrue;
	}

	// draw from the bottom up
	{
		// draw arrows to show the buffer is backscrolled
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4( g_color_table[ ColorIndex(S_COLOR_RED) ] ));
		for (i = 0; i < 80; i++) {
			ImGui::TextUnformatted("^");
			ImGui::SameLine();
		}
		ImGui::PopStyleColor();

		ImGui::NewLine();
		ImGui::NewLine();
	}

	currentColorIndex = ColorIndex(S_COLOR_WHITE);

	for (i = 0, text = con.text; i < con.bytesUsed; text++, i++) {
		// track color changes
		colorIndex = (text[i] >> 8) & 63;
		if (currentColorIndex != colorIndex) {
			currentColorIndex = colorIndex;
			if (usedColor) {
				ImGui::PopStyleColor();
				usedColor = qfalse;
			}
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4( g_color_table[ colorIndex ] ));
			usedColor = qtrue;
		}
		text += 2;

		switch (*text) {
		case '\n':
			if (usedColor) {
				ImGui::PopStyleColor();
				currentColorIndex = ColorIndex(S_COLOR_WHITE);
			}
			ImGui::NewLine();
			break;
		case '\r':
			ImGui::SameLine();
			break;
		default:
			s[0] = *text;
			s[1] = 0;
			ImGui::TextWrapped(s);
			ImGui::SameLine();
			break;
		};
	}

	if (customColor) {
		ImGui::PopStyleColor();
	}
	
	Con_DrawInput();

	ImGui::PopStyleVar();
	ImGui::End();
}

static void Con_DrawNotify( void ) {
}

/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void ) {

	// check for console width changes from a vid mode change
	Con_CheckResize();

	if ( Key_GetCatcher() & KEYCATCH_CONSOLE ) {
		Con_DrawSolidConsole( con.displayFrac );
	} else {
		// draw notify lines
		if ( gi.state == GS_LEVEL ) {
			Con_DrawNotify();
		}
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole( void ) 
{
	// decide on the destination height of the console
	if ( Key_GetCatcher( ) & KEYCATCH_CONSOLE )
		con.finalFrac = 0.5;	// half screen
	else
		con.finalFrac = 0.0;	// none visible
	
	// scroll towards the destination height
	if ( con.finalFrac < con.displayFrac )
	{
		con.displayFrac -= con_conspeed->f * gi.realtime * 0.001;
		if ( con.finalFrac > con.displayFrac )
			con.displayFrac = con.finalFrac;

	}
	else if ( con.finalFrac > con.displayFrac )
	{
		con.displayFrac += con_conspeed->f * gi.realtime * 0.001;
		if ( con.finalFrac < con.displayFrac )
			con.displayFrac = con.finalFrac;
	}
}


void Con_PageUp( uint32_t lines )
{
	if ( lines == 0 )
		lines = con.vispage - 2;

	con.display -= lines;
	
//	Con_Fixup();
}


void Con_PageDown( uint32_t lines )
{
	if ( lines == 0 )
		lines = con.vispage - 2;

	con.display += lines;

//	Con_Fixup();
}


void Con_Top( void )
{
	// this is generally incorrect but will be adjusted in Con_Fixup()
	con.display = con.current - con.totallines;

//	Con_Fixup();
}


void Con_Bottom( void )
{
	con.display = con.current;

//	Con_Fixup();
}


void Con_Close( void )
{
//	if ( !com_cl_running->i )
//		return;

	Field_Clear( &g_consoleField );
	Con_ClearNotify();
	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CONSOLE );
	con.finalFrac = 0.0;			// none visible
	con.displayFrac = 0.0;
}