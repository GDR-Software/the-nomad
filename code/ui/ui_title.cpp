#include "../game/g_game.h"
#include "ui_public.hpp"
#include "ui_menu.h"
#include "ui_lib.h"
#include "ui_window.h"
#include "ui_string_manager.h"

typedef struct {
    CUIMenu menu;

    const stringHash_t *thenomad;
    const stringHash_t *enterGame;

    ImFont *font;
} titlemenu_t;

static titlemenu_t title;

static void DrawStringCentered( const char *str )
{
    uint64_t length;
    float font_size;

    length = strlen( str );
    font_size = ImGui::GetFontSize() * length / 2;
    ImGui::SameLine( ImGui::GetWindowSize().x / 2.0f - font_size + (font_size / 2.15) );
    ImGui::TextUnformatted( str, str + length );
}

static void TitleMenu_Draw( void )
{
    float font_scale;
    uint32_t i;
    const int windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize;

    font_scale = ImGui::GetFont()->Scale;
    FontCache()->SetActiveFont( title.font );

    // setup window
    ImGui::Begin( "TitleMenu", NULL, windowFlags );
    ImGui::SetWindowFontScale( ( font_scale * 4.5f ) * ui->scale );
    ImGui::SetWindowPos( ImVec2( 300 * ui->scale, 64 * ui->scale ) );
    ImGui::TextUnformatted( title.thenomad->value );
    ImGui::End();

    ImGui::Begin( "TitleMenu2", NULL, windowFlags );
    ImGui::SetWindowFontScale( ( font_scale * 1.25f ) * ui->scale );
    ImGui::SetWindowPos( ImVec2( 360 * ui->scale, 700 * ui->scale ) );
    ImGui::TextUnformatted( title.enterGame->value );
    ImGui::End();

    // if the console's open, don't catch
    if ( Key_GetCatcher() & KEYCATCH_CONSOLE ) {
        return;
    }
    // exit?
    if ( Key_IsDown( KEY_ESCAPE ) ) {
        Cbuf_AddText( "quit\n" );
    }

    // is a key down?
    if ( Key_AnyDown() ) {
        ui->SetActiveMenu( UI_MENU_MAIN );
    }
}

void TitleMenu_Cache( void )
{
    memset(&title, 0, sizeof(title));

    title.menu.Draw = TitleMenu_Draw;

    title.font = FontCache()->AddFontToCache( "PressStart2P", "Regular" );

    title.thenomad = strManager->ValueForKey("MENU_LOGO_STRING");
    title.enterGame = strManager->ValueForKey("MENU_TITLE_ENTER_GAME");
}

void UI_TitleMenu( void )
{
    TitleMenu_Cache();

    ui->PushMenu( &title.menu );
}
