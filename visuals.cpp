#include "visuals.hpp"
#include "source.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "weapon.hpp"
#include "hooked.hpp"
#include "displacement.hpp"
#include "anti_aimbot.hpp"
#include "resolver.hpp"
#include "prop_manager.hpp"
#include <algorithm>
#include "movement.hpp"
#include "menu.hpp"
#include <time.h>
#include "prediction.hpp"
#include "rage_aimbot.hpp"
#include "lag_comp.hpp"
#include "chams.hpp"
#include "sound_parser.hpp"
#include "autowall.hpp"
#include "music_player.hpp"
#include "grenades.hpp"

#include <thread>
#include <cctype>
#include <map>

std::vector<_event> _events;
std::vector<c_bullet_tracer> bullet_tracers;
std::vector<c_damage_indicator> damage_indicators;
//std::vector<std::string> m_esp_info;

constexpr auto _hk = LIT(("HK"));
constexpr auto _k = LIT(("K"));
constexpr auto _h = LIT(("H"));
constexpr auto _r8 = LIT(("revolver"));
constexpr auto _z = LIT(("ZOOM"));
constexpr auto _pin = LIT(("PIN"));
constexpr auto _c4 = LIT(("C4"));
constexpr auto _vip = LIT(("VIP"));
constexpr auto _hs = LIT(("HEADSHOT"));

constexpr auto _m = LIT(("read tutorial on forums"));

bool c_visuals::get_espbox(C_BasePlayer* entity, int& x, int& y, int& w, int& h)
{
	if (!entity || !entity->GetClientClass())
		return false;

	if (entity->IsPlayer() && entity->GetCollideable())
	{

		auto log = &feature::lagcomp->records[entity->entindex() - 1];

			auto min = entity->GetCollideable()->OBBMins();
			auto max = entity->GetCollideable()->OBBMaxs();

			Vector dir, vF, vR, vU;

			csgo.m_engine()->GetViewAngles(dir);
			dir.x = 0;
			dir.z = 0;

			Math::AngleVectors(dir, &vF, &vR, &vU);

			auto zh = vU * max.z + vF * max.y + vR * min.x; // = Front left front
			auto e = vU * max.z + vF * max.y + vR * max.x; //  = Front right front
			auto d = vU * max.z + vF * min.y + vR * min.x; //  = Front left back
			auto c = vU * max.z + vF * min.y + vR * max.x; //  = Front right back

			auto g = vU * min.z + vF * max.y + vR * min.x; //  = Bottom left front
			auto f = vU * min.z + vF * max.y + vR * max.x; //  = Bottom right front
			auto a = vU * min.z + vF * min.y + vR * min.x; //  = Bottom left back
			auto b = vU * min.z + vF * min.y + vR * max.x; //  = Bottom right back*-

			Vector pointList[] = {
				a,
				b,
				c,
				d,
				e,
				f,
				g,
				zh,
			};

			Vector transformed[ARRAYSIZE(pointList)];

			for (int i = 0; i < ARRAYSIZE(pointList); i++)
			{	
				auto origin = !entity->IsDormant() ? entity->get_abs_origin() : entity->m_vecOrigin();
				
				if (log && log->player == entity && log->records_count > 0 && !entity->IsDormant() && ctx.m_local() && !ctx.m_local()->IsDead()) {
					auto last = &log->tick_records[log->records_count & 63];
					origin = entity->get_bone_pos(8) + (last->origin - last->head_pos);
				}



				pointList[i] += origin;
				if (!Drawing::WorldToScreen(pointList[i], transformed[i]))
					return false;
			}

			float left = FLT_MAX;
			float top = -FLT_MAX;
			float right = -FLT_MAX;
			float bottom = FLT_MAX;

			for (int i = 0; i < ARRAYSIZE(pointList); i++) {
				if (left > transformed[i].x)
					left = transformed[i].x;
				if (top < transformed[i].y)
					top = transformed[i].y;
				if (right < transformed[i].x)
					right = transformed[i].x;
				if (bottom > transformed[i].y)
					bottom = transformed[i].y;
			}

			x = left;
			y = bottom;
			w = right - left;
			h = top - bottom;
		//}

		return true;
	}
	else
	{
		Vector /*vOrigin, */min, max, flb, brt, blb, frt, frb, brb, blt, flt;
		//float left, top, right, bottom;

		auto* collideable = entity->GetCollideable();

		if (!collideable)
			return false;

		min = collideable->OBBMins();
		max = collideable->OBBMaxs();

		matrix3x4_t& trans = entity->GetCollisionBoundTrans();

		Vector points[] =
		{
			Vector(min.x, min.y, min.z),
			Vector(min.x, max.y, min.z),
			Vector(max.x, max.y, min.z),
			Vector(max.x, min.y, min.z),
			Vector(max.x, max.y, max.z),
			Vector(min.x, max.y, max.z),
			Vector(min.x, min.y, max.z),
			Vector(max.x, min.y, max.z)
		};

		Vector pointsTransformed[8];
		for (int i = 0; i < 8; i++) {
			Math::VectorTransform(points[i], trans, pointsTransformed[i]);
		}

		Vector pos = entity->get_abs_origin();

		if (!Drawing::WorldToScreen(pointsTransformed[3], flb) || !Drawing::WorldToScreen(pointsTransformed[5], brt)
			|| !Drawing::WorldToScreen(pointsTransformed[0], blb) || !Drawing::WorldToScreen(pointsTransformed[4], frt)
			|| !Drawing::WorldToScreen(pointsTransformed[2], frb) || !Drawing::WorldToScreen(pointsTransformed[1], brb)
			|| !Drawing::WorldToScreen(pointsTransformed[6], blt) || !Drawing::WorldToScreen(pointsTransformed[7], flt))
			return false;

		Vector arr[] = { flb, brt, blb, frt, frb, brb, blt, flt };
		//+1 for each cuz of borders at the original box
		float left = flb.x;        // left
		float top = flb.y;        // top
		float right = flb.x;    // right
		float bottom = flb.y;    // bottom

		for (int i = 1; i < 8; i++)
		{
			if (left > arr[i].x)
				left = arr[i].x;
			if (bottom < arr[i].y)
				bottom = arr[i].y;
			if (right < arr[i].x)
				right = arr[i].x;
			if (top > arr[i].y)
				top = arr[i].y;
		}

		x = (int)left;
		y = (int)top;
		w = (int)(right - left);
		h = (int)(bottom - top);

		return true;
	}
	return false;
}

constexpr int dlinesize = 3, dlinedec = 5;
constexpr int linesize = 5, linedec = 11;

void c_visuals::damage_esp()
{
	if (damage_indicators.empty() || damage_indicators.size() > 20) return;

	int last_tick;
	Vector last_pos;

	for (auto it = damage_indicators.begin(); it != damage_indicators.end();)
	{
		auto record = &(*it);

		if (it == damage_indicators.end() || !record)
			break;

		if (csgo.m_globals()->realtime >= record->_time) {
			it = damage_indicators.erase(it);
			continue;
		}

		if (damage_indicators.size() > 10 && record->_time != damage_indicators.front()._time)
		{
			int skip = float(0.5f * (int)damage_indicators.size());

			auto prev = *(damage_indicators.begin() + skip);

			if (record->_time < prev._time) {
				++it;
				continue;
			}
		}

		if (!Drawing::WorldToScreen(record->_spot, record->w2s))
		{ 
			++it;
			continue;
		}

		const auto was_same = record->_tick == csgo.m_globals()->tickcount;

		if (!record->_headshot)
			last_tick = csgo.m_globals()->tickcount;

		auto alpha = min(245, 90 * (record->_time - csgo.m_globals()->realtime));

		Drawing::DrawLine( record->w2s.x - dlinesize, record->w2s.y - dlinesize, record->w2s.x - dlinedec, record->w2s.y - dlinedec, Color( 200, 200, 200, alpha ) );
		Drawing::DrawLine( record->w2s.x - dlinesize, record->w2s.y + dlinesize, record->w2s.x - dlinedec, record->w2s.y + dlinedec, Color( 200, 200, 200, alpha ) );
		Drawing::DrawLine( record->w2s.x + dlinesize, record->w2s.y + dlinesize, record->w2s.x + dlinedec, record->w2s.y + dlinedec, Color( 200, 200, 200, alpha ) );
		Drawing::DrawLine( record->w2s.x + dlinesize, record->w2s.y - dlinesize, record->w2s.x + dlinedec, record->w2s.y - dlinedec, Color( 200, 200, 200, alpha ) );

		last_pos = record->w2s;
		

		++it;
	}
}

void c_visuals::logs()
{
	int bottom = 0;
	if (_events.empty())
		return;

	int x = 8;
	int y = 5;
	auto count = 0;
	const int fontTall = csgo.m_surface()->GetFontTall(F::Events) + 1;

	for (auto& event : _events)
	{
		if (_events.back()._time < csgo.m_globals()->realtime)
			_events.pop_back();

		if (event._time < csgo.m_globals()->realtime && event._displayticks > 0)
			continue;

		if (count > 10)
			break;

		if (!event._msg.empty())
		{
			if (!event._displayed)
			{
				csgo.m_engine_cvars()->ConsoleColorPrintf(Color::White(), sxor("["));
				csgo.m_engine_cvars()->ConsoleColorPrintf(Color::LightBlue(), sxor("deathhole"));
				csgo.m_engine_cvars()->ConsoleColorPrintf(Color::White(), sxor("] %s\n"), event._msg.c_str());

				event._displayed = true;
			}
			if (event._msg[0] == 'f' && event._msg[2] == 'r' && event._msg[6] == 's')
				continue;
			
			Color clr = Color::White();

			const float timeleft = fabs(event._time - csgo.m_globals()->realtime);

			if (timeleft < .5f)
			{
				float f = Math::clamp(timeleft, 0.0f, .5f) / .5f;

				clr[3] = (int)(f * 255);

				if (count == 0 && f < 0.2f)
				{
					y -= (1.0f - f / 0.2f) * fontTall;
				}
			}
			else
			{
				clr[3] = 255;
			}

			Drawing::DrawString(F::Events, x, y, clr, FONT_LEFT, "%s", event._msg.c_str());

			y += fontTall;

			count++;
		}
	}
}

void c_visuals::skeleton(C_BasePlayer* Entity, Color color, matrix3x4_t* pBoneToWorldOut)
{
	auto model = Entity->GetModel();
	if (!model) return;

	auto studio_model = csgo.m_model_info()->GetStudioModel(Entity->GetModel());
	if (!studio_model) return;

	for (int i = 0; i < studio_model->numbones; i++)
	{
		auto pBone = studio_model->pBone(i);

		if (!pBone || !(pBone->flags & 256) || pBone->parent == -1)
			continue;

		if (Vector(pBoneToWorldOut[i][0][3], pBoneToWorldOut[i][1][3], pBoneToWorldOut[i][2][3]).IsZero() || Vector(pBoneToWorldOut[pBone->parent][0][3], pBoneToWorldOut[pBone->parent][1][3], pBoneToWorldOut[pBone->parent][2][3]).IsZero())
			continue;

		Vector vBonePos1;
		if (!Drawing::WorldToScreen(Vector(pBoneToWorldOut[i][0][3], pBoneToWorldOut[i][1][3], pBoneToWorldOut[i][2][3]), vBonePos1))
			continue;

		Vector vBonePos2;
		if (!Drawing::WorldToScreen(Vector(pBoneToWorldOut[pBone->parent][0][3], pBoneToWorldOut[pBone->parent][1][3], pBoneToWorldOut[pBone->parent][2][3]), vBonePos2))
			continue;

		Drawing::DrawLine((int)vBonePos1.x, (int)vBonePos1.y, (int)vBonePos2.x, (int)vBonePos2.y, color);
	}
}

void c_visuals::offscreen_esp(C_BasePlayer* entity, float alpha)
{

	Vector vEnemyOrigin = entity->get_abs_origin();
	Vector vLocalOrigin = ctx.m_local()->get_abs_origin();

	auto is_on_screen = [](Vector origin, Vector& screen) -> bool
	{
		if (!Drawing::WorldToScreen(origin, screen))
			return false;

		return (screen.x > 0 && screen.x < ctx.screen_size.x) && (ctx.screen_size.y > screen.y && screen.y > 0);
	};

	Vector screenPos;

	if (!entity->IsDormant())
	{
		if (entity->m_bone_count() <= 10 || is_on_screen(entity->get_bone_pos(6), screenPos)) //TODO (?): maybe a combo/checkbox to turn this on/off
			return;
	}
	else
	{
		if (is_on_screen(vEnemyOrigin, screenPos))
			return;
	}

	Vector dir;

	csgo.m_engine()->GetViewAngles(dir);

	float view_angle = dir.y;

	if (view_angle < 0)
		view_angle += 360;
	
	view_angle = DEG2RAD(view_angle);

	auto entity_angle = Math::CalcAngle(vLocalOrigin, vEnemyOrigin);
	entity_angle.Normalize();

	if (entity_angle.y < 0.f)
		entity_angle.y += 360.f;
	
	entity_angle.y = DEG2RAD(entity_angle.y);
	entity_angle.y -= view_angle;

	const float wm = ctx.screen_size.x / 2, hm = ctx.screen_size.y / 2;

	auto position = Vector2D(wm, hm);
	position.x -= ctx.m_settings.esp_arrows_distance * 5.25f;

	Drawing::rotate_point(position, Vector2D(wm, hm), false, entity_angle.y);

	const auto size = static_cast<int>(ctx.m_settings.esp_arrows_size);//std::clamp(100 - int(vEnemyOrigin.Distance(vLocalOrigin) / 6), 10, 25);

	static float old_fade_alpha = 1;
	static bool sw = false;

	if (old_fade_alpha <= 0)
		sw = true;
	else if (old_fade_alpha >= 1)
		sw = false;

	if (sw)
		old_fade_alpha = Math::clamp(old_fade_alpha + csgo.m_globals()->frametime * 1.2f, 0, 1);
	else
		old_fade_alpha = Math::clamp(old_fade_alpha - csgo.m_globals()->frametime * 1.2f, 0, 1);

	float fade_alpha = old_fade_alpha;

	if (alpha < fade_alpha)
		fade_alpha = alpha;

	Drawing::filled_tilted_triangle(position, Vector2D(size - 1, size), position, true, -entity_angle.y,
		ctx.flt2color(ctx.m_settings.colors_esp_offscreen).malpha(fade_alpha * 0.6), true,
		ctx.flt2color(ctx.m_settings.colors_esp_offscreen).malpha(alpha));
}

int indicators_count = 0;

class c_indicator
{
public:
	c_indicator()
	{
		text = "";
		max_value = 0.f;
	};

	c_indicator(const char* _text, const float _max_value)
	{
		text = _text;
		max_value = _max_value;
	};

	float get_max_value(const float meme = FLT_MAX)
	{
		if (meme < FLT_MAX)
			max_value = meme;
		
		return max_value;
	}

	void draw(const float factor, Color color = Color(123, 194, 21, 250), const float max_factor = -1.f)
	{
		const auto max = max_factor > 0.f ? max_factor : max_value;

		if (csgo.m_client()->IsChatRaised())
			color._a() = 10;

		const auto text_size = Drawing::GetTextSize(F::LBY, text);
		Drawing::DrawString(F::LBY, 10, ctx.screen_size.y - 88 - 26 * indicators_count, Color(30, 30, 30, csgo.m_client()->IsChatRaised() ? 10 : 250), FONT_LEFT, text);
		const auto draw_factor = Math::clamp((factor / max), 0, 1);
		*reinterpret_cast<bool*>(uintptr_t(csgo.m_surface()) + 0x280) = true;
		int x, y, x1, y1;
		Drawing::GetDrawingArea(x, y, x1, y1);
		Drawing::LimitDrawingArea(0, ctx.screen_size.y - 88 - 26 * indicators_count, int((text_size.right + 15) * draw_factor), (int)text_size.bottom);
		Drawing::DrawString(F::LBY, 10, ctx.screen_size.y - 88 - 26 * indicators_count++, color, FONT_LEFT, text);
		Drawing::LimitDrawingArea(x, y, x1, y1);
		*reinterpret_cast<bool*>(uintptr_t(csgo.m_surface()) + 0x280) = false;
	};
private:
	const char* text = "";
	float max_value = 0.f;
};

int g_Math(float num, float max, int arrsize)
{
	if (num > max)
		num = max;

	auto tmp = max / num;
	auto i = (arrsize / tmp);

	//i = (i >= 0 && floor(i + 0.5) || ceil(i - 0.5));

	if (i >= 0)
		i = floor(i + 0.5f);
	else
		i = ceil(i - 0.5);

	return i;
}

Color g_ColorByInt(float number, float max) {
	static Color Colors[] = {
		{ 255, 0, 0 },
		{ 237, 27, 3 },
		{ 235, 63, 6 },
		{ 229, 104, 8 },
		{ 228, 126, 10 },
		{ 220, 169, 16 },
		{ 213, 201, 19 },
		{ 176, 205, 10 },
		{ 124, 195, 13 }
	};

	auto i = g_Math(number, max, ARRAYSIZE(Colors)-1);
	return Colors[Math::clamp(i, 0, (int)ARRAYSIZE(Colors)-1)];
}

std::string _name[15] = { (std::string)sxor("fakeduck"), (std::string)sxor("doubletap"), (std::string)sxor("voice player"), (std::string)sxor("thirdperson"), (std::string)sxor("slow walk"), (std::string)sxor("force body aim"), (std::string)sxor("min damage override"), (std::string)sxor("side control"), (std::string)sxor("manual backward"), (std::string)sxor("manual right"), (std::string)sxor("manual left"), (std::string)sxor("auto peek"), (std::string)sxor("freestanding"), (std::string)sxor("suppress shot"), (std::string)sxor("time stop") };
std::string _type[5] = { (std::string)sxor("toggle"), (std::string)sxor("on key"), (std::string)sxor("off key"), (std::string)sxor("always on"), (std::string)sxor("pressed") };
float keybinds_time[15];
constexpr int size_info = 8;

void DECLSPEC_NOINLINE c_visuals::render(bool reset)
{
	logs();

	auto get_name_by_paint_kit = [](int wpn, int id) -> const char*
	{
		if (parser::weapons.list[wpn].skins.list.empty())
			return "";

		auto weapon = -1;

		for (auto i = 0; i < parser::weapons.list.size(); i++) {
			auto& cwpn = parser::weapons.list[i];
			if (cwpn.id == wpn) {
				weapon = i;
			}
		}

		if (weapon == -1)
			return "";

		for (auto i = 0; i < parser::weapons.list[weapon].skins.list.size(); i++) {
			auto& skin = parser::weapons.list[weapon].skins.list[i];

			if (skin.id == id) {

				if (skin.translated_name.size() <= 1)
					continue;

				auto strname = std::string(skin.translated_name.begin(), skin.translated_name.end());

				char name[128] = "";
				sprintf_s(name, " | %s", strname.c_str());

				return name;
			}
		}

		return "";
	};

	auto get_spectators = []() -> std::list<int>
	{
		std::list<int> list = {};

		if (!csgo.m_engine()->IsInGame() || !ctx.m_local() || ctx.m_local()->IsDead()) {
			return list;
		}

		for (int i = 1; i < 64; i++)
		{
			auto ent = csgo.m_entity_list()->GetClientEntity(i);

			if (ent == nullptr)
				continue;

			if (ent->IsDormant() || !ent->GetClientClass())
				continue;

			if (ent->m_hObserverTarget() == INVALID_HANDLE_VALUE)
				continue;

			auto target = csgo.m_entity_list()->GetClientEntityFromHandle(*ent->m_hObserverTarget());

			if (!target || ctx.m_local() != target)
				continue;

			list.push_back(i);
		}

		return list;
	};

	auto get_keybinds = [&](int& working_keybinds) -> std::list<c_keybindinfo>
	{

		std::list<c_keybindinfo> list = {};

		if (!csgo.m_engine()->IsInGame() || !ctx.m_local() || ctx.m_local()->IsDead())
			return list;



		for (auto i = 0; i < 15; i++)
		{
			const auto w = working_keybinds;

			ctx.active_keybinds[i].index = i;

			if (ctx.active_keybinds[i].mode > 0)
			{ //(csgo.m_globals()->realtime - keybinds_time[i]) >= csgo.m_globals()->interval_per_tick)
				keybinds_time[i] = csgo.m_globals()->realtime;
				++working_keybinds;
			}
			else if (((csgo.m_globals()->realtime - keybinds_time[i]) * 2.0f) < 0.9f)
				++working_keybinds;
			else
				ctx.active_keybinds[i].sort_index = 0;
			
			if (w != working_keybinds && ctx.active_keybinds[i].mode > 0 && !ctx.active_keybinds[i].prev_state)
				ctx.active_keybinds[i].sort_index = ctx.active_keybinds_visible++;
			else
			{
				if (ctx.active_keybinds[i].prev_state && ctx.active_keybinds[i].mode < 0)
					ctx.active_keybinds_visible--;
			}

			list.push_back(c_keybindinfo{ ctx.active_keybinds[i].index, _name[i].c_str(), _type[Math::clamp(ctx.active_keybinds[i].mode - 1, 0, 4)].c_str(), ctx.active_keybinds[i].sort_index });

			ctx.active_keybinds[i].prev_state = w != working_keybinds;
		}

		list.sort(
			[](const c_keybindinfo& a, const c_keybindinfo& b) {
				return a.sort_index < b.sort_index;
			});


		return list;
	};

	static auto prev_frame = csgo.m_globals()->framecount;

	static char buffer[255];
	static char timebuff[80];

	if (abs(prev_frame - csgo.m_globals()->framecount) > 1) {
		time_t now = time(0);   
		 tm  tstruct;
		tstruct = *localtime(&now);


		strftime(timebuff, sizeof(timebuff), "%X", &tstruct);

		prev_frame = csgo.m_globals()->framecount;
	}

	static float piska = 0.f;

	if (!ctx.auto_peek_spot.IsZero()) {
		const float step = (M_PI * 2.0f) / 24;
		float radius = 24;
		static bool lol = true;
		static auto start = 0.f;
		static auto max_a = ((M_PI * 2.0f) - step);
		static float prev_a = 0.f;
		Vector w2sCenter;
		if (Drawing::WorldToScreen(ctx.auto_peek_spot, w2sCenter)) {
			for (float a = 0; a < ((M_PI * 2.0f) - step); a += step) {
				float cos_end, sin_end;
				sin_end = sin(a + step);
				cos_end = cos(a + step);

				Vector end(radius * cos_end + ctx.auto_peek_spot.x, radius * sin_end + ctx.auto_peek_spot.y, ctx.auto_peek_spot.z);

				Vector end2d;

				if (!Drawing::WorldToScreen(end, end2d))
					break;

				if (a < prev_a)
					max_a = prev_a;

				if (start >= (max_a + (step * 0.5f)))
					lol = false;
				if (start <= -(step * 0.5f))
					lol = true;

				start += (lol ? (min(max(csgo.m_globals()->frametime * 0.1f, max_a - start), csgo.m_globals()->frametime * 0.1f)) : (max(-max(csgo.m_globals()->frametime * 0.1f, start), csgo.m_globals()->frametime * -0.1f)));

				start = Math::clamp(start, -(step * 0.5f), (max_a + (step * 0.5f)));



				bool nrender = start > a;

				Drawing::DrawRect(end2d.x, end2d.y, 5.f, 5.f, ctx.m_settings.menu_color.alpha(nrender ? 10 : 230));
				Drawing::DrawOutlinedRect(end2d.x - 1, end2d.y - 1, 7.f, 7.f, Color(38, 38, 38, 240));

				prev_a = a;
			}
		}
	}

	indicators_count = 0;

	if ( ctx.m_settings.watermark && ctx.screen_size.x > 0.0 && ctx.screen_size.y > 0.0 ) {
		Drawing::DrawRect( ctx.screen_size.x - 196, 10, 190, 18, Color( 35, 35, 35, 150 ) );
		Drawing::DrawRect( ctx.screen_size.x - 196, 10, 190, 2.0f, ctx.flt2color( ctx.m_settings.f_menu_color ) );
		Drawing::DrawRectGradientVertical( ctx.screen_size.x - 196, 10, 2.0f, 18, ctx.flt2color( ctx.m_settings.f_menu_color ), Color( 35, 35, 35, 100 ) );
		Drawing::DrawRectGradientVertical( ctx.screen_size.x - 8, 10, 2.0f, 18, ctx.flt2color( ctx.m_settings.f_menu_color ), Color( 35, 35, 35, 100 ) );
		Drawing::DrawString( F::ESP, ctx.screen_size.x - 100, 15, Color( 255.0, 255.0, 255.0, 255.0 ), FONT_CENTER, sxor( "developer | deathhole [ beta ]" ) );
	}

	if (ctx.m_settings.visuals_extra_windows[0] && ctx.screen_size.x > 0.0 && ctx.screen_size.y > 0.0)
	{
		static double prev_alpha = 0;
		auto spectators = 1;
		static auto d = Drawing::GetTextSize(F::ESP, "H");
		static auto lol = get_spectators();
		static auto tick = csgo.m_globals()->tickcount;

		if (tick != csgo.m_globals()->tickcount) {
			lol = get_spectators();
			tick = csgo.m_globals()->tickcount;
		}

		const Vector2D spectator_size = { 150, int(lol.size()) * float(d.bottom) + 20 };

		if ((ctx.m_settings.visuals_spectators_pos + spectator_size) > ctx.screen_size)
		{
			if ((ctx.m_settings.visuals_spectators_pos.x + spectator_size.x) >= ctx.screen_size.x)
				ctx.m_settings.visuals_spectators_pos.x = ctx.screen_size.x - 150.0;

			if ((ctx.m_settings.visuals_spectators_pos.y + spectator_size.y) >= ctx.screen_size.y)
				ctx.m_settings.visuals_spectators_pos.y = ctx.screen_size.y - (int(lol.size()) * int(d.bottom) + 15);
		}
		else if (ctx.m_settings.visuals_spectators_pos < Vector2D(0, 0)) {
			if (ctx.m_settings.visuals_spectators_pos.x < 0.0)
				ctx.m_settings.visuals_spectators_pos.x = 0.0;

			if (ctx.m_settings.visuals_spectators_pos.y < 0.0)
				ctx.m_settings.visuals_spectators_pos.y = 0.0;
		}

		if (feature::menu->_menu_opened)
			prev_alpha = 255.f;

		if (csgo.m_client()->IsChatRaised())
			prev_alpha = 10.0;

		Drawing::DrawRect		(ctx.m_settings.visuals_spectators_pos.x, ctx.m_settings.visuals_spectators_pos.y + 1.0, spectator_size.x, spectator_size.y + 2.0, Color(35, 35, 35, prev_alpha * float(ctx.m_settings.visuals_spectators_alpha / 130.0)));
		Drawing::DrawOutlinedRect(ctx.m_settings.visuals_spectators_pos.x - 1.0, ctx.m_settings.visuals_spectators_pos.y, spectator_size.x + 2.0, spectator_size.y + 4.0, Color(10, 10, 10, prev_alpha * float(ctx.m_settings.visuals_spectators_alpha / 130.0)));
		Drawing::DrawRect		(ctx.m_settings.visuals_spectators_pos.x - 1.0, ctx.m_settings.visuals_spectators_pos.y, spectator_size.x + 2.0, 2.0, ctx.flt2color( ctx.m_settings.f_menu_color ).alpha(prev_alpha * float(ctx.m_settings.visuals_spectators_alpha / 100.f)));
		Drawing::DrawString(F::ESP, ctx.m_settings.visuals_spectators_pos.x + spectator_size.x / 2.0, ctx.m_settings.visuals_spectators_pos.y + 3.0, Color(255.0, 255.0, 255.0, prev_alpha * float(ctx.m_settings.visuals_spectators_alpha / 100.f)), FONT_CENTER, "%s [%d]", sxor("spectators"), (int)lol.size());

		Drawing::DrawRectGradientHorizontal( ctx.m_settings.visuals_spectators_pos.x, ctx.m_settings.visuals_spectators_pos.y + ( d.bottom + 4.0 ), spectator_size.x / 2, 2.0, Color( 35, 35, 35, prev_alpha * double( ctx.m_settings.visuals_spectators_alpha / 130.0 ) ), ctx.flt2color( ctx.m_settings.f_menu_color ).alpha( prev_alpha * double( ctx.m_settings.visuals_spectators_alpha / 130.0 ) ) );
		Drawing::DrawRectGradientHorizontal( ctx.m_settings.visuals_spectators_pos.x + spectator_size.x / 2, ctx.m_settings.visuals_spectators_pos.y + ( d.bottom + 4.0 ), spectator_size.x / 2 - 1.0, 2.0, ctx.flt2color( ctx.m_settings.f_menu_color ).alpha( prev_alpha * double( ctx.m_settings.visuals_spectators_alpha / 130.0 ) ), Color( 35, 35, 35, prev_alpha * double( ctx.m_settings.visuals_spectators_alpha / 130.0 ) ) );

		if (!lol.empty())
		{
			if (prev_alpha < 255.0 && !csgo.m_client()->IsChatRaised())
				prev_alpha += min(5.0, 255.0 - prev_alpha);

			for (int spec : lol)
			{
				player_info spec_inf;
				csgo.m_engine()->GetPlayerInfo(spec, &spec_inf);

				auto size = Drawing::GetTextSize(F::ESP, spec_inf.name);
				std::string name(spec_inf.name);

				if (name.length() > 20)
					name.resize(20);

				Drawing::DrawString(F::ESP, ctx.m_settings.visuals_spectators_pos.x + 3.0f, ctx.m_settings.visuals_spectators_pos.y + 6.0f + 2.0f + (d.bottom * spectators), Color(255, 255, 255, prev_alpha), FONT_LEFT, name.c_str());

				spectators++;
			}
		}
		else
		{
			if (prev_alpha > 0.f && !feature::menu->_menu_opened)
				prev_alpha -= min(5, prev_alpha);
		}

		if (feature::menu->_menu_opened && !was_moved_hotkeys)
		{
			if (ctx.pressed_keys[1] && feature::menu->mouse_in_pos(Vector(ctx.m_settings.visuals_spectators_pos.x, ctx.m_settings.visuals_spectators_pos.y, 0), Vector(ctx.m_settings.visuals_spectators_pos.x + spectator_size.x, ctx.m_settings.visuals_spectators_pos.y + 15.0, 0)) || was_moved)
			{
				if (save_pos == false)
				{
					saved_x = feature::menu->_cursor_position.x - ctx.m_settings.visuals_spectators_pos.x;
					saved_y = feature::menu->_cursor_position.y - ctx.m_settings.visuals_spectators_pos.y;
					save_pos = true;
				}
				ctx.m_settings.visuals_spectators_pos.x = feature::menu->_cursor_position.x;
				ctx.m_settings.visuals_spectators_pos.y = feature::menu->_cursor_position.y;
				ctx.m_settings.visuals_spectators_pos.x = ctx.m_settings.visuals_spectators_pos.x - saved_x;
				ctx.m_settings.visuals_spectators_pos.y = ctx.m_settings.visuals_spectators_pos.y - saved_y;
			}
			else
				save_pos = was_moved = false;

			if (!was_moved)
				was_moved = ctx.pressed_keys[1] && feature::menu->mouse_in_pos(Vector(ctx.m_settings.visuals_spectators_pos.x, ctx.m_settings.visuals_spectators_pos.y, 0), Vector(ctx.m_settings.visuals_spectators_pos.x + spectator_size.x, ctx.m_settings.visuals_spectators_pos.y + 15.0, 0));
			else
				was_moved = ctx.pressed_keys[1];
		}
		else
			was_moved = false;
	}

	if (ctx.m_settings.visuals_extra_windows[1] && ctx.screen_size.x > 0 && ctx.screen_size.y > 0)
	{
		static float prev_alpha = 0;

		auto spectators = 1;

		static auto d = Drawing::GetTextSize(F::ESP, "H");

		int working_keybinds = 0;

		auto lol = get_keybinds(working_keybinds);

		Vector2D spectator_size = { 150.0f, working_keybinds * d.bottom + 20.0f };

		if ((ctx.m_settings.visuals_keybinds_pos + spectator_size) > ctx.screen_size)
		{
			if ((ctx.m_settings.visuals_keybinds_pos.x + spectator_size.x) >= ctx.screen_size.x)
				ctx.m_settings.visuals_keybinds_pos.x = ctx.screen_size.x - 150.0;

			if ((ctx.m_settings.visuals_keybinds_pos.y + spectator_size.y) >= ctx.screen_size.y)
				ctx.m_settings.visuals_keybinds_pos.y = ctx.screen_size.y - (float(working_keybinds * d.bottom) + 15.0f);
		}
		else if (ctx.m_settings.visuals_keybinds_pos < Vector2D(0, 0)) {
			if (ctx.m_settings.visuals_keybinds_pos.x < 0)
				ctx.m_settings.visuals_keybinds_pos.x = 0;

			if (ctx.m_settings.visuals_keybinds_pos.y < 0)
				ctx.m_settings.visuals_keybinds_pos.y = 0;
		}

		if (feature::menu->_menu_opened)
			prev_alpha = 255;

		if (csgo.m_client()->IsChatRaised())
			prev_alpha = 10;

		if (ctx.m_settings.visuals_keybinds_alpha > 2) {
			Drawing::DrawRect(ctx.m_settings.visuals_keybinds_pos.x, ctx.m_settings.visuals_keybinds_pos.y + 1.0, spectator_size.x, spectator_size.y + 2.0, Color(35, 35, 35, prev_alpha * float(ctx.m_settings.visuals_keybinds_alpha / 130.0)));
			Drawing::DrawOutlinedRect(ctx.m_settings.visuals_keybinds_pos.x - 1.0, ctx.m_settings.visuals_keybinds_pos.y, spectator_size.x + 2.0, spectator_size.y + 4.0, Color(10, 10, 10, prev_alpha * float(ctx.m_settings.visuals_keybinds_alpha / 130.0)));
			Drawing::DrawRect(ctx.m_settings.visuals_keybinds_pos.x - 1.0, ctx.m_settings.visuals_keybinds_pos.y, spectator_size.x + 2.0, 2, ctx.flt2color( ctx.m_settings.f_menu_color ).alpha(prev_alpha * float(ctx.m_settings.visuals_keybinds_alpha / 100.0)));
			Drawing::DrawString(F::ESP, ctx.m_settings.visuals_keybinds_pos.x + spectator_size.x / 2.0, ctx.m_settings.visuals_keybinds_pos.y + 3.0, Color(255.0, 255.0, 255.0, prev_alpha * float(ctx.m_settings.visuals_keybinds_alpha / 100.0)), FONT_CENTER, sxor("active keys"));

			Drawing::DrawRectGradientHorizontal(ctx.m_settings.visuals_keybinds_pos.x, ctx.m_settings.visuals_keybinds_pos.y + (d.bottom + 4.0), spectator_size.x / 2, 2.0, Color( 35, 35, 35, prev_alpha* double( ctx.m_settings.visuals_spectators_alpha / 130.0 ) ), ctx.flt2color( ctx.m_settings.f_menu_color ).alpha(prev_alpha * float(ctx.m_settings.visuals_keybinds_alpha / 130.0)));
			Drawing::DrawRectGradientHorizontal(ctx.m_settings.visuals_keybinds_pos.x + spectator_size.x / 2, ctx.m_settings.visuals_keybinds_pos.y + (d.bottom + 4.0), spectator_size.x / 2 - 1.0, 2.0, ctx.flt2color( ctx.m_settings.f_menu_color ).alpha( prev_alpha* double( ctx.m_settings.visuals_spectators_alpha / 130.0 ) ), Color( 35, 35, 35, prev_alpha* double( ctx.m_settings.visuals_spectators_alpha / 130.0 ) ) );
		}

		if (working_keybinds > 0)
		{
			if (prev_alpha < 255 && !csgo.m_client()->IsChatRaised())
				prev_alpha += min(5, 255 - prev_alpha);

			for (auto bind : lol)
			{
				if (((csgo.m_globals()->realtime - keybinds_time[bind.index]) * 2.f) > 0.89f)
					continue;

				auto size = Drawing::GetTextSize(F::ESP, bind.name);
				auto new_alpha = min(prev_alpha, 250 * min(1.0f, 1.0f - abs(csgo.m_globals()->realtime - keybinds_time[bind.index]) * 2.0f));

				Drawing::DrawString(F::ESP, ctx.m_settings.visuals_keybinds_pos.x + 3.0f + (ctx.m_settings.visuals_keybinds_alpha == 1 ? spectator_size.x / 2 : 0), ctx.m_settings.visuals_keybinds_pos.y + 6.0f + 2.0f + float(d.bottom * spectators), Color(255, 255, 255, new_alpha), ctx.m_settings.visuals_keybinds_alpha == 1 ? FONT_CENTER : FONT_LEFT, "%s", bind.name);

				if (ctx.m_settings.visuals_keybinds_alpha > 1) {
					if (ctx.active_keybinds[bind.index].mode > 0 || bind.index >= 7 && bind.index < 15)
						Drawing::DrawString(F::ESP, ctx.m_settings.visuals_keybinds_pos.x + spectator_size.x - 3.0f, ctx.m_settings.visuals_keybinds_pos.y + 6.0f + 2.0f + float(d.bottom * spectators), Color(255, 255, 255, new_alpha), FONT_RIGHT, "%s", (bind.index >= 7 && bind.index < 11 ? _type[4].c_str() : bind.type));
				}

				spectators++;
			}
		}
		else
		{
			if (prev_alpha > 0 && !feature::menu->_menu_opened)
				prev_alpha -= min(5, prev_alpha);
		}

		if (feature::menu->_menu_opened && !was_moved)
		{
			if (ctx.pressed_keys[1] && feature::menu->mouse_in_pos(Vector(ctx.m_settings.visuals_keybinds_pos.x, ctx.m_settings.visuals_keybinds_pos.y, 0), Vector(ctx.m_settings.visuals_keybinds_pos.x + spectator_size.x, ctx.m_settings.visuals_keybinds_pos.y + 15.f, 0)) || was_moved_hotkeys)
			{
				if (save_pos_hotkeys == false)
				{
					saved_x_hotkeys = feature::menu->_cursor_position.x - ctx.m_settings.visuals_keybinds_pos.x;
					saved_y_hotkeys = feature::menu->_cursor_position.y - ctx.m_settings.visuals_keybinds_pos.y;
					save_pos_hotkeys = true;
				}
				ctx.m_settings.visuals_keybinds_pos.x = feature::menu->_cursor_position.x;
				ctx.m_settings.visuals_keybinds_pos.y = feature::menu->_cursor_position.y;
				ctx.m_settings.visuals_keybinds_pos.x = ctx.m_settings.visuals_keybinds_pos.x - saved_x_hotkeys;
				ctx.m_settings.visuals_keybinds_pos.y = ctx.m_settings.visuals_keybinds_pos.y - saved_y_hotkeys;
			}
			else
				save_pos_hotkeys = was_moved_hotkeys = false;

			if (!was_moved_hotkeys)
				was_moved_hotkeys = ctx.pressed_keys[1] && feature::menu->mouse_in_pos(Vector(ctx.m_settings.visuals_keybinds_pos.x, ctx.m_settings.visuals_keybinds_pos.y, 0), Vector(ctx.m_settings.visuals_keybinds_pos.x + spectator_size.x, ctx.m_settings.visuals_keybinds_pos.y + 15.f, 0));
			else
				was_moved_hotkeys = ctx.pressed_keys[1];
		}
		else
			was_moved_hotkeys = false;
	}

	if (ctx.m_local() == nullptr || !csgo.m_engine()->IsInGame()) return;

	static auto linegoesthrusmoke = Memory::Scan(sxor("client.dll"), sxor("55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0"));
	static auto smokecout = *reinterpret_cast<DWORD*>(linegoesthrusmoke + 0x8);

	if (ctx.m_settings.visuals_no_smoke)
		*reinterpret_cast<int*>(smokecout) = 0;

	// SKEET COLORS ARE IN ABGR
	
	const int centerX = ctx.screen_size.x / 2, centerY = ctx.screen_size.y / 2;
	render_tracers();
	
	if (m_weapon() != nullptr && !ctx.m_local()->IsDead() && (DWORD)ctx.m_local()->get_animation_state() > 0x5000)
	{
		if (ctx.m_local()->m_bIsScoped() && ctx.m_settings.visuals_no_scope)
		{
			if ( ctx.m_settings.visuals_no_scope_type == 0 ) {
				Drawing::DrawLine( 0, centerY, centerX + 1, centerY, Color::Black( ) );
				Drawing::DrawLine( centerX - 1, centerY, ctx.screen_size.x, centerY, Color::Black( ) );

				Drawing::DrawLine( centerX, 0, centerX, centerY + 1, Color::Black( ) );
				Drawing::DrawLine( centerX, centerY - 1, centerX, ctx.screen_size.y, Color::Black( ) );
			}
			else if ( ctx.m_settings.visuals_no_scope_type == 1 ) {
				static int width, height;
				csgo.m_engine( )->GetScreenSize( width, height );

				Drawing::DrawRectGradientVertical( width / 2, height / 2 - 165, 1, 150, Color( 255, 0, 0, 0 ), ctx.flt2color( ctx.m_settings.no_scope_color ) );
				Drawing::DrawRectGradientHorizontal( width / 2 - 170, height / 2, 150, 1, Color( 255, 0, 0, 0 ), ctx.flt2color( ctx.m_settings.no_scope_color ) );
				Drawing::DrawRectGradientHorizontal( width / 2 + 19, height / 2, 150, 1, ctx.flt2color( ctx.m_settings.no_scope_color ), Color( 255, 0, 0, 0 ) );
				Drawing::DrawRectGradientVertical( width / 2, height / 2 + 15, 1, 150, ctx.flt2color( ctx.m_settings.no_scope_color ), Color( 255, 0, 0, 0 ) );
			}
		}

		auto color = ctx.autowall_crosshair == 0 ? Color::Red() : (ctx.autowall_crosshair == 2 ? Color::Green() : Color::Red( ) );

		if (ctx.m_settings.visuals_autowall_crosshair) {
			Drawing::DrawRect(ctx.screen_size.x / 2.f, ctx.screen_size.y / 2.f, 6, 6, color);
			Drawing::DrawRect(ctx.screen_size.x / 2.f, ctx.screen_size.y / 2.f, 6, 6, color);
		}


		if (csgo.m_engine()->IsInGame())
		{

			static c_indicator lc_indicator("LC", 1);

			if (ctx.m_local()->m_vecVelocity().Length2D() > 260) {
				lc_indicator.draw(1.f, (ctx.breaks_lc ? Color(123, 194, 21, csgo.m_client()->IsChatRaised() ? 5 : 250) : Color::Red(csgo.m_client()->IsChatRaised() ? 5 : 250)));
			}
			else
				indicators_count++;
			if (csgo.m_engine()->IsInGame() && ctx.has_exploit_toggled && (ctx.exploit_allowed || !ctx.fakeducking)) {

				static float draw_factor = 0.f;

				if (ctx.ticks_allowed > 2 && (csgo.m_client_state()->m_iChockedCommands < 2 || ctx.is_charging)) {
					draw_factor += min(1.f - draw_factor, csgo.m_globals()->frametime * 3.0f);

					if (draw_factor > 1)
						draw_factor = 1;
				}
				else
				{
					draw_factor -= min(draw_factor, csgo.m_globals()->frametime * 4.0f);

					if (draw_factor < 0)
						draw_factor = 0;
				}
					const auto text_size = Drawing::GetTextSize(F::LBY, ctx.main_exploit == 1 ? sxor("HS") : sxor("DT"));
					Drawing::DrawString(F::LBY,
						10,
						ctx.screen_size.y - 88 - 26 * indicators_count,
						Color(30, 30, 30, csgo.m_client()->IsChatRaised() ? 5 : 250),
						FONT_LEFT,
						ctx.main_exploit == 1 ? sxor("HS") : sxor("DT"));

					*reinterpret_cast<bool*>(uintptr_t(csgo.m_surface()) + 0x280) = true;
					int x, y, x1, y1;
					Drawing::GetDrawingArea(x, y, x1, y1);
					Drawing::LimitDrawingArea(0, ctx.screen_size.y - 88 - 26 * indicators_count + text_size.bottom * (1.f - draw_factor), text_size.right + 10, (int)text_size.bottom);
					Drawing::DrawString(F::LBY,
						10,
						ctx.screen_size.y - 88 - 26 * indicators_count++,
						ctx.ticks_allowed > 13 && !ctx.fakeducking && ctx.exploit_allowed ? Color(123, 194, 21, csgo.m_client()->IsChatRaised() ? 5 : 250) : Color::Red(csgo.m_client()->IsChatRaised() ? 5 : 250),
						FONT_LEFT,
						ctx.main_exploit == 1 ? sxor("HS") : sxor("DT"));
					Drawing::LimitDrawingArea(x, y, x1, y1);
					*reinterpret_cast<bool*>(uintptr_t(csgo.m_surface()) + 0x280) = false;
				
			}
			if (ctx.main_exploit >= 2)
			{
				static float prev_alpha = 1.f;
				static float prev_alpha_first_bullet = 1.f;

				if (ctx.m_local() && !ctx.m_local()->IsDead() && m_weapon() && m_weapon()->can_shoot() && ctx.latest_weapon_data) {
					prev_alpha_first_bullet += min(1.f - prev_alpha_first_bullet, csgo.m_globals()->frametime * 2.0f);

					if (m_weapon()->can_exploit(ctx.latest_weapon_data->flCycleTime + 0.01f))
						prev_alpha_first_bullet = 1;

					if (prev_alpha_first_bullet > 1)
						prev_alpha_first_bullet = 1;
				}
				else
				{
					prev_alpha_first_bullet -= min(prev_alpha_first_bullet, csgo.m_globals()->frametime * 5.0f);

					if (prev_alpha_first_bullet < 0)
						prev_alpha_first_bullet = 0;
				}

				if (ctx.has_exploit_toggled && ctx.exploit_allowed && ctx.main_exploit >= 2) {
					if (ctx.ticks_allowed > 2 && prev_alpha_first_bullet >= 1) {
						prev_alpha += min(1.f - prev_alpha, csgo.m_globals()->frametime * 2.0f);

						if (prev_alpha > 1)
							prev_alpha = 1;
					}
					else
					{
						prev_alpha -= min(prev_alpha, csgo.m_globals()->frametime * 4.0f);

						if (prev_alpha < 0)
							prev_alpha = 0;
					}

					Drawing::DrawString(F::Icons, 45, ctx.screen_size.y - 88 - 26 * (indicators_count - 1) + 14, Color::White(prev_alpha * 255), FONT_LEFT, "u");
				}


				Drawing::DrawString(F::Icons, 45, ctx.screen_size.y - 88 - 26 * (indicators_count - 1) + 3, Color::White(prev_alpha_first_bullet * 255), FONT_LEFT, "u");


			}
		}


		if (csgo.m_engine()->IsInGame())
		{

			static c_indicator fake_indicator("FAKE", 1);
			fake_indicator.draw(1.f, g_ColorByInt(feature::anti_aim->max_delta, 60));


		}

		if (ctx.m_settings.misc_visuals_indicators_2[7])
		{
			static bool filled[3] = { false,false,false };

			auto left_pos = Vector2D(ctx.screen_size.x/2 - 45, ctx.screen_size.y / 2), right_pos = Vector2D(ctx.screen_size.x / 2 + 45, ctx.screen_size.y / 2), down_pos = Vector2D(ctx.screen_size.x / 2, ctx.screen_size.y / 2 + 45), siz = Vector2D(8, 8);

			static std::vector< Vertex_t > vertices_left =
			{
				Vertex_t{ Vector2D(left_pos.x - siz.x, left_pos.y + siz.y), Vector2D() },
				Vertex_t{ Vector2D(left_pos.x, left_pos.y - siz.y), Vector2D() },
				Vertex_t{ left_pos + siz, Vector2D() }
			};

			static std::vector< Vertex_t > vertices_right =
			{
				Vertex_t{ Vector2D(right_pos.x - siz.x, right_pos.y + siz.y), Vector2D() },
				Vertex_t{ Vector2D(right_pos.x, right_pos.y - siz.y), Vector2D() },
				Vertex_t{ right_pos + siz, Vector2D() }
			};

			static std::vector< Vertex_t > vertices_down =
			{
				Vertex_t{ Vector2D(down_pos.x - siz.x, down_pos.y + siz.y), Vector2D() },
				Vertex_t{ Vector2D(down_pos.x, down_pos.y - siz.y), Vector2D() },
				Vertex_t{ down_pos + siz, Vector2D() }
			};

			if (reset) {
				filled[0] = false;
				filled[1] = false;
				filled[2] = false;
			}

			if (!filled[0]) {
				vertices_left =
				{
					Vertex_t{ Vector2D(left_pos.x - siz.x, left_pos.y + siz.y), Vector2D() },
					Vertex_t{ Vector2D(left_pos.x, left_pos.y - siz.y), Vector2D() },
					Vertex_t{ left_pos + siz, Vector2D() }
				};

				for (unsigned int p = 0; p < vertices_left.size(); p++) {
					Drawing::rotate_point(vertices_left[p].m_Position, left_pos, false, 3.15);
				}
				filled[0] = true;
			}

			if (!filled[1]) {
				vertices_right =
				{
					Vertex_t{ Vector2D(right_pos.x - siz.x, right_pos.y + siz.y), Vector2D() },
					Vertex_t{ Vector2D(right_pos.x, right_pos.y - siz.y), Vector2D() },
					Vertex_t{ right_pos + siz, Vector2D() }
				};

				for (unsigned int p = 0; p < vertices_right.size(); p++) {
					Drawing::rotate_point(vertices_right[p].m_Position, right_pos, false, 0);
				}
				filled[1] = true;
			}

			if (!filled[2]) {
				vertices_down =
				{
					Vertex_t{ Vector2D(down_pos.x - siz.x, down_pos.y + siz.y), Vector2D() },
					Vertex_t{ Vector2D(down_pos.x, down_pos.y - siz.y), Vector2D() },
					Vertex_t{ down_pos + siz, Vector2D() }
				};

				for (unsigned int p = 0; p < vertices_down.size(); p++) {
					Drawing::rotate_point(vertices_down[p].m_Position, down_pos, false, 61.25);
				}
				filled[2] = true;
			}

			auto fc = (ctx.side == 0 ? ctx.m_settings.menu_color.alpha(160) : Color::Black(130));
			auto sc = (ctx.side == 1 ? ctx.m_settings.menu_color.alpha(160) : Color::Black(130));
			auto tc = (ctx.side == 2 ? ctx.m_settings.menu_color.alpha(160) : Color::Black(130));

			Drawing::TexturedPolygon(vertices_left.size(), vertices_left, fc); //Color(50, 122, 239, 200)
			Drawing::TexturedPolygon(vertices_right.size(), vertices_right, sc);
			Drawing::TexturedPolygon(vertices_down.size(), vertices_down, tc);

		}



		static float alpha = 0;

		if (ctx.hurt_time > csgo.m_globals()->realtime)
			alpha = 255;
		else
			alpha = alpha - 255 / 0.3 * csgo.m_globals()->frametime;

		if (alpha > 0 && ctx.m_settings.misc_visuals_indicators_2[0]) {
			Drawing::DrawLine(ctx.screen_size.x / 2 - linesize, ctx.screen_size.y / 2 - linesize, ctx.screen_size.x / 2 - linedec, ctx.screen_size.y / 2 - linedec, Color(200, 200, 200, alpha));
			Drawing::DrawLine(ctx.screen_size.x / 2 - linesize, ctx.screen_size.y / 2 + linesize, ctx.screen_size.x / 2 - linedec, ctx.screen_size.y / 2 + linedec, Color(200, 200, 200, alpha));
			Drawing::DrawLine(ctx.screen_size.x / 2 + linesize, ctx.screen_size.y / 2 + linesize, ctx.screen_size.x / 2 + linedec, ctx.screen_size.y / 2 + linedec, Color(200, 200, 200, alpha));
			Drawing::DrawLine(ctx.screen_size.x / 2 + linesize, ctx.screen_size.y / 2 - linesize, ctx.screen_size.x / 2 + linedec, ctx.screen_size.y / 2 - linedec, Color(200, 200, 200, alpha));
		}
	}

	if (ctx.current_tickcount % 2 == 1)
	{

		if (ctx.time_to_reset_sound > 0.f && ctx.time_to_reset_sound <= csgo.m_globals()->realtime) {

			feature::music_player->stop();
			ctx.time_to_reset_sound = 0.f;
		}
	}

	if (!ctx.m_settings.visuals_enabled)
		return;

	static auto max_bombtime = csgo.m_engine_cvars()->FindVar("mp_c4timer");
	static auto mp_teammates_are_enemies = csgo.m_engine_cvars()->FindVar("mp_teammates_are_enemies");
	player_info info;
	int x, y, w, h;

	auto radar_base = feature::find_hud_element<DWORD>(sxor("CCSGO_HudRadar"));
	auto hud_radar = (CCSGO_HudRadar*)(radar_base + 0x74);

	feature::sound_parser->draw_sounds();

	for (auto k = 0; k < csgo.m_entity_list()->GetHighestEntityIndex(); k++)
	{
		C_BasePlayer* entity = csgo.m_entity_list()->GetClientEntity(k);

		if (entity == nullptr ||
			!entity->GetClientClass() ||
			entity == ctx.m_local() ||
			((DWORD)entity->GetClientRenderable() < 0x1000))
			continue;

		if (entity->GetClientClass()->m_ClassID == class_ids::CCSPlayer && k < 64)
		{


			if (!entity 
				|| entity->m_iTeamNum() == ctx.m_local()->m_iTeamNum() && ((int)mp_teammates_are_enemies < 1000 || !mp_teammates_are_enemies->GetBool())
				|| !csgo.m_engine()->GetPlayerInfo(entity->entindex(), &info)) continue;

			const auto idx = entity->entindex() - 1;

			const auto is_teammate = (entity->m_iTeamNum() == ctx.m_local()->m_iTeamNum());

			auto &radar_info = *reinterpret_cast<RadarPlayer_t*>(radar_base + (0x174 * (k + 1)) - 0x3C);

			auto was_spotted = (entity->m_bSpotted() || radar_info.spotted) && hud_radar && radar_base && !is_teammate;

			if (ctx.m_settings.misc_engine_radar && !entity->IsDormant() && entity->m_iHealth() > 0)
				entity->m_bSpotted() = true;

			c_player_records* log = &feature::lagcomp->records[idx];
			resolver_records* r_log = &feature::resolver->player_records[idx];

			if (entity->IsDormant() && was_spotted && TIME_TO_TICKS(csgo.m_globals()->curtime - entity->m_flSpawnTime()) > 5) {
				log->saved_hp = radar_info.health;
			}

			if (entity->IsDormant() && ctx.m_settings.esp_dormant && abs(log->last_sound - csgo.m_globals()->realtime) < 3.f && !log->render_origin.IsZero() && log->render_origin.IsValid()/*&& dormant_alpha[idx] > 0*/) {
				auto dd = 255 * (1.0f - min(1.0f, abs(log->last_sound - csgo.m_globals()->realtime) * 0.3f));

				Vector mins, maxs;
				Vector bottom, top;

				mins = csgo.m_movement()->GetPlayerMins(log->dormant_flags & FL_DUCKING);
				maxs = csgo.m_movement()->GetPlayerMaxs(log->dormant_flags & FL_DUCKING);

				mins = { log->render_origin.x, log->render_origin.y, log->render_origin.z + mins.z };
				maxs = { log->render_origin.x, log->render_origin.y, log->render_origin.z + maxs.z + 8.f };

				if (!Drawing::WorldToScreen(mins, bottom) || !Drawing::WorldToScreen(maxs, top))
					continue;

				int h = bottom.y - top.y;
				int y = bottom.y - h;
				int w = h / 2.f;
				int x = bottom.x - (w / 2.f);

				int right = 0;
				int down = 0;

				static auto size_info = 9.f;

				auto box_color = ctx.flt2color(ctx.m_settings.box_enemy_color);
				auto skeletons_color = ctx.flt2color(ctx.m_settings.colors_skeletons_enemy);
				auto name_color = ctx.flt2color(ctx.m_settings.colors_esp_name);

				int lol = 0;

				if (ctx.m_settings.esp_box) {
					Drawing::DrawOutlinedRect(x, y, w, h, box_color.alpha(min(box_color.a(), dd)));
				}

				if (ctx.m_settings.esp_name) {
					auto text_size = Drawing::GetTextSize(F::ESP, log->saved_info.name);
					Drawing::DrawString(F::ESP, x + w / 2 - text_size.right / 2, y - 14, name_color.alpha(min(name_color.a(), dd - 25.f)), FONT_LEFT, "%s", log->saved_info.name);
				}

				if (ctx.m_settings.esp_health) {
					int hp = log->saved_hp;

					if (hp > 100)
						hp = 100;

					int hp_percent = h - (int)((h * hp) / 100);

					int width = (w * (hp / 100.f));

					int red = 0x50;
					int green = 0xFF;
					int blue = 0x50;

					if (hp >= 25)
					{
						if (hp < 50)
						{
							red = 0xD7;
							green = 0xC8;
							blue = 0x50;
						}
					}
					else
					{
						red = 0xFF;
						green = 0x32;
						blue = 0x50;
					}

					char hps[10] = "";

					sprintf(hps, "%iHP", hp);

					auto text_size = Drawing::GetTextSize(F::ESPInfo, hps);

					Drawing::DrawRect(x - 5, y - 1, 4, h + 2, Color(80, 80, 80, dd * 0.49f));
					Drawing::DrawOutlinedRect(x - 5, y - 1, 4, h + 2, Color(10, 10, 10, (dd * 0.5f)));
					Drawing::DrawRect(x - 4, y + hp_percent, 2, h - hp_percent, Color(red, green, 0, dd));

					if (hp < 93)
						Drawing::DrawString(F::ESPInfo, x - text_size.right - 6, y - 1, Color(255, 255, 255, dd - 55.f), FONT_LEFT, hps);
				}

				if (ctx.m_settings.esp_flags[0] && entity->m_ArmorValue() > 1)
					Drawing::DrawString(F::ESPInfo, x + w + 3, y + right++ * size_info, Color(255, 255, 255, min(0xC8, dormant_alpha[idx] - 10)), FONT_LEFT, (entity->m_bHasHelmet() && entity->m_ArmorValue() > 0 ? _hk.operator std::string().c_str() : (entity->m_bHasHelmet() ? _h.operator std::string().c_str() : _k.operator std::string().c_str())));

				continue;
			}

			if (entity->m_iHealth() <= 0)
				continue;

			log->saved_hp = entity->m_iHealth();
			log->render_origin = Math::interpolate(log->render_origin, entity->get_abs_origin(), Math::clamp((csgo.m_globals()->realtime - log->last_sound) * csgo.m_globals()->frametime, 0, 1));


			dormant_alpha[idx] = 255 * (1.f - min(1.f, abs(entity->m_flSimulationTime() - TICKS_TO_TIME(csgo.m_globals()->tickcount)) * 0.3f));

			if (ctx.m_settings.esp_offscreen && !is_teammate)
				offscreen_esp(entity, max(1, dormant_alpha[idx]) / 255);

			auto bk = entity->m_vecOrigin();
			entity->m_vecOrigin() = log->render_origin;
			if (!get_espbox(entity, x, y, w, h)) { entity->m_vecOrigin() = bk; continue; }
			entity->m_vecOrigin() = bk;

			if (log)
				log->last_esp_box.Set(x, y, w, h);

			int right = 0;
			int down = 0;

			auto box_color = ctx.flt2color(ctx.m_settings.box_enemy_color);
			auto skeletons_color = ctx.flt2color(ctx.m_settings.colors_skeletons_enemy);
			auto name_color = ctx.flt2color(ctx.m_settings.colors_esp_name);
			auto ammo_color = ctx.flt2color(ctx.m_settings.colors_esp_ammo);

			const int lol = (is_teammate ? 1 : 0);

			if (ctx.m_settings.esp_box) {
				Drawing::DrawOutlinedRect(x, y, w, h, box_color.alpha(min(box_color.a(), dormant_alpha[idx])));
				Drawing::DrawOutlinedRect(x - 1, y - 1, w + 2, h + 2, Color(10, 10, 10, min(box_color.a(), dormant_alpha[idx] * 0.8)));
				Drawing::DrawOutlinedRect(x + 1, y + 1, w - 2, h - 2, Color(10, 10, 10, min(box_color.a(), dormant_alpha[idx] * 0.8)));
			}

			if (ctx.m_settings.esp_name)
				Drawing::DrawString(F::ESP, x + w / 2, y - 14, name_color.alpha(min(name_color.a(), dormant_alpha[idx] - 10)), FONT_CENTER, "%s", info.name);

			if (ctx.m_settings.esp_health) {
				int hp = entity->m_iHealth( );

				if ( hp > 100 )
					hp = 100;

				int hp_percent = h - ( int )( ( h * hp ) / 100 );

				int width = ( w * ( hp / 100.f ) );

				int red = 255 - ( hp * 2.55 );
				int green = hp * 2.55;

				char hps[ 10 ] = "";

				sprintf( hps, "%iHP", hp );

				auto text_size = Drawing::GetTextSize( F::ESPInfo, hps );

				Drawing::DrawRect( x - 5, y - 1, 4, h + 2, Color( 80, 80, 80, dormant_alpha[ idx ] * 0.49f ) );
				Drawing::DrawOutlinedRect( x - 5, y - 1, 4, h + 2, Color( 10, 10, 10, ( dormant_alpha[ idx ] * 0.8f ) ) );
				Drawing::DrawRect( x - 4, y + hp_percent, 2, h - hp_percent, Color( red, green, 0, dormant_alpha[ idx ] ) );

				if ( hp < 90 )
					Drawing::DrawString( F::ESPInfo, x - text_size.right - 6, y - 1, Color( 255, 255, 255, dormant_alpha[ idx ] - 55.f ), FONT_LEFT, hps );
				//Drawing::DrawRectGradientHorizontal(x - 3, y + hp_percent, 14, 11, Color(0, 0, 0, 0), Color(7, 39, 17, dormant_alpha[idx] - 35));

				float height = h / 10.f;
			}

			if (ctx.m_settings.esp_flags[0] && entity->m_ArmorValue() > 1)
				Drawing::DrawString( F::ESPInfo, x + w + 3, y + right++ * size_info, Color( 255, 255, 255, dormant_alpha[ idx ] - 55.f ), FONT_LEFT, ( entity->m_bHasHelmet( ) ? "HK" : "K" ) );

			if (ctx.m_settings.esp_flags[1] && strlen(entity->m_szLastPlaceName()) > 1) {


				const char* last_place = entity->m_szLastPlaceName();

				if (last_place && *last_place)
				{
					const wchar_t* u_last_place = csgo.m_localize()->find(last_place);

					if (u_last_place && *u_last_place)
						Drawing::DrawStringUnicode(F::ESPInfo, x + w + 3, y + right++ * size_info, entity->IsDormant() ? Color(255, 170, 170, min(0xC8, dormant_alpha[idx] - 10)) : Color(255, 255, 255, min(0xC8, dormant_alpha[idx] - 10)), FONT_LEFT, u_last_place);
				}
			}

			if (ctx.last_aim_index == entity->entindex())
				Drawing::DrawString(F::ESPInfo, x + w + 3, y + right++ * size_info, Color(255, 255, 255, min(0xC8, dormant_alpha[idx] - 10)), FONT_LEFT, "%i", ctx.last_aim_state);

			if (ctx.m_settings.esp_skeleton)
				skeleton(entity, skeletons_color.alpha(min(skeletons_color.a(), dormant_alpha[idx] - 10)), entity->m_CachedBoneData().Base());

			const auto &weapon = (C_WeaponCSBaseGun*)(csgo.m_entity_list()->GetClientEntityFromHandle(entity->m_hActiveWeapon()));

			if (weapon)
			{
				bool low_ammo = false;

				const auto& weap_info = weapon->GetCSWeaponData();

				if (!weap_info)
					continue;

				int ammo = weapon->m_iClip1();

				if (ammo > weap_info->max_clip)
					ammo = weap_info->max_clip;

				low_ammo = ammo < (weap_info->max_clip / 2);

				if (ctx.m_settings.esp_weapon_ammo) {
					int ammo = weapon->m_iClip1( );

					if ( ammo > weap_info->max_clip )
						ammo = weap_info->max_clip;

					int hp_percent = w - ( int )( ( w * ammo ) / 100 );

					int width = ( w * ( ammo / float( weap_info->max_clip ) ) );

					char ammostr[ 10 ];
					sprintf( ammostr, "%d", ammo );

					const auto text_size = Drawing::GetTextSize( F::ESPInfo, ammostr );

					Drawing::DrawRect( x, y + 2 + h, w, 4, Color( 80, 80, 80, dormant_alpha[ idx ] * 0.49f ) );
					Drawing::DrawOutlinedRect( x, y + 2 + h, w, 4, Color( 10, 10, 10, ( dormant_alpha[ idx ] * 0.8f ) ) );
					Drawing::DrawRect( x + 1, y + 3 + h, width - 2, 2, Color::LightBlue( ).alpha( dormant_alpha[ idx ] ) );

					if ( ammo < ( weap_info->max_clip / 2 ) && ammo > 0 )
						Drawing::DrawString( F::ESPInfo, x + width - 1 - text_size.right, y + h, Color( 255, 255, 255, dormant_alpha[ idx ] - 55.f ), FONT_LEFT, ammostr );

					down++;
				}

				if (ctx.m_settings.esp_weapon) {


					if (ctx.m_settings.esp_weapon_type == 0) {

						const auto& name = weapon_names[weapon->m_iItemDefinitionIndex()];

						if (name != nullptr && wcslen(name) < 1)
						{
							auto st = std::string(weap_info->weapon_name + 7);

							if (weapon->m_iItemDefinitionIndex() == 64)
								st = _r8;

							std::wstring wstr(st.begin(), st.end());

							weapon_names[weapon->m_iItemDefinitionIndex()] = wstr.c_str();
						}

						if (name != nullptr)
						{
							auto wpn_name_size = Drawing::GetTextSize(F::ESPInfo, name);
							Drawing::DrawStringUnicode(F::ESPInfo, x + w / 2 - wpn_name_size.right / 2, y + 1 + h + ( down++ * 6.f ), Color( 255, 255, 255, dormant_alpha[ idx ] - 55.f ), FONT_LEFT, name);
						}

					}
					else
					{
						const auto name = weapon->get_icon();

						if (name.size() > 0)
							Drawing::DrawString(F::Icons, x + w / 2, y + h + 5, Color::White().alpha(min(0xC8, dormant_alpha[idx] - 10)), FONT_CENTER, name.c_str());
					}
				}

				if ((weapon->m_zoomLevel() > 0) && ctx.m_settings.esp_flags[2] && weapon->IsSniper())
					Drawing::DrawString( F::ESPInfo, x + w + 3, y + right++ * size_info, Color( 244, 215, 66, dormant_alpha[ idx ] - 55.f ), FONT_LEFT, "SCOPED" ); // ping color : 77, 137, 234

				if (weapon->IsGrenade() && weapon->m_bPinPulled() && !is_teammate && ctx.m_settings.esp_flags[4])
					Drawing::DrawString(F::ESPInfo, x + w + 3, y + right++ * size_info, Color(230, 0, 0, min(0xC8, dormant_alpha[idx] - 10)), FONT_LEFT, _pin.operator std::string().c_str());

				if (csgo.m_player_resource())
				{
					if (ctx.m_settings.esp_flags[5] && csgo.m_player_resource()->get_c4_carrier() == entity->entindex())
						Drawing::DrawString(F::ESPInfo, x + w + 3, y + right++ * size_info, Color(230, 0, 0, min(0xC8, dormant_alpha[idx] - 10)), FONT_LEFT, _c4.operator std::string().c_str());
					if (ctx.m_settings.esp_flags[5] && csgo.m_player_resource()->get_hostage_carrier() == entity->entindex())
						Drawing::DrawString(F::ESPInfo, x + w + 3, y + right++ * size_info, Color(230, 0, 0, min(0xC8, dormant_alpha[idx] - 10)), FONT_LEFT, _vip.operator std::string().c_str());
				}
			}

			if (!is_teammate && ctx.m_settings.esp_flags[3] && log != nullptr && log->records_count > 0) {

				if (log->tick_records[log->records_count & 63].valid) {
					Drawing::DrawString(F::ESPInfo, x + w + 3, y + right++ * size_info, Color(255, 0, 0, min(0xC8, dormant_alpha[idx] - 10)), FONT_LEFT, sxor("%d"), log->tick_records[log->records_count & 63].simulation_time_delay);

					if (log->tick_records[log->records_count & 63].shot_this_tick)
						Drawing::DrawString(F::ESPInfo, x + w + 3, y + right++ * size_info, Color(255, 255, 255, min(0xC8, dormant_alpha[idx] - 10)), FONT_LEFT, sxor("Shot"));

					if (log->tick_records[log->records_count & 63].animations_updated)
						Drawing::DrawString(F::ESPInfo, x + w + 3, y + right++ * size_info, Color(170, 255, 170, min(0xC8, dormant_alpha[idx] - 10)), FONT_LEFT, sxor("R"));

					if (csgo.m_engine()->IsInGame() && entity->m_flDuckSpeed() >= 4.f && entity->m_flDuckAmount() > 0.f && entity->m_flDuckAmount() < 1 && entity->m_fFlags() & FL_ONGROUND)
					{
						++r_log->duck_ticks;
					}
					else
						r_log->duck_ticks = 0;

				}
			}
		}
		else
		{
			draw_items(entity);
		}
	}

	;
}

void c_visuals::draw_items(C_BasePlayer* m_entity)
{
	int x, y, w, h;
	static ConVar* max_bombtime = csgo.m_engine_cvars()->FindVar("mp_c4timer");
	static c_indicator bomb_indicator("BOMB", max_bombtime->GetFloat());
	static c_indicator defuse_indicator("DEFUSE", 10.f);

	static auto scale_bomb_damage = [](float flDamage, int armor_value) -> float
	{
		const float flArmorRatio = 0.5f;
		const float flArmorBonus = 0.5f;
		if (armor_value > 0) {
			float flNew = flDamage * flArmorRatio;
			float flArmor = (flDamage - flNew) * flArmorBonus;

			if (flArmor > static_cast<float>(armor_value)) {
				flArmor = static_cast<float>(armor_value) * (1.f / flArmorBonus);
				flNew = flDamage - flArmor;
			}

			flDamage = flNew;
		}
		return flDamage;
	};

	if (m_entity == nullptr)
		return;

	const auto& client_class = m_entity->GetClientClass();

	if (client_class == nullptr)
		return;

	Vector screen;
	const Vector origin = m_entity->get_abs_origin();

	ctx.is_local_defusing = false;

	const auto class_id = client_class->m_ClassID;

	if (!Drawing::WorldToScreen(origin, screen) && class_id != class_ids::CPlantedC4)
		return;

	switch (class_id)
	{
	case class_ids::CPlantedC4: //BOMB
	{
		if (!ctx.m_settings.esp_world_bomb || !m_entity->m_bBombTicking())
			return;

		auto bomb_blow_timer = m_entity->get_bomb_blow_timer();
		const auto bomb_defuse_timer = m_entity->get_bomb_defuse_timer();
		const auto can_defuse = (bomb_defuse_timer <= bomb_blow_timer && ctx.m_local()->m_iTeamNum() == 3) || (5.0 > bomb_blow_timer && ctx.m_local()->m_iTeamNum() == 2) || (bomb_defuse_timer <= bomb_blow_timer && ctx.m_local()->m_iTeamNum() != 2 && ctx.m_local()->m_iTeamNum() != 3);
		const auto is_defusing = bomb_defuse_timer > 0.0 && m_entity->m_hBombDefuser();

		if (ctx.m_local()->m_iHealth() > 0)
		{
			const float damagePercentage = 1;

			float flDamage = 500; // 500 - default, if radius is not written on the map https://i.imgur.com/mUSaTHj.png
			auto flBombRadius = flDamage * 3.5f;
			auto flDistanceToLocalPlayer = ((m_entity->get_abs_origin() + m_entity->m_vecViewOffset()) - (ctx.m_local()->get_abs_origin() + ctx.m_local()->m_vecViewOffset())).Length();//c4bomb origin - localplayer origin
			float fSigma = flBombRadius / 3;
			float fGaussianFalloff = exp(-flDistanceToLocalPlayer * flDistanceToLocalPlayer / (2 * fSigma * fSigma));
			auto flAdjustedDamage = flDamage * fGaussianFalloff * damagePercentage;

			flDamage = scale_bomb_damage(flAdjustedDamage, ctx.m_local()->m_ArmorValue());

			float dmg = 100 - (float(ctx.m_local()->m_iHealth()) - flDamage);

			if (dmg > 100)
				dmg = 100;

			if (dmg < 0)
				dmg = 0;

			int Red = dmg * 2.55;
			int Green = 255 - dmg * 2.55;

			if (Red > 255)
				Red = 255;
			if (Green > 255)
				Green = 255;

			if (flDamage > 1) {
				if (flDamage >= ctx.m_local()->m_iHealth()) //- (is_defusing ? 30 : 10)
					Drawing::DrawString( F::LBY, 10, 150, Color( Red, Green, 0, 255 ), FONT_LEFT, "FATAL: -%.1fHP", flDamage );
				else
					Drawing::DrawString( F::LBY, 10, 150, Color( Red, Green, 0, 255 ), FONT_LEFT, "-%.1fHP", flDamage );
			}
		}

		if (bomb_blow_timer < 0)
			bomb_blow_timer = 0;

		if (bomb_blow_timer >= 0)
		{
			int screenW, screenH;
			csgo.m_engine( )->GetScreenSize( screenW, screenH );
			auto ind_h = screenH / 2 + 10;
			auto max_bombtimer = max_bombtime->GetFloat( );
			int indicators = 0;
			auto btime = bomb_blow_timer * ( 100.f / max_bombtimer );

			if ( btime > 100.f )
				btime = 100.f;
			else if ( btime < 0.f )
				btime = 0.f;

			int blow_percent = screenH - ( int )( ( 98 * btime ) / 100 );
			int b_red = 255 - ( btime * 2.55 );
			int b_green = btime * 2.55;

			Drawing::DrawRect( 10, ind_h + 15 + 20 * indicators, 100, 15, { 0, 0, 0, 130 } );
			Drawing::DrawString( F::ESPInfo, 60, ind_h + 15 + 20 * indicators, can_defuse ? Color::Green( 240 ) : Color::Red( 240 ), FONT_CENTER, "BOMB" );

			Drawing::DrawRect( 11,
				ind_h + 25 + 20 * indicators++,
				( 98.f * float( bomb_blow_timer / max_bombtimer ) ),
				4,
				{ b_red,b_green ,0,130 } );

			if ( Vector dummy = Vector::Zero; Drawing::WorldToScreen( m_entity->m_vecOrigin( ), dummy ) && !dummy.IsZero( ) )
				Drawing::DrawString( F::ESPInfo, dummy.x, dummy.y, Color::Green( 240 ), FONT_CENTER, "BOMB: %.1f", bomb_blow_timer );
	
		}
		break;
	}
	case class_ids::CInferno:
	{
		const auto owner = m_entity->m_hOwnerEntity();

		const auto& eowner = csgo.m_entity_list()->GetClientEntityFromHandle(owner);

		if (!ctx.m_settings.esp_world_nades || !get_espbox(m_entity, x, y, w, h) || !eowner || !ctx.m_local() || (eowner->m_iTeamNum() == ctx.m_local()->m_iTeamNum() && eowner != ctx.m_local()) && *(float*)(uintptr_t(m_entity) + 0x20) > 0) return;

		const double spawn_time = *(float*)(uintptr_t(m_entity) + 0x20);
		const double factor = ((spawn_time + 7.031) - csgo.m_globals()->curtime) / 7.031;

		if (factor <= 0)
			break;

		const int red = max(min(255 * factor, 255), 0);
		const int green = max(min(255 * (1.0 - factor), 255), 0);

		static auto text_size = Drawing::GetTextSize(F::ESPInfo, sxor("MOLOTOV"));
		Drawing::DrawRect(x - 49, y + 10, 98.f, 4, { 80, 80, 80, 125 });
		Drawing::DrawRect(x - 49, y + 10, 98.f * factor, 4, { red, green, 0, 250 });
		Drawing::DrawString(F::ESPInfo, x, y, { 255, 255, 255, 250 }, FONT_CENTER, sxor("MOLOTOV"));

		Drawing::DrawString(F::ESPInfo, x - 49.0 + 98.0 * factor, y + 8.0, { 255, 255, 255, 250 }, FONT_CENTER, "%.0f", (spawn_time + 7.031) - csgo.m_globals()->curtime);

		break;
	}
	case class_ids::CSmokeGrenadeProjectile:
	{
		const auto owner = m_entity->m_hOwnerEntity();

		const auto& eowner = csgo.m_entity_list()->GetClientEntityFromHandle(owner);

		if (!ctx.m_settings.esp_world_nades || !get_espbox(m_entity, x, y, w, h) || !eowner || !ctx.m_local()) return;

		const float spawn_time = TICKS_TO_TIME(m_entity->m_nSmokeEffectTickBegin());
		const double factor = ((spawn_time + 18.041) - csgo.m_globals()->curtime) / 18.041;

		if (factor <= 0)
			break;

		const int red = max(min(255 * factor, 255), 0);
		const int green = max(min(255 * (1.0 - factor), 255), 0);

		static auto text_size = Drawing::GetTextSize(F::ESPInfo, sxor("SMOKE"));
		if (spawn_time > 0.f) {
			Drawing::DrawRect(x - 49, y + 10, 98, 4, { 80, 80, 80, 125 });
			Drawing::DrawRect(x - 49, y + 10, 98 * factor, 4, { red, green, 0, 245 });
			Drawing::DrawString(F::ESPInfo, x - 49 + 98 * factor, y + 8, { 255, 255, 255, 250 }, FONT_CENTER, "%.0f", (spawn_time + 18.04125) - csgo.m_globals()->curtime);
		}
		Drawing::DrawString(F::ESPInfo, x, y, { 255, 255, 255, 250 }, FONT_CENTER, sxor("smoke"));

		break;
	}
	case class_ids::CC4:
	{
		const auto owner = m_entity->m_hOwnerEntity();

		if (!ctx.m_settings.esp_world_bomb || !get_espbox(m_entity, x, y, w, h) || owner != -1) return;

		Drawing::DrawString(F::ESPInfo, x + w / 2, y + h / 2, Color(150, 200, 60, 0xb4), FONT_CENTER, sxor("C4"));

		break;
	}
	default:
	{
		const auto owner = m_entity->m_hOwnerEntity();

		if (ctx.m_settings.esp_world_weapons)
		{
			if (owner == -1)
			{
				auto* const weapon = reinterpret_cast<C_WeaponCSBaseGun*>(m_entity);

				if (!weapon || !weapon->is_weapon())
					return;

				const auto& name = weapon_names[weapon->m_iItemDefinitionIndex()];

				auto* const wdata = weapon->GetCSWeaponData();

				if (name != nullptr && wcslen(name) < 1 && wdata)
				{
					auto st = std::string(wdata->weapon_name + 7);

					if (weapon->m_iItemDefinitionIndex() == 64)
						st = sxor("revolver");

					std::wstring wstr(st.begin(), st.end());

					weapon_names[weapon->m_iItemDefinitionIndex()] = wstr.c_str();
				}

				auto dist = ctx.m_local()->get_abs_origin().DistanceSquared(m_entity->m_vecOrigin()) * 0.01905f;

				if (ctx.m_local()->IsDead())
					dist = csgo.m_input()->m_vecCameraOffset.DistanceSquared(m_entity->m_vecOrigin()) * 0.01905f;

				const auto cl_dist = Math::clamp(dist - 500.f, 0, 510);
				const auto alpha = min(0xb4, 255 - (cl_dist / 2));

				if (alpha <= 0 || name == nullptr || wcslen(name) <= 1 || !get_espbox(m_entity, x, y, w, h)) return;

				const auto text_size = Drawing::GetTextSize(F::ESPInfo, name);

				if (ctx.m_settings.esp_world_weapons)
					Drawing::DrawStringUnicode(F::ESPInfo, x + w / 2 - text_size.right / 2, y + h / 2.0f, ctx.m_settings.world_esp_color.alpha(min(ctx.m_settings.world_esp_color.a(), alpha)), false, name);

				if (ctx.m_settings.esp_world_weapons && !weapon->IsGrenade() && weapon->GetCSWeaponData() != nullptr)
				{
					auto clip = weapon->m_iClip1();
					auto maxclip = weapon->GetCSWeaponData()->max_clip;
					clip = std::clamp(clip, 0, 1000);
					maxclip = std::clamp(maxclip, 1, 1000);

					const auto nx = x + w / 2 - text_size.right / 2;

					w = text_size.right;

					const auto width = Math::clamp(w * clip / maxclip, 0, w);

					Drawing::DrawRect(nx, (y + h / 2.f) + text_size.bottom, w, 3, Color(80, 80, 80, alpha * 0.49));
					Drawing::DrawOutlinedRect(nx - 1,  y + h / 2.f - 1 + text_size.bottom, w + 2, 4, Color(10, 10, 10, (alpha * 0.5)));
					Drawing::DrawRect(nx,  (y + h / 2.f)  + text_size.bottom, width, 2, ctx.m_settings.world_esp_color.alpha(min(ctx.m_settings.world_esp_color.a(), alpha)));
				}
			}
		}

		if (ctx.m_settings.esp_world_nades)
		{

			if (class_id == class_ids::CMolotovProjectile
				|| class_id == class_ids::CDecoyProjectile
				|| class_id == class_ids::CSmokeGrenadeProjectile
				|| class_id == class_ids::CSnowballProjectile
				|| class_id == class_ids::CBreachChargeProjectile
				|| class_id == class_ids::CBumpMineProjectile
				|| class_id == class_ids::CBaseCSGrenadeProjectile
				|| class_id == class_ids::CSensorGrenadeProjectile)
			{

				if (!get_espbox(m_entity, x, y, w, h)) return;

				// draw decoy.
				if (class_id == class_ids::CDecoyProjectile)
					Drawing::DrawString(F::ESPInfo, x + w / 2, y + h, Color(255, 255, 255, 0xb4), FONT_CENTER, sxor("DECOY"));
				else if (class_id == class_ids::CBaseCSGrenadeProjectile) {
					auto model = m_entity->GetModel();

					if (model) {
						std::string name{ model->szName };

						if (name.find(sxor("flashbang")) != std::string::npos)
							Drawing::DrawString(F::ESPInfo, x + w / 2, y + h, Color::LightBlue(0xb4), FONT_CENTER, sxor("FLASH"));
						else if (name.find(sxor("fraggrenade")) != std::string::npos && m_entity->m_nExplodeEffectTickBegin() < 1)
							Drawing::DrawString(F::ESPInfo, x + w / 2, y + h, Color(255, /*10*/0, /*10*/0, 0xb4), FONT_CENTER, sxor("GRENADE"));
					}
				}
				else if (class_id == class_ids::CMolotovProjectile)
					Drawing::DrawString(F::ESPInfo, x + w / 2, y + h, Color(255, 130, 0, 0xb4), FONT_CENTER, sxor("MOLOTOV"));
			}
			break;
		}
	}
	}
}