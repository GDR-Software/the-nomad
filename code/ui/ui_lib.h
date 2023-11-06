#ifndef __UI_LIB__
#define __UI_LIB__

#pragma once

#define MAX_MENU_DEPTH 8

#define RCOLUMN_OFFSET			( BIGCHAR_WIDTH )
#define LCOLUMN_OFFSET			(-BIGCHAR_WIDTH )

#define SLIDER_RANGE			10

#define MTYPE_NULL				0
#define MTYPE_SLIDER			1	
#define MTYPE_ACTION			2
#define MTYPE_SPINCONTROL		3
#define MTYPE_FIELD				4
#define MTYPE_RADIOBUTTON		5
#define MTYPE_BITMAP			6	
#define MTYPE_TEXT				7
#define MTYPE_SCROLLLIST		8
#define MTYPE_PTEXT				9
#define MTYPE_BTEXT				10

#define QMF_BLINK				0x00000001
#define QMF_SMALLFONT			0x00000002
#define QMF_LEFT_JUSTIFY		0x00000004
#define QMF_CENTER_JUSTIFY		0x00000008
#define QMF_RIGHT_JUSTIFY		0x00000010
#define QMF_NUMBERSONLY			0x00000020	// edit field is only numbers
#define QMF_HIGHLIGHT			0x00000040
#define QMF_HIGHLIGHT_IF_FOCUS	0x00000080	// steady focus
#define QMF_PULSEIFFOCUS		0x00000100	// pulse if focus
#define QMF_HASMOUSEFOCUS		0x00000200
#define QMF_NOONOFFTEXT			0x00000400
#define QMF_MOUSEONLY			0x00000800	// only mouse input allowed
#define QMF_HIDDEN				0x00001000	// skips drawing
#define QMF_GRAYED				0x00002000	// grays and disables
#define QMF_INACTIVE			0x00004000	// disables any input
#define QMF_NODEFAULTINIT		0x00008000	// skip default initialization
#define QMF_OWNERDRAW			0x00010000
#define QMF_PULSE				0x00020000
#define QMF_LOWERCASE			0x00040000	// edit field is all lower case
#define QMF_UPPERCASE			0x00080000	// edit field is all upper case
#define QMF_SILENT				0x00100000

// callback notifications
#define EVENT_GOTFOCUS				1
#define EVENT_LOSTFOCUS			    2
#define EVENT_ACTIVATED			    3

#include "ui_menu.h"

class CUILib
{
public:
    CUILib( void ) = default;
    ~CUILib() = default;

    void Init( void );
    void Shutdown( void );
    
    void PushMenu( CUIMenu *menu );
    void PopMenu( void );
    void ForceMenuOff( void );

    void AdjustFrom640( float *x, float *y, float *w, float *h ) const;
    void DrawNamedPic( float x, float y, float width, float height, const char *picname ) const;
    void DrawHandlePic( float x, float y, float w, float h, nhandle_t hShader ) const;
    void FillRect( float x, float y, float width, float height, const float *color ) const;
    void DrawRect( float x, float y, float width, float height, const float *color ) const;
    void SetColor( const float *rgba ) const;
    void Refresh( uint64_t realtime );
    qboolean CursorInRect( int x, int y, int width, int height ) const;
    void DrawTextBox( int x, int y, int width, int lines ) const;
    void DrawString( int x, int y, const char *str, int style, vec4_t color ) const;
    void DrawMenu( void ) const;
    void DrawChar( int x, int y, int ch, int style, vec4_t color ) const;
    void DrawProportionalString_AutoWrapped( int x, int y, int xmax, int ystep, const char* str, int style, vec4_t color ) const;
    void DrawProportionalString( int x, int y, const char* str, int style, vec4_t color ) const;
    qboolean IsFullscreen( void ) const;
    int ProportionalStringWidth( const char* str ) const;
    float ProportionalSizeScale( int style ) const;
    void DrawBannerString( int x, int y, const char* str, int style, vec4_t color ) const;
    void SetActiveMenu( uiMenu_t menu );

    void MouseEvent( uint32_t dx, uint32_t dy );
    void KeyEvent( uint32_t key, qboolean down );

    CUIMenu *GetCurrentMenu( void ) {
        return curmenu;
    }
    const CUIMenu *GetCurrentMenu( void ) const {
        return curmenu;
    }

    int GetFrameTime( void ) const {
        return frametime;
    }
    int GetRealTime( void ) const {
        return realtime;
    }

    void SetFrameTime( uint64_t n ) {
        frametime = n;
    }
    void SetRealTime (uint64_t n ) {
        realtime = n;
    }

    qboolean GetFirstDraw( void ) const {
        return firstdraw;
    }
    void SetFirstDraw( qboolean yas ) {
        firstdraw = yas;
    }

    int GetCursorX( void ) const {
        return cursorx;
    }
    int GetCursorY( void ) const {
        return cursory;
    }

    int GetDebug( void ) const {
        return debug;
    }
    int IsDebug( void ) const {
        return debug;
    }
    void SetDebug( int i ) {
        debug = i;
    }

    const gpuConfig_t& GetConfig( void ) const {
        return gpuConfig;
    }

public:
    nhandle_t whiteShader;
    nhandle_t menubackShader;
    nhandle_t charset;
    nhandle_t rb_on;
    nhandle_t rb_off;
private:
    void DrawString2( int x, int y, const char* str, vec4_t color, int charw, int charh ) const;
    void DrawBannerString2( int x, int y, const char* str, vec4_t color ) const;
    void DrawProportionalString2( int x, int y, const char* str, vec4_t color, float sizeScale, nhandle_t charset ) const;

    CUIMenu *stack[MAX_MENU_DEPTH];
    CUIMenu *curmenu;

    int menusp;
    qboolean firstdraw;
    int debug;

    float scale;
    float bias;

    int cursorx;
    int cursory;

    int frametime;
    int realtime;

    gpuConfig_t gpuConfig;
};

extern CUILib *ui;
extern qboolean m_entersound;

// cvars
extern cvar_t *ui_language;
extern cvar_t *ui_printStrings;

extern const char *UI_LangToString( int32_t lang );

extern void			Menu_Cache( void );
extern void			Menu_Focus( CUIMenuWidget *m );
extern void			Menu_AddItem( CUIMenuWidget *menu, void *item );
extern void			Menu_AdjustCursor( CUIMenuWidget *menu, int dir );
extern void			Menu_Draw( CUIMenu *menu );
extern void			*Menu_ItemAtCursor( CUIMenu *m );
extern sfxHandle_t	Menu_ActivateItem( CUIMenuWidget *s, CUIMenuWidget* item );
extern void			Menu_SetCursor( CUIMenu *m, int cursor );
extern void			Menu_SetCursorToItem( CUIMenu  *m, void* ptr );
extern sfxHandle_t	Menu_DefaultKey( CUIMenu *s, uint32_t key );
extern void			Bitmap_Init( mbitmap_t *b );
extern void			Bitmap_Draw( mbitmap_t *b );
extern void			ScrollList_Draw( mlist_t *l );
extern sfxHandle_t	ScrollList_Key( mlist_t *l, uint32_t key );
extern sfxHandle_t	menu_in_sound;
extern sfxHandle_t	menu_move_sound;
extern sfxHandle_t	menu_out_sound;
extern sfxHandle_t	menu_buzz_sound;
extern sfxHandle_t	menu_null_sound;
extern sfxHandle_t	weaponChangeSound;
extern vec4_t		menu_text_color;
extern vec4_t		menu_grayed_color;
extern vec4_t		menu_dark_color;
extern vec4_t		menu_highlight_color;
extern vec4_t		menu_red_color;
extern vec4_t		menu_black_color;
extern vec4_t		menu_dim_color;
extern vec4_t		color_black;
extern vec4_t		color_white;
extern vec4_t		color_yellow;
extern vec4_t		color_blue;
extern vec4_t		color_orange;
extern vec4_t		color_red;
extern vec4_t		color_dim;
extern vec4_t		name_color;
extern vec4_t		list_color;
extern vec4_t		listbar_color;
extern vec4_t		text_color_disabled; 
extern vec4_t		text_color_normal;
extern vec4_t		text_color_highlight;

//
// ui_mfield.cpp
//
extern void			MField_Clear( mfield_t *edit );
extern void			MField_KeyDownEvent( mfield_t *edit, uint32_t key );
extern void			MField_CharEvent( mfield_t *edit, int ch );
extern void			MField_Draw( mfield_t *edit, int x, int y, int style, vec4_t color );
extern void			MenuField_Init( mfield_t *m );
extern void			MenuField_Draw( mfield_t *f );
extern sfxHandle_t	MenuField_Key( mfield_t* m, uint32_t* key );

//
// ui_title.cpp
//
extern void         UI_TitleMenu( void );
extern void         TitleMenu_Cache( void );

//
// ui_settings.cpp
//
extern void         UI_SettingsMenu( void );
extern void         SettingsMenu_Cache( void );

#include "ui_defs.h"

#endif