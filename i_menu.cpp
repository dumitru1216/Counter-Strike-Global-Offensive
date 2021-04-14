#include "horizon.hpp"
#include "i_menu.hpp"
#include "parser.hpp"
#include "source.hpp"
//#include "background_img.hpp"
//#include "imgui_extension.hpp"
#include "rendering.hpp"
#include "menu.hpp"
#include "misc.hpp"
#include "menu/setup/settings.h"
#include "rage_aimbot.hpp"

#pragma region "Key"
inline std::string get_kname( int key ) {
	switch ( key ) {
	case VK_LBUTTON:
		return "mouse1";
	case VK_RBUTTON:
		return "mouse2";
	case VK_CANCEL:
		return "break";
	case VK_MBUTTON:
		return "mouse3";
	case VK_XBUTTON1:
		return "mouse4";
	case VK_XBUTTON2:
		return "mouse5";
	case VK_BACK:
		return "backspace";
	case VK_TAB:
		return "tab";
	case VK_CLEAR:
		return "clear";
	case VK_RETURN:
		return "enter";
	case VK_SHIFT:
		return "shift";
	case VK_CONTROL:
		return "ctrl";
	case VK_MENU:
		return "alt";
	case VK_PAUSE:
		return "[19]";
	case VK_CAPITAL:
		return "capslock";
	case VK_SPACE:
		return "space";
	case VK_PRIOR:
		return "pgup";
	case VK_NEXT:
		return "pgdown";
	case VK_END:
		return "end";
	case VK_HOME:
		return "home";
	case VK_LEFT:
		return "left";
	case VK_UP:
		return "up";
	case VK_RIGHT:
		return "right";
	case VK_DOWN:
		return "down";
	case VK_SELECT:
		return "select";
	case VK_INSERT:
		return "insert";
	case VK_DELETE:
		return "delete";
	case '0':
		return "0";
	case '1':
		return "1";
	case '2':
		return "2";
	case '3':
		return "3";
	case '4':
		return "4";
	case '5':
		return "5";
	case '6':
		return "6";
	case '7':
		return "7";
	case '8':
		return "8";
	case '9':
		return "9";
	case 'A':
		return "a";
	case 'B':
		return "b";
	case 'C':
		return "c";
	case 'D':
		return "d";
	case 'E':
		return "e";
	case 'F':
		return "f";
	case 'G':
		return "g";
	case 'H':
		return "h";
	case 'I':
		return "i";
	case 'J':
		return "j";
	case 'K':
		return "k";
	case 'L':
		return "l";
	case 'M':
		return "m";
	case 'N':
		return "n";
	case 'O':
		return "o";
	case 'P':
		return "p";
	case 'Q':
		return "q";
	case 'R':
		return "r";
	case 'S':
		return "s";
	case 'T':
		return "t";
	case 'U':
		return "u";
	case 'V':
		return "v";
	case 'W':
		return "w";
	case 'X':
		return "x";
	case 'Y':
		return "y";
	case 'Z':
		return "z";
	case VK_LWIN:
		return "left win";
	case VK_RWIN:
		return "right win";
	case VK_NUMPAD0:
		return "num 0";
	case VK_NUMPAD1:
		return "num 1";
	case VK_NUMPAD2:
		return "num 2";
	case VK_NUMPAD3:
		return "num 3";
	case VK_NUMPAD4:
		return "num 4";
	case VK_NUMPAD5:
		return "num 5";
	case VK_NUMPAD6:
		return "num 6";
	case VK_NUMPAD7:
		return "num 7";
	case VK_NUMPAD8:
		return "num 8";
	case VK_NUMPAD9:
		return "num 9";
	case VK_MULTIPLY:
		return "num mult";
	case VK_ADD:
		return "num add";
	case VK_SEPARATOR:
		return "|";
	case VK_SUBTRACT:
		return "num sub";
	case VK_DECIMAL:
		return "num decimal";
	case VK_DIVIDE:
		return "num divide";
	case VK_F1:
		return "f1";
	case VK_F2:
		return "f2";
	case VK_F3:
		return "f3";
	case VK_F4:
		return "f4";
	case VK_F5:
		return "f5";
	case VK_F6:
		return "f6";
	case VK_F7:
		return "f7";
	case VK_F8:
		return "f8";
	case VK_F9:
		return "f9";
	case VK_F10:
		return "f10";
	case VK_F11:
		return "f11";
	case VK_F12:
		return "f12";
	case VK_NUMLOCK:
		return "num lock";
	case VK_SCROLL:
		return "break";
	case VK_LSHIFT:
		return "shift";
	case VK_RSHIFT:
		return "shift";
	case VK_LCONTROL:
		return "ctrl";
	case VK_RCONTROL:
		return "ctrl";
	case VK_LMENU:
		return "alt";
	case VK_RMENU:
		return "alt";
	case VK_OEM_COMMA:
		return "),";
	case VK_OEM_PERIOD:
		return ".";
	case VK_OEM_1:
		return ";";
	case VK_OEM_MINUS:
		return "-";
	case VK_OEM_PLUS:
		return "=";
	case VK_OEM_2:
		return "/";
	case VK_OEM_3:
		return "grave";
	case VK_OEM_4:
		return "[";
	case VK_OEM_5:
		return "\\";
	case VK_OEM_6:
		return "]";
	case VK_OEM_7:
		return "[222]";
	default:
		return "";
	}
}
#pragma endregion

template <typename T>
static bool items_getter( void* data, int idx, const char** out_text )
{
	auto items = ( T* )data;
	if ( out_text ) {
		*out_text = items[ idx ].item_name.data( );//std::string(items[idx].name.begin(), items[idx].translated_name.end()).data();
	}
	return true;
};

int tabcount = 0;

struct s_tab
{
	const char* name = "";
	const char* icon = 0;
	ImVec2 size = ImVec2( 0, 0 );
	int num = 0;

	s_tab( const char* _name, const char* _icon = "", ImVec2 _size = ImVec2( 0, 0 ) )
	{
		name = _name;
		icon = _icon;
		size = _size;
		num = tabcount;
		tabcount++;
	}

	~s_tab( )
	{
		/*tabcount--;
		name = "";
		icon = 0;
		size = ImVec2(0, 0);
		num = 0;*/
	}
};

namespace ImGui
{
	bool ColorButton( const char* desc_id, const float col[ ], ImGuiColorEditFlags flags, ImVec2 size )
	{
		return ImGui::ColorButton( desc_id, ImColor( col[ 0 ], col[ 1 ], col[ 2 ], col[ 3 ] ), flags, size );
	}

	ImGuiID Colorpicker_Close = 0;
	__inline void CloseLeftoverPicker( ) { if ( Colorpicker_Close ) ImGui::ClosePopup( Colorpicker_Close ); }
	bool ColorPickerBox( const char* picker_idname, float col_ct[ ], float col_t[ ], float col_ct_invis[ ], float col_t_invis[ ], bool alpha = true, bool use_buttons = false )
	{
		bool ret = false;

		ImGui::SameLine( );
		static bool switch_entity_teams = false;
		static bool switch_color_vis = false;

		float* col = use_buttons ? ( switch_entity_teams ? ( switch_color_vis ? col_t : col_t_invis ) : ( switch_color_vis ? col_ct : col_ct_invis ) ) : col_ct;

		const auto cursor_pos_y = GetCursorPosY( );

		SetCursorPosY( cursor_pos_y + 5 );
		bool open_popup = ImGui::ColorButton( picker_idname, col, ImGuiColorEditFlags_NoSidePreview
			| ImGuiColorEditFlags_NoSmallPreview
			| ImGuiColorEditFlags_AlphaPreview
			| ImGuiColorEditFlags_NoTooltip
			| ImGuiColorEditFlags_ColorPicker
			| ImGuiColorEditFlags_ColorPickerSameline, ImVec2( 18, 10 ) );

		SetCursorPosY( GetCursorPosY( ) - 5 );

		/*SetCursorPosY(cursor_pos_y);*/
		if ( open_popup ) {
			ImGui::OpenPopup( picker_idname );
			Colorpicker_Close = ImGui::GetID( picker_idname );
		}

		if ( ImGui::BeginPopup( picker_idname ) )
		{
			if ( use_buttons ) {
				const char* button_name0 = switch_entity_teams ? "Terrorists" : "Counter-Terrorists";
				if ( ImGui::Button( button_name0, ImVec2( -1, 0 ) ) )
					switch_entity_teams = !switch_entity_teams;

				const char* button_name1 = switch_color_vis ? "Visible" : "Invisible";
				if ( ImGui::Button( button_name1, ImVec2( -1, 0 ) ) )
					switch_color_vis = !switch_color_vis;
			}

			std::string id_new = picker_idname;
			id_new += "##pickeritself_";

			ret = ImGui::ColorPicker( id_new.c_str( ), col, ( alpha ? ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaBar : 0 ) );
			ImGui::EndPopup( );
		}

		return ret;
	}

	bool mouse_in_pos( ImRect minmaxs )
	{
		ImGuiContext& g = *GImGui;
		return ( g.IO.MousePos.x >= minmaxs.Min.x && g.IO.MousePos.y >= minmaxs.Min.y && g.IO.MousePos.x <= minmaxs.Max.x && g.IO.MousePos.y <= minmaxs.Max.y );
	}

	bool Tab( const char* label, const ImVec2& size_arg, bool selected )
	{
		return ButtonEx( label, size_arg, ( selected ? ImGuiItemFlags_TabButton : 0 ) );
	}

	// NB: This is an internal helper. The user-facing IsItemHovered() is using data emitted from ItemAdd(), with a slightly different logic.
	bool IsHovered( const ImRect& bb, ImGuiID id, bool flatten_childs )
	{
		ImGuiContext& g = *GImGui;
		if ( g.HoveredId == 0 || g.HoveredId == id || g.HoveredIdAllowOverlap )
		{
			ImGuiWindow* window = GetCurrentWindowRead( );
			if ( g.HoveredWindow == window || ( flatten_childs && g.HoveredRootWindow == window->RootWindow ) )
				if ( ( g.ActiveId == 0 || g.ActiveId == id || g.ActiveIdAllowOverlap ) && IsMouseHoveringRect( bb.Min, bb.Max ) )
					return true;
		}
		return false;
	}


	bool Bind( const char* label, int* key, const ImVec2& size_arg )
	{
		ImGuiWindow* window = GetCurrentWindow( );

		if ( window->SkipItems )
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiIO& io = g.IO;
		const ImGuiStyle& style = g.Style;

		const ImGuiID id = window->GetID( label );

		ImVec2 label_size = CalcTextSize( label, NULL, true );
		ImVec2 size = CalcItemSize( size_arg, CalcItemWidth( ) / 2, label_size.y + style.FramePadding.y );// *2.0f);

		const ImRect frame_bb( window->DC.CursorPos, window->DC.CursorPos + size );
		const ImRect total_bb( frame_bb.Min, frame_bb.Max + ImVec2( label_size.x > 0.0f ? ( style.ItemInnerSpacing.x + label_size.x ) : 0.0f, 0.0f ) );

		ItemSize( total_bb, style.FramePadding.y );

		if ( !ItemAdd( total_bb, id ) )
			return false;

		bool value_changed = false;

		std::string text = "None";

		bool hovered = false, held = false;

		if ( g.ActiveId == id )
		{
			text = "Press a Key";

			if ( !g.ActiveIdIsJustActivated )
			{
				for ( int i = 0; i < 255; i++ )
				{
					if ( ctx.pressed_keys[ i ] )//g.IO.KeysReleased[i])
					{
						SetActiveID( 0, window );
						*key = ( i == 0x1B/*VK_ESCAPE*/ ) ? 0 : i;
						value_changed = true;
						break;
					}
				}

				/*for (int i = 0; i < 6; i++)
				{
					if (g.IO.MouseReleased[i])
					{
						SetActiveID(0);
						*key = i + 1;
						value_changed = true;
						break;
					}
				}*/
			}
		}
		else
		{
			hovered = IsHovered( frame_bb, id, false );

			if ( hovered )
			{
				SetHoveredID( id );

				if ( g.IO.MouseDown[ 0 ] )
				{
					held = true;
					FocusWindow( window );
				}
				else if ( g.IO.MouseReleased[ 0 ] )
				{
					SetActiveID( id, window );
				}
			}

			text = get_kname( *key );
		}

		const ImU32 col = GetColorU32( ( hovered && held ) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg );
		RenderFrame( frame_bb.Min, frame_bb.Max, col, true, style.FrameRounding ); // main frame
		label_size = CalcTextSize( text.c_str( ), NULL, true );
		ImVec2 pos_min = frame_bb.Min;
		ImVec2 pos_max = frame_bb.Max;
		ImVec2 label_pos = pos_min;
		label_pos.x = ImMax( label_pos.x, ( label_pos.x + pos_max.x - label_size.x ) * 0.5f );
		label_pos.y = ImMax( label_pos.y, ( label_pos.y + pos_max.y - label_size.y ) * 0.5f );
		RenderText( label_pos, text.c_str( ), NULL );
		RenderText( ImVec2( frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y - 3 ), label ); // render item lable

		return value_changed;
	}
}

namespace menu
{
	LPDIRECT3DTEXTURE9 flag_ewropi = NULL;
	LPDIRECT3DTEXTURE9 m_tplayer_with_glow = NULL;
	LPDIRECT3DTEXTURE9 m_tplayer_no_glow = NULL;
	IDirect3DDevice9* m_device = nullptr;

	int category = -1;
	int new_category = -1;

	void init( const float& alpha )
	{
		auto style = &ImGui::GetStyle( );
		ImVec4* colors = style->Colors;


		const auto main_color = ImColor( 255, 0, 0 );
		const auto main_color_fade = ImColor( 255, 0, 0 );

		colors[ ImGuiCol_Text ] = ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );
		colors[ ImGuiCol_TextDisabled ] = ImVec4( 0.50f, 0.50f, 0.50f, 1.00f );
		colors[ ImGuiCol_WindowBg ] = ImColor( 15, 15, 15 );
		colors[ ImGuiCol_PopupBg ] = ImVec4( 0.08f, 0.08f, 0.08f, 0.94f );
		colors[ ImGuiCol_Border ] = ImVec4( 33 / 255.f, 34 / 255.f, 36 / 255.f, 1.0f );
		colors[ ImGuiCol_BorderShadow ] = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
		colors[ ImGuiCol_ChildWindowBg ] = ImVec4( 20 / 255.f, 20 / 255.f, 20 / 255.f, 1.0f );
		colors[ ImGuiCol_FrameBg ] = ImVec4( 33 / 255.f, 33 / 255.f, 33 / 255.f, 1.0f );
		colors[ ImGuiCol_FrameBgHovered ] = ImVec4( 62 / 255.f, 62 / 255.f, 62 / 255.f, 1.0f );
		colors[ ImGuiCol_FrameBgActive ] = ImVec4( 62 / 255.f, 62 / 255.f, 62 / 255.f, 1.0f );
		colors[ ImGuiCol_TitleBgActive ] = ImVec4( 35 / 255.f, 35 / 255.f, 35 / 255.f, 1.0f );
		colors[ ImGuiCol_TitleBg ] = ImVec4( 35 / 255.f, 35 / 255.f, 35 / 255.f, 1.0f );
		colors[ ImGuiCol_TitleBgCollapsed ] = ImVec4( 35 / 255.f, 35 / 255.f, 35 / 255.f, 1.0f );
		colors[ ImGuiCol_MenuBarBg ] = ImVec4( 0.14f, 0.14f, 0.14f, 1.00f );
		//colors[ ImGuiCol_CheckMark ] = ImColor( ctx.m_settings.f_menu_color[0], ctx.m_settings.f_menu_color[ 1 ], ctx.m_settings.f_menu_color[ 2 ] );
		colors[ ImGuiCol_ScrollbarBg ] = ImVec4( 0.17f, 0.17f, 0.17f, 0.00f );
		colors[ ImGuiCol_ScrollbarGrab ] = ImVec4( 0.25f, 0.25f, 0.25f, 0.00f );
		colors[ ImGuiCol_ScrollbarGrabHovered ] = ImVec4( 0.25f, 0.25f, 0.25f, 0.00f );
		colors[ ImGuiCol_ScrollbarGrabActive ] = ImVec4( 0.25f, 0.25f, 0.25f, 0.00f );
		colors[ ImGuiCol_Button ] = ImColor( 15, 15, 15 );
		colors[ ImGuiCol_ButtonHovered ] = ImColor( 25, 25, 25 ); //
		colors[ ImGuiCol_ButtonActive ] = ImColor( 35, 35, 35 ); //
		colors[ ImGuiCol_Header ] = ImVec4( 167 / 255.f, 24 / 255.f, 71 / 255.f, 1.0f ); //multicombo, combo selected item color.
		colors[ ImGuiCol_HeaderHovered ] = ImVec4( 35 / 255.f, 35 / 255.f, 35 / 255.f, 1.0f );
		colors[ ImGuiCol_HeaderActive ] = ImVec4( 35 / 255.f, 35 / 255.f, 35 / 255.f, 1.0f );
		colors[ ImGuiCol_Separator ] = ImVec4( 0, 0, 0, 1 );
		colors[ ImGuiCol_SeparatorHovered ] = ImVec4( 0, 0, 0, 1 );
		colors[ ImGuiCol_SeparatorActive ] = ImVec4( 0, 0, 0, 1 );
		colors[ ImGuiCol_ResizeGrip ] = ImVec4( 0.26f, 0.59f, 0.98f, 0.25f );
		colors[ ImGuiCol_ResizeGripHovered ] = ImVec4( 0.26f, 0.59f, 0.98f, 0.67f );
		colors[ ImGuiCol_ResizeGripActive ] = ImVec4( 0.26f, 0.59f, 0.98f, 0.95f );
		colors[ ImGuiCol_PlotLines ] = ImVec4( 0.61f, 0.61f, 0.61f, 1.00f );
		colors[ ImGuiCol_PlotLinesHovered ] = ImVec4( 1.00f, 0.43f, 0.35f, 1.00f );
		colors[ ImGuiCol_PlotHistogram ] = ImVec4( 0.90f, 0.70f, 0.00f, 1.00f );
		colors[ ImGuiCol_PlotHistogramHovered ] = ImVec4( 1.00f, 0.60f, 0.00f, 1.00f );
		colors[ ImGuiCol_TextSelectedBg ] = ImVec4( 0.26f, 0.59f, 0.98f, 0.35f );
		colors[ ImGuiCol_CloseButton ] = ImVec4( 0, 0, 0, 0 );
		colors[ ImGuiCol_CloseButtonHovered ] = ImVec4( 0, 0, 0, 0 );
		//colors[ ImGuiCol_HotkeyOutline ] = ImVec4( 0, 0, 0, 0 );

		colors[ ImGuiCol_Tab ] = ImVec4( 0.26f, 0.59f, 0.98f, 0.40f * alpha );
		colors[ ImGuiCol_TabHovered ] = ImVec4( 0.29f, 0.70f, 0.99f, 0.40f * alpha );
		colors[ ImGuiCol_TabActive ] = ImVec4( 0.26f, 0.59f, 0.98f, 0.67f * alpha );
		colors[ ImGuiCol_TabText ] = ImVec4( 0.50f, 0.50f, 0.50f, alpha );
		colors[ ImGuiCol_TabTextActive ] = ImVec4( 1.00f, 1.00f, 1.00f, alpha );
		colors[ ImGuiCol_TabSelected ] = ImVec4( 0.06f, 0.53f, 0.98f, alpha );

		style->ScrollbarSize = 14.f;
		style->FrameRounding = 0;
		style->WindowRounding = 0.0f;
		style->WindowTitleAlign = ImVec2( 0.5f, 0.5f );
		style->FramePadding = ImVec2( 8, 4 );
	}

	void skins_listbox( const char* label, std::string filter, int selected_rarity, int* current_item, skin_list_t& skins, int height_in_items, const float& alpha )
	{
		if ( !ImGui::ListBoxHeader( label, skins.list.size( ), height_in_items ) )
			return;

		for ( int i = 0; i < skins.list.size( ); i++ )
		{
			const auto item_selected = ( i == *current_item );
			const auto item_text = skins.list[ i ].translated_name;
			auto rarity = parser::rarities.get_by_id( skins.list[ i ].rarity );
			auto color = rarity.Color.alpha( alpha * 255 );

			if ( selected_rarity != 0 &&
				selected_rarity != rarity.id )
				continue;

			std::string pstr = std::string( item_text.begin( ), item_text.end( ) );

			if ( !filter.empty( ) &&
				parser::to_lower( pstr ).find( parser::to_lower( filter ) ) == std::string::npos )
				continue;

			ImGui::PushID( i );
			ImGui::PushStyleColor( ImGuiCol_Text, ( ImVec4 )color );
			if ( ImGui::Selectable( pstr.data( ), item_selected ) )
			{
				*current_item = i;
			}
			ImGui::PopStyleColor( );
			ImGui::PopID( );
		}

		ImGui::ListBoxFooter( );
	}

	bool add_tab( s_tab tab, int* var, ImVec2 override_size = ImVec2( -1, -1 ), bool hovered = true )
	{
		bool ret = false;
		const auto allowed_to_use_icon = strlen( tab.icon ) > 0;

		if ( allowed_to_use_icon && !hovered )
			ImGui::PushFont( d::fonts::menu_icons.imgui );
		else
			ImGui::PushFont( d::fonts::menu_tab.imgui );

		if ( ImGui::Tab( ( allowed_to_use_icon && !hovered ? tab.icon : tab.name ), override_size == ImVec2( -1, -1 ) ? tab.size : override_size, *var == tab.num ) )
		{
			ret = true;

			if (/*is_clickable && */var != nullptr )
				*var = tab.num;
		}

		ImGui::PopFont( );

		//ImGui::SameLine(0);
		return ret;
	}

	std::list<int> get_spec( int playerId )
	{
		std::list<int> list;

		if ( !csgo.m_engine( )->IsInGame( ) )
			return list;

		auto player = csgo.m_entity_list( )->GetClientEntity( playerId );
		if ( !player )
			return list;

		if ( player->IsDead( ) )
		{
			auto observerTarget = player->m_hObserverTarget( ) ? csgo.m_entity_list( )->GetClientEntityFromHandle( *player->m_hObserverTarget( ) ) : nullptr;
			if ( !observerTarget )
				return list;

			player = observerTarget;
		}

		for ( int i = 1; i < csgo.m_globals( )->maxClients; i++ )
		{
			auto pPlayer = csgo.m_entity_list( )->GetClientEntity( i );

			if ( !pPlayer )
				continue;

			if ( pPlayer->IsDormant( ) || !pPlayer->IsDead( ) )
				continue;

			auto target = pPlayer->m_hObserverTarget( ) ? csgo.m_entity_list( )->GetClientEntityFromHandle( *pPlayer->m_hObserverTarget( ) ) : nullptr;
			if ( player != target )
				continue;

			list.push_back( i );
		}

		return list;
	}

	void speclist( )
	{
		if ( !ctx.m_settings.misc_spectators_list )
			return;

		ImGuiStyle& style = ImGui::GetStyle( );
		//float oldAlpha = style.Colors[ImGuiCol_WindowBg].w;
		//style.Colors[ImGuiCol_WindowBg].w = Vars.Visuals.SpectatorListAlpha / 255.0f;

		if ( ImGui::Begin( sxor( "spectator list" ), &ctx.m_settings.misc_spectators_list, ImVec2( 300, 300 ), 1.f, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_ShowBorders ) )// |  ImGuiWindowFlags_ShowBorders))
		{
			ImGui::TextColored( ImVec4( style.Colors[ ImGuiCol_Text ].x - 0.3f, style.Colors[ ImGuiCol_Text ].y - 0.3f, style.Colors[ ImGuiCol_Text ].z - 0.3f, style.Colors[ ImGuiCol_Text ].w ), sxor( "spectators:" ) );
			ImGui::Separator( );
			if ( csgo.m_engine( )->IsInGame( ) ) {
				for ( int playerId : get_spec( csgo.m_engine( )->GetLocalPlayer( ) ) )
				{
					auto player = csgo.m_entity_list( )->GetClientEntity( playerId );

					player_info entityInformation;
					csgo.m_engine( )->GetPlayerInfo( playerId, &entityInformation );

					std::string name( entityInformation.name );

					if ( name.size( ) > 31 )
						name.resize( 31 );

					ImGui::Text( name.c_str( ) );
				}
			}
			//ImGui::Separator();

			ImGui::End( );
		}
		//style.WindowPadding = oldPadding;
		//style.Colors[ImGuiCol_WindowBg].w = oldAlpha;
	}

	void draw( IDirect3DDevice9* device )
	{
		ImGui::is_input_allowed = true;
		m_device = device;

		ImGuiIO& io = ImGui::GetIO( );
		io.MousePos.x = feature::menu->_cursor_position.x;
		io.MousePos.y = feature::menu->_cursor_position.y;
		//ImGui::GetIO().MouseDrawCursor = m_opened; // makes cursor dissapear when menu is closed ///кл€тий курсор, куди ти под≥вс€

		if ( ctx.m_settings.misc_status_list )
		{
			static bool tr = true;
			float cursor_pos_y;
			ImGui::Begin( "status list", &ctx.m_settings.misc_status_list, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_ShowBorders );
			{
				if ( csgo.m_engine( )->IsInGame( ) && !ctx.m_local( )->IsDead( ) )
				{
					ImGui::is_input_allowed = false;
					int desync_range = fabs( Math::angle_diff( ctx.angles[ ANGLE_REAL ], ctx.angles[ ANGLE_FAKE ] ) );
					int fakelag_choke = csgo.m_client_state( )->m_iChockedCommands;

					ImGui::Text( "desync delta:" );
					ImGui::SameLine( );

					cursor_pos_y = ImGui::GetCursorPosY( );

					ImGui::SetCursorPosY( cursor_pos_y + 8 );
					ImGui::SliderInt( "##lmao", &desync_range, 0, 60 );
					ImGui::SetCursorPosY( ImGui::GetCursorPosY( ) - 8 );


					ImGui::Text( "speed: %.0f", ctx.m_local( )->m_vecVelocity( ).Length2D( ) );

					ImGui::Text( "fakelag:" );
					ImGui::SameLine( );
					cursor_pos_y = ImGui::GetCursorPosY( );

					ImGui::SetCursorPosY( cursor_pos_y + 8 );
					ImGui::SliderInt( "##flmao", &fakelag_choke, 0, 15 );
					ImGui::SetCursorPosY( ImGui::GetCursorPosY( ) - 8 );

					if ( ctx.has_exploit_toggled )
					{
						ImGui::Text( "exploit:" );

						if ( ctx.exploit_allowed && ctx.ticks_allowed > 13 && ctx.main_exploit != 0 ) {
							ImGui::SameLine( );

							if ( ctx.main_exploit == 1 )
								ImGui::Text( "hide shots" );
							else if ( ctx.main_exploit > 1 )
								ImGui::Text( "doubletap" );
						}
						else
						{
							ImGui::SameLine( );
							ImGui::TextColored( ImVec4( 1.f, 0.f, 0.f, 1.f ), "none" );
						}
					}

					if ( ctx.m_settings.aimbot_key != 0 )
					{
						ImGui::Text( "rage aimbot: " );
						ImGui::SameLine( );

						if ( ctx.allows_aimbot )
							ImGui::Text( "enabled" );
						else
							ImGui::TextColored( ImVec4( 1.f, 0.f, 0.f, 1.f ), "disabled" );

					}

					if ( ctx.allows_aimbot )
					{
						ImGui::Text( "min damage override: " );
						ImGui::SameLine( );

						if ( feature::ragebot->m_damage_key )
							ImGui::Text( "enabled" );
						else
							ImGui::Text( "disabled" );

					}

				}
			}
			ImGui::End( );
		}

		speclist( );

		if ( !feature::menu->_menu_opened )
			return;

		ImGui::is_input_allowed = true;

		static auto alpha = 1.f;
		static auto pos = 0.4f;
		static auto switch_p = false;
		static auto switch_category = false;
		static auto was_hovered = false;
		static auto tab_size = 100.f;


		static auto t_ragebot = s_tab( ( "Ragebot" ), "c" );
		static auto t_legitbot = s_tab( ( "Legitbot" ), "a" );
		static auto t_visuals = s_tab( ( "Visuals" ), "d" );
		//static auto t_skins = s_tab(("Skins"), "e");
		static auto t_misc = s_tab( ( "Misc" ), "f" );
		static auto	curtab = 0;

		//bool pressed_return = false;

		if ( alpha < 1.f )
			init( 1.f ); //н≥чого кр≥м меню не повинно зм≥нювати прозор≥сть

		ImGui::SetNextWindowSize( ImVec2( 600, 400 ) );

		if ( ImGui::Begin( sxor( "birdie v4" ), ( bool* )0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar/*| ImGuiWindowFlags_CoolStyle*/ | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse, 0 ) )
		{
			auto window = ImGui::GetCurrentWindow( );
			const auto cursorpos = ImGui::GetCursorPos( );
			ImVec2 p = ImGui::GetCursorScreenPos( ); //ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left
			ImGui::GetWindowDrawList( )->AddRectFilledMultiColor( ImVec2( p.x + 2, p.y - 4 ), ImVec2( p.x + ( 600 - 5 ), p.y - 2 ), ImColor( ctx.m_settings.f_menu_color[ 0 ], ctx.m_settings.f_menu_color[ 1 ], ctx.m_settings.f_menu_color[ 2 ] ), ImColor( 0, 0, 0, 0 ), ImColor( ctx.m_settings.f_menu_color[ 0 ], ctx.m_settings.f_menu_color[ 1 ], ctx.m_settings.f_menu_color[ 2 ] ), ImColor( 0, 0, 0, 0 ) );

			if ( alpha < 1.f ) //ЅЋя“№ ну вот честно не ебу зачем € поставил эту хуйню два раза LINE#308, похуй.
				init( alpha ); //н≥чого кр≥м меню не повинно зм≥нювати прозор≥сть

			auto is_hovered = ImGui::mouse_in_pos( ImRect( window->Pos, window->Pos + ImVec2( static_cast< float >( tab_size * 0.45f ) + window->WindowPadding.x * 2.f, window->Size.y ) ) );

			if ( is_hovered || was_hovered && ImGui::mouse_in_pos( ImRect( window->Pos, window->Pos + ImVec2( tab_size + window->WindowPadding.x * 2.f, window->Size.y ) ) ) ) {
				if ( pos < 1.f )
					pos += 0.08f;

				is_hovered = true;
			}
			else {
				if ( pos > 0.5f )
					pos -= 0.08f;
			}

			pos = Math::clamp( pos, 0.5f, 1.f );

			const auto show_names = ( pos > 0.65f );
			ImGui::SetCursorPos( cursorpos );

			ImGui::PushClipRect( window->Pos, window->Pos + ImVec2( tab_size * pos + window->WindowPadding.x * 2.f, window->Size.y ), false );
			add_tab( t_ragebot, &curtab, ImVec2( tab_size * pos, static_cast< float >( tab_size * 0.45f ) ), show_names );
			//add_tab(t_legitbot, &curtab, ImVec2(tab_size * pos, static_cast<float>(tab_size * 0.45f)), show_names); 
			add_tab( t_visuals, &curtab, ImVec2( tab_size * pos, static_cast< float >( tab_size * 0.45f ) ), show_names );
			//add_tab(t_skins, &curtab, ImVec2(tab_size * pos, static_cast<float>(tab_size * 0.45f)), show_names);
			add_tab( t_misc, &curtab, ImVec2( tab_size * pos, static_cast< float >( tab_size * 0.45f ) ), show_names );
			ImGui::PopClipRect( );
			ImGui::SetCursorPos( ImVec2( tab_size * pos + window->WindowPadding.x * 2.f, cursorpos.y ) );
			alpha = Math::clamp( alpha, 0.f, 1.f );
			static int currentCategory{ 0 };
			static int currentWeapon{ 0 };

			ImGui::BeginChild( sxor( "##mainshit" ), ImVec2( 0, window->Size.y - window->WindowPadding.y * 2.f ), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse );
			{
				ImGui::GetStyle( ).Colors[ ImGuiCol_CheckMark ] = ImVec4( ctx.m_settings.f_menu_color[ 0 ], ctx.m_settings.f_menu_color[ 1 ], ctx.m_settings.f_menu_color[ 2 ], 1.f );

				float cursor_pos_y;
				switch ( curtab ) {
					case 0: {
						ImGui::BeginChild( "###rage1", ImVec2( 250, 365 / 2 ), true ); {
							ImGui::Checkbox( sxor( "Ragebot enabled" ), &ctx.m_settings.aimbot_enabled );
							if ( ctx.m_settings.aimbot_enabled ) {
								ImGui::SliderInt( sxor( "Field of view" ), &ctx.m_settings.aimbot_fov_limit, 0, 180 );
								ImGui::Spacing( );
							}
							ImGui::Checkbox( sxor( "Automatic revolver" ), &ctx.m_settings.aimbot_auto_revolver );
							ImGui::Checkbox( sxor( "Silent aim" ), &ctx.m_settings.aimbot_silent_aim );
							ImGui::Checkbox( sxor( "No Recoil/Spread" ), &ctx.m_settings.aimbot_no_spread );
							ImGui::Checkbox( sxor( "Automatic scope" ), &ctx.m_settings.aimbot_autoscope );
							ImGui::Checkbox( sxor( "Hitchance" ), &ctx.m_settings.aimbot_hitchance );
							if ( ctx.m_settings.aimbot_hitchance ) {
								ImGui::SliderInt( sxor( "Hitchance" ), &ctx.m_settings.aimbot_hitchance_val, 0, 100 );
								ImGui::Spacing( );
								ImGui::SliderInt( sxor( "Min damage" ), &ctx.m_settings.aimbot_min_damage_viable, 0, 101 );
								ImGui::Spacing( );
							}
							ImGui::Checkbox( sxor( "Prefer body aim" ), &ctx.m_settings.aimbot_prefer_body );
							ImGui::Checkbox( sxor( "Autowall" ), &ctx.m_settings.aimbot_autowall );
							if ( ctx.m_settings.aimbot_autowall ) {
								ImGui::SliderInt( sxor( "Autowall Mindmg" ), &ctx.m_settings.aimbot_min_damage, 0, 101 );
								ImGui::Spacing( );
							}
							ImGui::Checkbox( sxor( "Quick Stop" ), &ctx.m_settings.aimbot_autostop );
							ImGui::Checkbox( sxor( "Between Shoots" ), &ctx.m_settings.autostop_only_when_shooting );
							ImGui::Checkbox( sxor( "Force Accuracy" ), &ctx.m_settings.autostop_force_accuracy );
							ImGui::Combo( "##autostop", &ctx.m_settings.autostop_type, "Slow motion\0Full Stop\0" );
							ImGui::Text( "Doubletap ( toggle )" );
							ImGui::SameLine( );
							ImGui::Checkbox( "Tickbase Exploit", &ctx.m_settings.tickbase_exploit );
							ImGui::Bind( "##DTkey", &ctx.m_settings.aimbot_doubletap_exploit_toggle.key, ImVec2( 80, 20 ) );
							ImGui::SliderInt( "DT Hitchance", &ctx.m_settings.aimbot_doubletap_hitchance_val, 0, 50 );
							ImGui::Spacing( );
							ImGui::Text( "Hide Shots ( semi-bugged )" );
							ImGui::SameLine( );
							ImGui::Bind( "##Hideshotskey", &ctx.m_settings.aimbot_hideshots_exploit_toggle.key, ImVec2( 80, 20 ) );
						} ImGui::EndChild( );
						ImGui::SameLine( );
						ImGui::BeginChild( "###rage2", ImVec2( 250, 365 / 2 ), true ); {
							ImGui::Checkbox( sxor( "Anti-Aim" ), &ctx.m_settings.anti_aim_enabled );
							//ImGui::Checkbox( sxor( "Position Adjustment" ), &ctx.m_settings.aimbot_position_adjustment_old );
							ImGui::Combo( "Pitch", &ctx.m_settings.anti_aim_typex, "Off\0Emotion\0" );
							ImGui::Combo( "Yaw", &ctx.m_settings.anti_aim_typey, "Off\0Back\0View\0Spin\0" );
							ImGui::Combo( "Fake Type", &ctx.m_settings.anti_aim_typeyfake, "Off\0Static\0Static Angle\0Electro\0" );
							if ( ctx.m_settings.anti_aim_typeyfake != 0 )
								ImGui::SliderInt( "", &ctx.m_settings.anti_aim_fake_limit, -60, 60 );
							ImGui::Text( "Invert Desync side" );
							ImGui::SameLine( );
							ImGui::Bind( "###switch", &ctx.m_settings.anti_aim_fake_switch.key, ImVec2( 80, 20 ) );
							ImGui::Checkbox( sxor( "At Targets" ), &ctx.m_settings.anti_aim_at_target[ 0 ] );
							if ( ctx.m_settings.anti_aim_at_target[ 0 ] ) {
								ImGui::Checkbox( sxor( "At Targets ( low distance )" ), &ctx.m_settings.anti_aim_at_target[ 1 ] );
							}
							ImGui::Checkbox( sxor( "Anti-Aim Fake Jittering" ), &ctx.m_settings.anti_aim_fake_jittering );
							if ( ctx.m_settings.anti_aim_fake_jittering ) {
								ImGui::SliderInt( "Fake-Jitter Speed", &ctx.m_settings.antiaim_file_jit_speed, 0, 100 );
								ImGui::SliderInt( "Fake-Jitter Min", &ctx.m_settings.aa_fake_jit_min, 0, 60 );
								ImGui::SliderInt( "Fake-Jitter Max", &ctx.m_settings.aa_fake_jit_max, 0, 60 );
							}
							ImGui::Checkbox( sxor( "Slidewalk" ), &ctx.m_settings.slidewalk );
						} ImGui::EndChild( );
						ImGui::SetCursorPos( ImVec2( 8, 365 / 2 + 15 ) );
						ImGui::BeginChild( "###rage3", ImVec2( 250, 365 / 2 ), true ); {
							/* hitboxes */
							ImGui::Checkbox( sxor( "Head" ), &ctx.m_settings.aimbot_hitboxes[ 0 ] );
							ImGui::Checkbox( sxor( "Chest" ), &ctx.m_settings.aimbot_hitboxes[ 1 ] );
							ImGui::Checkbox( sxor( "Body" ), &ctx.m_settings.aimbot_hitboxes[ 2 ] );
							ImGui::Checkbox( sxor( "Arms" ), &ctx.m_settings.aimbot_hitboxes[ 3 ] );
							ImGui::Checkbox( sxor( "Legs" ), &ctx.m_settings.aimbot_hitboxes[ 4 ] );
							ImGui::Checkbox( sxor( "Feet" ), &ctx.m_settings.aimbot_hitboxes[ 5 ] );

							ImGui::Checkbox( sxor( "Head Scale" ), &ctx.m_settings.aimbot_multipoint[ 0 ] );
							if ( ctx.m_settings.aimbot_multipoint[ 0 ] )
							{
								ImGui::SliderInt( sxor( "##Headscale" ), &ctx.m_settings.aimbot_pointscale[ 0 ], 1, 100 );
								ImGui::Spacing( );
							}
							ImGui::Checkbox( sxor( "Chest Scale" ), &ctx.m_settings.aimbot_multipoint[ 1 ] );
							if ( ctx.m_settings.aimbot_multipoint[ 1 ] )
							{
								ImGui::SliderInt( sxor( "##chestscale" ), &ctx.m_settings.aimbot_pointscale[ 1 ], 1, 100 );
								ImGui::Spacing( );
							}
							ImGui::Checkbox( sxor( "Body Scale" ), &ctx.m_settings.aimbot_multipoint[ 2 ] );
							if ( ctx.m_settings.aimbot_multipoint[ 2 ] )
							{
								ImGui::SliderInt( sxor( "##bodyscale" ), &ctx.m_settings.aimbot_pointscale[ 2 ], 1, 100 );
								ImGui::Spacing( );
							}
							ImGui::Checkbox( sxor( "Arms Scale" ), &ctx.m_settings.aimbot_multipoint[ 3 ] );
							if ( ctx.m_settings.aimbot_multipoint[ 3 ] )
							{
								ImGui::SliderInt( sxor( "##armsscale" ), &ctx.m_settings.aimbot_pointscale[ 3 ], 1, 100 );
								ImGui::Spacing( );
							}
							ImGui::Checkbox( sxor( "Legs Scale" ), &ctx.m_settings.aimbot_multipoint[ 4 ] );
							if ( ctx.m_settings.aimbot_multipoint[ 4 ] )
							{
								ImGui::SliderInt( sxor( "##legsscale" ), &ctx.m_settings.aimbot_pointscale[ 4 ], 1, 100 );
								ImGui::Spacing( );
							}
							ImGui::Checkbox( sxor( "Feet Scale" ), &ctx.m_settings.aimbot_multipoint[ 5 ] );
							if ( ctx.m_settings.aimbot_multipoint[ 5 ] )
							{
								ImGui::SliderInt( sxor( "##feetscale" ), &ctx.m_settings.aimbot_pointscale[ 5 ], 1, 100 );
								ImGui::Spacing( );
							}
						} ImGui::EndChild( );
						ImGui::SameLine( );
						ImGui::BeginChild( "###rage4", ImVec2( 250, 365 / 2 ), true ); {
							ImGui::Checkbox( sxor( "Fakelag" ), &ctx.m_settings.fake_lag_enabled );
							ImGui::Combo( "##typeFakelag", &ctx.m_settings.fake_lag_type, "Maximum\0Break\0" );
							ImGui::SliderInt( "##fakelagval", &ctx.m_settings.fake_lag_value, 0, 15 );
							ImGui::Spacing( );
							ImGui::Checkbox( sxor( "Choke while shooting" ), &ctx.m_settings.fake_lag_shooting );
							ImGui::Checkbox( sxor( "Prediction when lischeck" ), &ctx.m_settings.fake_lag_special );
						} ImGui::EndChild( );
					} break;
					case 2: {
						ImGui::BeginChild( "###vis1", ImVec2( 250, 365 / 2 ), true ); {
							ImGui::Checkbox( "Visuals enabled", &ctx.m_settings.visuals_enabled );
							ImGui::Checkbox( "Dormant", &ctx.m_settings.esp_dormant );
							ImGui::Checkbox( "Offscreen arrows", &ctx.m_settings.esp_offscreen );
							ImGui::ColorPickerBox( "#nigarrows", ctx.m_settings.colors_esp_offscreen, nullptr, nullptr, nullptr );
							ImGui::Checkbox( "Name", &ctx.m_settings.esp_name );
							ImGui::ColorPickerBox( "#nigname", ctx.m_settings.colors_esp_name, nullptr, nullptr, nullptr );
							ImGui::Checkbox( "Health", &ctx.m_settings.esp_health );
							ImGui::Checkbox( "Weapon", &ctx.m_settings.esp_weapon );
							ImGui::Checkbox( "Weapon ammo", &ctx.m_settings.esp_weapon_ammo );
							ImGui::Checkbox( "Skeleton", &ctx.m_settings.esp_skeleton );
							ImGui::ColorPickerBox( "#nigton", ctx.m_settings.colors_skeletons_enemy, nullptr, nullptr, nullptr );

						} ImGui::EndChild( );
						ImGui::SameLine( );
						ImGui::BeginChild( "###vis2", ImVec2( 250, 365 / 2 ), true ); {
							ImGui::Checkbox( "Glow ESP", &ctx.m_settings.esp_glow );
							ImGui::ColorPickerBox( "#nigglow", ctx.m_settings.colors_glow_enemy, nullptr, nullptr, nullptr );
							ImGui::Checkbox( "Visible Chams", &ctx.m_settings.chams_enemy );
							ImGui::ColorPickerBox( "#nigchamviable", ctx.m_settings.colors_chams_enemy_viable, nullptr, nullptr, nullptr );
							ImGui::Checkbox( "Behind Wall", &ctx.m_settings.chams_walls );
							ImGui::ColorPickerBox( "#nigchamshidden", ctx.m_settings.colors_chams_enemy_hidden, nullptr, nullptr, nullptr );
							ImGui::Text( sxor( "Override player material" ) );
							ImGui::Combo( "##playamaterial", &ctx.m_settings.menu_chams_type, "Flat\0Filled\0Glowing\0Model Glowing\0" );
						} ImGui::EndChild( );
						ImGui::SetCursorPos( ImVec2( 8, 365 / 2 + 15 ) );
						ImGui::BeginChild( "###vis3", ImVec2( 250, 365 / 2 ), true ); {
							ImGui::Checkbox( "Flags ( armor )", &ctx.m_settings.esp_flags[ 0 ] );
							ImGui::Checkbox( "Flags ( scoped )", &ctx.m_settings.esp_flags[ 2 ] );

							ImGui::Checkbox( "Watermark", &ctx.m_settings.watermark );
							ImGui::Checkbox( "Spectators", &ctx.m_settings.visuals_extra_windows[ 0 ] );
							ImGui::Checkbox( "Keybinds", &ctx.m_settings.visuals_extra_windows[ 1 ] );
						} ImGui::EndChild( );
						ImGui::SameLine( );
						ImGui::BeginChild( "###vis4", ImVec2( 250, 365 / 2 ), true ); {
							ImGui::Checkbox( "Nightmode", &ctx.m_settings.misc_visuals_world_modulation[ 1 ] );
							ImGui::ColorPickerBox( "#nigmode", ctx.m_settings.colors_world_color, nullptr, nullptr, nullptr );
							ImGui::Checkbox( "Remove visual recoil", &ctx.m_settings.visuals_no_recoil );
							ImGui::Checkbox( "Remove ingame post processing", &ctx.m_settings.visuals_no_postprocess );
							ImGui::Checkbox( "Remove scope", &ctx.m_settings.visuals_no_scope );
							if ( ctx.m_settings.visuals_no_scope ) {
								ImGui::Combo( "##playamaterial", &ctx.m_settings.visuals_no_scope_type, "Normal\0Gradient\0" );
								if ( ctx.m_settings.visuals_no_scope_type == 1 ) {
									ImGui::ColorPickerBox( "#colorpicker", ctx.m_settings.no_scope_color, nullptr, nullptr, nullptr );
								}
							}
							ImGui::Checkbox( "Remove 1st zoom level", &ctx.m_settings.visuals_no_first_scope );
							ImGui::Checkbox( "Remove smoke", &ctx.m_settings.visuals_no_smoke );
							ImGui::Checkbox( "Remove flash", &ctx.m_settings.visuals_no_flash );
							ImGui::Checkbox( "Override fov", &ctx.m_settings.misc_override_fov );
							if ( ctx.m_settings.misc_override_fov ) {
								ImGui::SliderInt( "Overide Fov", &ctx.m_settings.misc_override_fov_val, -20, 60 );
							}
							ImGui::Checkbox( "Penetration crosshair", &ctx.m_settings.visuals_autowall_crosshair );
							ImGui::Checkbox( "Bullet impacts", &ctx.m_settings.visuals_draw_local_impacts );
							ImGui::Checkbox( "Grenade Prediction", &ctx.m_settings.misc_grenade_preview );
							ImGui::Checkbox( "Nade Info", &ctx.m_settings.esp_world_nades );
							ImGui::Checkbox( "Dropped Weapons", &ctx.m_settings.esp_world_weapons );
							ImGui::Checkbox( "Hitmarker", &ctx.m_settings.misc_visuals_indicators_2[ 0 ] );
							ImGui::Checkbox( "Hitsound", &ctx.m_settings.misc_hitsound_type );
							ImGui::Checkbox( "Bomb Esp", &ctx.m_settings.esp_world_bomb );
							ImGui::Checkbox( "Local bullets tracer", &ctx.m_settings.visuals_draw_local_beams );
							ImGui::ColorPickerBox( "#nigtrace", ctx.m_settings.local_beams_color, nullptr, nullptr, nullptr );
							ImGui::Combo( "##typexAA", &ctx.m_settings.bullet_tracer_materials, "purplelaser1\0blueglow1\0bubble\0glow01\0physbeam\0purpleglow1\0radio\0white" );
							ImGui::Spacing( );
						} ImGui::EndChild( );
					} break;
					case 3: {
						ImGui::BeginChild( "##misc1", ImVec2( 250, 365 / 2 ), true ); {
							ImGui::Checkbox( sxor( "Auto bhop" ), &ctx.m_settings.misc_bhop );
							ImGui::Checkbox( sxor( "Auto strafe" ), &ctx.m_settings.misc_autostrafer );
							ImGui::Checkbox( sxor( "WASD" ), &ctx.m_settings.misc_autostrafer_wasd );
							ImGui::Checkbox( sxor( "In-game Inventory" ), &ctx.m_settings.misc_unlock_inventory );
							ImGui::Checkbox( sxor( "Left Hand Knife" ), &ctx.m_settings.misc_knife_hand_switch );
							ImGui::Checkbox( sxor( "Preserve killfeed" ), &ctx.m_settings.misc_preserve_killfeed );
							ImGui::SliderInt( sxor( "Aspect Ratio" ), &ctx.m_settings.misc_aspect_ratio, 0, 150 );
							ImGui::Spacing( );
							ImGui::Text( sxor( "Auto peek" ) );
							ImGui::SameLine( );
							ImGui::Bind( "##apkey", &ctx.m_settings.anti_aim_autopeek_key.key, ImVec2( 80, 20 ) );
							ImGui::Spacing( );
							ImGui::Text( sxor( "Fakeduck" ) );
							ImGui::SameLine( );
							ImGui::Bind( "##fdkey", &ctx.m_settings.anti_aim_fakeduck_key.key, ImVec2( 80, 20 ) );
							ImGui::Checkbox( sxor( "Thirdperson" ), &ctx.m_settings.visuals_tp_force );
							ImGui::SameLine( );
							ImGui::Bind( "##tpkey", &ctx.m_settings.visuals_tp_key.key, ImVec2( 80, 20 ) );
							if ( ctx.m_settings.visuals_tp_force )
								ImGui::Spacing( );
							ImGui::SliderInt( sxor( "##tpdistance" ), &ctx.m_settings.visuals_tp_dist, 30, 300 );


						} ImGui::EndChild( );
						ImGui::SameLine( );
						ImGui::BeginChild( "###misc2", ImVec2( 250, 365 / 2 ), true ); {
							ImGui::Text( sxor( "Menu Accent" ) );
							ImGui::SameLine( );
							ImGui::ColorPickerBox( "#menuacc", ctx.m_settings.f_menu_color, nullptr, nullptr, nullptr );
							ImGui::Text( sxor( "CFGS" ) );
							ImGui::Combo( "##cfgselct", &g_settings.cur_cfg, "Legit\0Rage\0Hvh\0MM hvh\0Semi-rage\0" );

							if ( ImGui::Button( sxor( "Save" ) ) )
								feature::misc->save_cfg( );

							if ( ImGui::Button( sxor( "Load" ) ) ) {
								feature::misc->load_cfg( );
								if ( ctx.m_settings.skinchanger_enabled ) ctx.updated_skin = true;
							}
						} ImGui::EndChild( );
						ImGui::SetCursorPos( ImVec2( 8, 365 / 2 + 15 ) );
						ImGui::BeginChild( "###misc3", ImVec2( 250, 365 / 2 ), true ); {
							ImGui::Checkbox( sxor( "Log misses" ), &ctx.m_settings.misc_notifications[ 2 ] );
							ImGui::Checkbox( sxor( "Log damage" ), &ctx.m_settings.misc_notifications[ 1 ] );
							ImGui::Checkbox( sxor( "Log purchases" ), &ctx.m_settings.misc_notifications[ 4 ] );
						} ImGui::EndChild( );
						ImGui::SameLine( );
						ImGui::BeginChild( "###misc4", ImVec2( 250, 365 / 2 ), true ); {
							ImGui::Checkbox( sxor( "Autobuy" ), &ctx.m_settings.misc_autobuy_enabled );
							if ( ctx.m_settings.misc_autobuy_enabled ) {
								ImGui::Text( sxor( "Autobuy primary" ) );
								ImGui::Combo( sxor( "##Autoprimary" ), &ctx.m_settings.misc_autobuy_primary, "none\0AK47/M4A1\0AWP\0SCAR20/G3\0SSG-08\0\0" );
								ImGui::Text( sxor( "Autobuy secondary" ) );
								ImGui::Combo( sxor( "##Autoecondary" ), &ctx.m_settings.misc_autobuy_secondary, "none\0Deagle/R8\0Tec9/FiveSeven\0Dual berettas\0P250/CZ-74\0" );
								ImGui::Separator( );
								ImGui::Text( sxor( "Minimum Money" ) );
								ImGui::SliderInt( sxor( "##musor" ), &ctx.m_settings.misc_autobuy_money_limit, 0, 10000 );
								ImGui::Spacing( );
								ImGui::Separator( );
								ImGui::Checkbox( sxor( "HE" ), &ctx.m_settings.misc_autobuy_etc[ 4 ] );
								ImGui::SameLine( );
								ImGui::Checkbox( sxor( "Smoke" ), &ctx.m_settings.misc_autobuy_etc[ 3 ] );
								ImGui::SameLine( );
								ImGui::Checkbox( sxor( "Flash" ), &ctx.m_settings.misc_autobuy_etc[ 5 ] );
								ImGui::SameLine( );
								ImGui::Checkbox( sxor( "Molotov" ), &ctx.m_settings.misc_autobuy_etc[ 7 ] );
								ImGui::Separator( );
								ImGui::Checkbox( sxor( "Armor" ), &ctx.m_settings.misc_autobuy_etc[ 1 ] );
								ImGui::SameLine( );
								ImGui::Checkbox( sxor( "Taser" ), &ctx.m_settings.misc_autobuy_etc[ 0 ] );
								ImGui::SameLine( );
								ImGui::Checkbox( sxor( "Kit" ), &ctx.m_settings.misc_autobuy_etc[ 2 ] );
							}
			
						} ImGui::EndChild( );
					} break;
				}
			} ImGui::EndChild( );

			was_hovered = is_hovered;
		}

		if ( alpha != 1.f )
			init( 1.f );

		ImGui::End( );
	}

}