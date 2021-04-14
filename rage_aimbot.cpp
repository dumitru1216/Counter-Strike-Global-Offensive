#include "rage_aimbot.hpp"
#include "source.hpp"
#include "entity.hpp"
#include "player.hpp"
#include "weapon.hpp"
#include "hooked.hpp"
#include "math.hpp"
#include "displacement.hpp"
#include "lag_comp.hpp"
#include "anti_aimbot.hpp"
#include "resolver.hpp"
#include "visuals.hpp"
#include "menu.hpp"
#include "movement.hpp"
#include "prediction.hpp"
#include "autowall.hpp"
#include "misc.hpp"
#include "usercmd.hpp"

static constexpr auto total_seeds = 128;
static constexpr auto autowall_traces = 48;

//std::vector<TargetListing_t> m_entities;

void c_aimbot::get_hitbox_data(C_Hitbox* rtn, C_BasePlayer* ent, int ihitbox, matrix3x4_t* matrix)
{
	if (ihitbox < 0 || ihitbox > 19) return;

	if (!ent) return;

	const model_t* const model = ent->GetModel();

	if (!model)
		return;

	studiohdr_t* const pStudioHdr = csgo.m_model_info()->GetStudioModel(model);

	if (!pStudioHdr)
		return;

	mstudiobbox_t* const hitbox = pStudioHdr->pHitbox(ihitbox, ent->m_nHitboxSet());

	if (!hitbox)
		return;

	const auto is_capsule = hitbox->radius != -1.f;

	Vector min, max;
	if (is_capsule) {
		Math::VectorTransform(hitbox->bbmin, matrix[hitbox->bone], min);
		Math::VectorTransform(hitbox->bbmax, matrix[hitbox->bone], max);
	}
	else
	{
		min = Math::VectorRotate(hitbox->bbmin, hitbox->rotation);
		max = Math::VectorRotate(hitbox->bbmax, hitbox->rotation);
		Math::VectorTransform(min, matrix[hitbox->bone], min);
		Math::VectorTransform(max, matrix[hitbox->bone], max);
	}

	rtn->hitboxID = ihitbox;
	rtn->isOBB = !is_capsule;
	rtn->radius = hitbox->radius;
	rtn->mins = min;
	rtn->maxs = max;
	rtn->hitgroup = hitbox->group;
	rtn->hitbox = hitbox;
	Math::VectorITransform(ctx.m_eye_position, matrix[hitbox->bone], rtn->start_scaled);
	rtn->bone = hitbox->bone;
}

Vector c_aimbot::get_hitbox(C_BasePlayer* ent, int ihitbox, matrix3x4_t mat[])
{
	if (ihitbox < 0 || ihitbox > 19) return Vector::Zero;

	if (!ent) return Vector::Zero;

	if (!ent->GetClientRenderable())
		return Vector::Zero;

	const model_t* const model = ent->GetModel();

	if (!model)
		return Vector::Zero;

	studiohdr_t* const pStudioHdr = csgo.m_model_info()->GetStudioModel(model);

	if (!pStudioHdr)
		return Vector::Zero;

	mstudiobbox_t* const hitbox = pStudioHdr->pHitbox(ihitbox, ent->m_nHitboxSet());

	if (!hitbox)
		return Vector::Zero;

	if (hitbox->bone > 128 || hitbox->bone < 0)
		return Vector::Zero;

	Vector min, max;
	Vector top_point;
	constexpr float rotation = 0.70710678f;
	Math::VectorTransform(hitbox->bbmin, mat[hitbox->bone], min);
	Math::VectorTransform(hitbox->bbmin, mat[hitbox->bone], max);

	auto center = (min + max) / 2.f;

	return center;
}

int low_count = 0;
int medium_count = 0;

void c_aimbot::build_seed_table()
{
	if (seeds_filled >= total_seeds)
		return;

	seeds_filled = 0;

	for (auto i = 0; i < total_seeds; i++) {
		if (seeds_filled >= 128)
			break;

		RandomSeed(seeds[i]);

		float a = RandomFloat(0.0f, 6.2831855f);
		float c = RandomFloat(0.0f, 1.0f);
		float b = RandomFloat(0.0f, 6.2831855f);

		precomputed_seeds[seeds_filled++] = std::tuple<float, float, float, float, float>(c,
			sin(a), cos(b), sin(b), cos(a));
	}
}

int mini_low_count = 0;

void c_aimbot::build_mini_hc_table()
{
	for (auto i = 0; i < 64; i++) 
	{
		RandomSeed(i);

		float a = RandomFloat(0.0f, 6.2831855f);
		float c = RandomFloat(0.0f, 1.0f);
		float b = RandomFloat(0.0f, 6.2831855f);

		precomputed_mini_seeds[i] = std::tuple<float, float, float, float, float>(c,
			sin(a), cos(b), sin(b), cos(a));
	}
}

bool c_aimbot::mini_hit_chance(Vector vhitbox, C_BasePlayer* ent, int hitbox, int& hc)
{
	build_mini_hc_table();

	if (!ctx.latest_weapon_data)
		return false;
	
	C_Hitbox ht;
	get_hitbox_data(&ht, ent, hitbox, ent->m_CachedBoneData().Base());

	auto traces_hit = 0;
	auto const ang = Math::CalcAngle(ctx.m_eye_position, vhitbox);

	Vector forward, right, up;
	Math::AngleVectors(ang, &forward, &right, &up);

	m_weapon()->UpdateAccuracyPenalty();
	const float weap_inaccuracy = m_weapon()->GetInaccuracy();

	if (int(weap_inaccuracy * 1000.f) == 0) {
		hc = 100;
		return true;
	}

	const auto weap_spread = m_weapon()->GetSpread();

	// performance optimization.
	if ((ctx.m_eye_position - ent->m_vecOrigin()).Length() >= (ctx.latest_weapon_data->range * 1.01f))
		return false;

	if (precomputed_mini_seeds.empty())
		return false;

	static std::tuple<float, float, float, float, float>* seed;
	static float c, spread_val, inaccuracy_val;
	static Vector v_spread, dir;
	
	Ray_t ray;

	for (int i = 0; i < 64; i++)
	{
		// get seed.
		seed = (&precomputed_mini_seeds[i]);


		c = std::get<0>(*seed);

		spread_val = c * weap_spread;
		inaccuracy_val = c * weap_inaccuracy;

		v_spread.Set((std::get<2>(*seed) * spread_val) + (std::get<4>(*seed) * inaccuracy_val), (std::get<3>(*seed) * spread_val) + (std::get<1>(*seed) * inaccuracy_val), 0);

		dir.Set(forward.x + (v_spread.x * right.x) + (v_spread.y * up.x),
		forward.y + (v_spread.x * right.y) + (v_spread.y * up.y),
		forward.z + (v_spread.x * right.z) + (v_spread.y * up.z));
		dir.NormalizeInPlace();
		//proper
		auto const end = ctx.m_eye_position + (dir * 8192.f);

		trace_t tr;
		ray.Init(ctx.m_eye_position, end);

		tr.fraction = 1.0;
		tr.startsolid = false;

		auto intersect = Math::ClipRayToHitbox(ray, ht.hitbox, ent->m_CachedBoneData().Base()[ht.bone], tr) >= 0;

		if (intersect)
			++traces_hit;
	}

	hc = traces_hit * 1.5625f;

	return true;
}

int c_aimbot::hitbox2hitgroup(C_BasePlayer* m_player, int ihitbox)
{
	if (ihitbox < 0 || ihitbox > 19) return 0;

	if (!m_player) return 0;

	const model_t* const model = m_player->GetModel();

	if (!model)
		return 0;

	studiohdr_t* const pStudioHdr = csgo.m_model_info()->GetStudioModel(model);

	if (!pStudioHdr)
		return 0;

	mstudiobbox_t* const hitbox = pStudioHdr->pHitbox(ihitbox, m_player->m_nHitboxSet());

	if (!hitbox)
		return 0;

	return hitbox->group;
}

int c_aimbot::safe_point(C_BasePlayer* entity, Vector eye_pos, Vector aim_point, int hitboxIdx, C_Tickrecord* record)
{
	resolver_records* resolve_info = &feature::resolver->player_records[entity->entindex() - 1];
	c_player_records* log = &feature::lagcomp->records[entity->entindex() - 1];

	if (!record || !record->data_filled || !record->valid)
		return 0;

	const auto is_colliding = [entity, hitboxIdx](Vector start, Vector end, C_Hitbox* hbox_data, matrix3x4_t *mx) -> bool
	{
		if (hbox_data->isOBB)
		{ 
			auto dir = end - start;
			dir.NormalizeInPlace();
			Vector delta;
			Math::VectorIRotate((dir * 8192.f), mx[hbox_data->bone], delta);

			return Math::IntersectBB(hbox_data->start_scaled, delta, hbox_data->mins, hbox_data->maxs);
		}
		else
		{
			if (Math::Intersect(ctx.m_eye_position, end, hbox_data->mins, hbox_data->maxs, hbox_data->radius))
				return true;
		}

		return false;
	};

	auto forward = aim_point - eye_pos;
	auto end = eye_pos + (forward * 8192.f);

	C_Hitbox box1; get_hitbox_data(&box1, entity, hitboxIdx, record->leftmatrixes);
	C_Hitbox box2; get_hitbox_data(&box2, entity, hitboxIdx, record->rightmatrixes);
	C_Hitbox box3; get_hitbox_data(&box3, entity, hitboxIdx, record->matrixes);

	int hits = 0;

	if (is_colliding(eye_pos, end, &box1, record->leftmatrixes)) ++hits;

	if (is_colliding(eye_pos, end, &box2, record->rightmatrixes)) ++hits;

	if (is_colliding(eye_pos, end, &box3, record->matrixes)) ++hits;

	return hits;
}

bool c_aimbot::safe_side_point(C_BasePlayer* entity, Vector eye_pos, Vector aim_point, int hitboxIdx, C_Tickrecord* record)
{
	resolver_records* resolve_info = &feature::resolver->player_records[entity->entindex() - 1];
	c_player_records* log = &feature::lagcomp->records[entity->entindex() - 1];

	if (!record || !record->data_filled || !record->valid)
		return false;

	const auto angle = Math::CalcAngle(eye_pos, aim_point);
	Vector forward;
	Math::AngleVectors(angle, &forward);
	auto const end(eye_pos + forward * 8192.f)/*(eye_pos.DistanceSquared(aim_point) * 1.01f)*/;

	C_Hitbox box1; get_hitbox_data(&box1, entity, hitboxIdx, record->leftmatrixes);
	C_Hitbox box2; get_hitbox_data(&box2, entity, hitboxIdx, record->rightmatrixes);
	C_Hitbox box3; get_hitbox_data(&box3, entity, hitboxIdx, record->matrixes);

	auto hits = 0;

	//bool ok = false;
	if (box2.isOBB)
	{
		Vector delta1;
		Math::VectorIRotate((forward * 8192.f), record->leftmatrixes[box1.bone], delta1);

		Vector delta2;
		Math::VectorIRotate((forward * 8192.f), record->rightmatrixes[box2.bone], delta2);

		Vector delta3;
		Math::VectorIRotate((forward * 8192.f), record->matrixes[box3.bone], delta3);

		if (Math::IntersectBB(box1.start_scaled, delta1, box1.mins, box1.maxs))
			++hits;
		if (Math::IntersectBB(box2.start_scaled, delta2, box2.mins, box2.maxs))
			++hits;
		if (Math::IntersectBB(box3.start_scaled, delta3, box3.mins, box3.maxs))
			++hits;
	}
	else
	{
		if (Math::Intersect(eye_pos, end, box1.mins, box1.maxs, box1.radius))
			++hits;
		if (Math::Intersect(eye_pos, end, box2.mins, box2.maxs, box2.radius))
			++hits;
		if (Math::Intersect(eye_pos, end, box3.mins, box3.maxs, box3.radius))
			++hits;
	}

	return (hits >= 2);
}

bool c_aimbot::hit_chance(QAngle angle, Vector point, C_BasePlayer* ent, float chance, int hitbox, float damage, float* hc)
{
	if (chance < 1.f)
		return true;

	if (ctx.latest_weapon_data == nullptr || !m_weapon())
		return false;

	build_seed_table();

	C_Hitbox ht;
	get_hitbox_data(&ht, ent, hitbox, ent->m_CachedBoneData().Base());

	int traces_hit = 0;
	int awalls_hit = 0;
	int awall_traces_done = 0;

	static Vector forward, right, up;

	// performance optimization.
	if ((ctx.m_eye_position - ent->m_vecOrigin()).Length() > (ctx.latest_weapon_data->range * 1.02f))
		return false;

	Math::AngleVectors(angle, &forward, &right, &up);
	const float& weap_inaccuracy = Engine::Prediction::Instance()->GetInaccuracy();

	if (int(weap_inaccuracy * 10000.f) == 0) {
		hc[0] = 13;
		hc[1] = 37;
		return true;
	}

	const auto weap_spread = Engine::Prediction::Instance()->GetSpread();
	auto is_special_weapon = m_weapon()->m_iItemDefinitionIndex() == 9
		|| m_weapon()->m_iItemDefinitionIndex() == 11
		|| m_weapon()->m_iItemDefinitionIndex() == 38
		|| m_weapon()->m_iItemDefinitionIndex() == 40;

	if (precomputed_seeds.empty())
		return false;

	static std::tuple<float, float, float, float, float>* seed;
	static float c, spread_val, inaccuracy_val;
	static Vector v_spread, dir, end;
	Ray_t ray;
	float average_spread = 0;

	//std::deque<Vector> hit_rays;
	for (auto i = 0; i < total_seeds; i++)
	{
		// get seed.
		seed = &precomputed_seeds[i];

		c = std::get<0>(*seed);

		spread_val = c * weap_spread;
		inaccuracy_val = c * weap_inaccuracy;

		v_spread = Vector((std::get<2>(*seed) * spread_val) + (std::get<4>(*seed) * inaccuracy_val), (std::get<3>(*seed) * spread_val) + (std::get<1>(*seed) * inaccuracy_val), 0);

		dir.x = forward.x + (v_spread.x * right.x) + (v_spread.y * up.x);
		dir.y = forward.y + (v_spread.x * right.y) + (v_spread.y * up.y);
		dir.z = forward.z + (v_spread.x * right.z) + (v_spread.y * up.z);

		dir.NormalizeInPlace();
		//proper
		end = ctx.m_eye_position + (dir * 8192.f);
		bool intersect = false;

		if (ht.isOBB) 
		{
			Vector delta;
			Math::VectorIRotate((dir * 8192.f), ent->m_CachedBoneData().Base()[ht.bone], delta);

			intersect = Math::IntersectBB(ht.start_scaled, delta, ht.mins, ht.maxs);
		}
		else
		{
			intersect = Math::Intersect(ctx.m_eye_position, end, ht.mins, ht.maxs, ht.radius);
		}

		if (intersect)
		{
			++traces_hit;

			if (m_weapon()->m_iItemDefinitionIndex() != 64 && !(is_special_weapon && !ctx.m_local()->m_bIsScoped())) {

				average_spread += inaccuracy_val;

				if (autowall_traces > awall_traces_done && (i < 5 || (average_spread / i) <= inaccuracy_val))
				{
					auto dmg = feature::autowall->CanHit(ctx.m_eye_position, end, ctx.m_local(), ent, hitbox);
					const auto dmg_fine = dmg >= damage;

					if (dmg_fine) {
						++awalls_hit;
					}

					++awall_traces_done;
				}
			}
			else
			{
				if (autowall_traces > awalls_hit)
					++awalls_hit;
			}
		}
	}

	const auto trace_chance = ((float(traces_hit) / float(total_seeds)) * 100.f);

	if (trace_chance >= 15.f && ctx.m_local()->m_fFlags() & FL_ONGROUND && is_special_weapon && !ctx.m_local()->m_bIsScoped() && m_weapon()->can_shoot()) {
		if (Engine::Prediction::Instance()->m_flCalculatedInaccuracy >= (m_weapon()->m_flAccuracyPenalty() * 0.02f))
			return true;
	}

	if (trace_chance < min(100.f, (chance + 5.f)))
		return false;

	const auto final_chance = (float(awalls_hit) / float(autowall_traces)) * 100.f;

	if (chance <= final_chance) {
		hc[0] = final_chance;
		hc[1] = trace_chance;
		return true;
	}
	
	return false;
}


void c_aimbot::visualize_hitboxes(C_BasePlayer* entity, matrix3x4_t* mx, Color color, float time)
{
	const model_t* model = entity->GetModel();

	if (!model)
		return;

	const studiohdr_t* studioHdr = csgo.m_model_info()->GetStudioModel(model);

	if (!studioHdr)
		return;

	const mstudiohitboxset_t* set = studioHdr->pHitboxSet(entity->m_nHitboxSet());

	if (!set)
		return;

	for (int i = 0; i < set->numhitboxes; i++)
	{
		mstudiobbox_t* hitbox = set->pHitbox(i);

		if (!hitbox)
			continue;

		Vector min, max/*, center*/;
		Math::VectorTransform(hitbox->bbmin, mx[hitbox->bone], min);
		Math::VectorTransform(hitbox->bbmax, mx[hitbox->bone], max);

		if (hitbox->radius != -1)
			csgo.m_debug_overlay()->AddCapsuleOverlay(min, max, hitbox->radius, color.r(), color.g(), color.b(), color.a(), time, 0, 1);

	}
}

void c_aimbot::autostop(CUserCmd* cmd, C_WeaponCSBaseGun* local_weapon)
{
	static auto accel = csgo.m_engine_cvars()->FindVar(sxor("sv_accelerate"));

	static bool was_onground = ctx.m_local()->m_fFlags() & FL_ONGROUND;

	if (ctx.m_settings.autostop_only_when_shooting && (!local_weapon->can_shoot() || m_weapon()->m_iItemDefinitionIndex() == 64)) {
		ctx.did_stop_before = false;
		ctx.do_autostop = false;
		return;
	}

	Engine::Prediction::Instance()->m_autostop_velocity_to_validate = 0.f;

	if (ctx.m_settings.aimbot_autostop && local_weapon && local_weapon->m_iItemDefinitionIndex() != WEAPON_TASER && ctx.m_local()->m_fFlags() & FL_ONGROUND && was_onground && ctx.latest_weapon_data/* && !(cmd->buttons & IN_JUMP)*/)
	{
		auto v10 = cmd->buttons & ~(IN_MOVERIGHT | IN_MOVELEFT | IN_BACK | IN_FORWARD | IN_JUMP | IN_SPEED);
		cmd->buttons = v10;

		const auto chocked_ticks = (cmd->command_number % 3) == 0 ? (14 - csgo.m_client_state()->m_iChockedCommands) : ((14 - csgo.m_client_state()->m_iChockedCommands) / 2);
		const auto max_speed = (local_weapon->GetMaxSpeed() * 0.33f) - 1.f - (float(chocked_ticks) * (m_weapon()->m_iItemDefinitionIndex() == WEAPON_AWP ? 1.35f : 1.65f) * + ctx.m_local()->m_iShotsFired()/* * 1.2f*/);//, (0.32f - float(0.005f * chocked_ticks)));

		auto velocity = ctx.m_local()->m_vecVelocity();
		velocity.z = 0;
		auto current_speed = ((velocity.x * velocity.x) + (velocity.y * velocity.y));
		current_speed = sqrtf(current_speed);

		const auto cmd_speed = sqrtf((cmd->sidemove * cmd->sidemove) + (cmd->forwardmove * cmd->forwardmove));

		auto new_sidemove = cmd->sidemove;
		auto new_forwardmove = cmd->forwardmove;

		if (current_speed >= 28.f) {
			if (current_speed <= max_speed && ctx.m_settings.autostop_type == 0)
			{
				if (current_speed > 0.0f)
				{
					//cmd->buttons |= IN_SPEED;
					if (current_speed <= 0.1f)
					{
						new_sidemove = cmd->sidemove * fminf(current_speed, max_speed);
						new_forwardmove = cmd->forwardmove * fminf(current_speed, max_speed);
					}
					else
					{
						new_forwardmove = (cmd->forwardmove / cmd_speed) * fminf(current_speed, max_speed);
						new_sidemove = (cmd->sidemove / cmd_speed) * fminf(current_speed, max_speed);
					}
				}
			}
			else
			{
				QAngle angle;
				Math::VectorAngles(velocity, angle);

				// fix direction by factoring in where we are looking.
				angle.y = ctx.cmd_original_angles.y - angle.y;

				// convert corrected angle back to a direction.
				Vector direction;
				Math::AngleVectors(angle, &direction);

				if (current_speed > 5.f) {
					auto stop = direction * -current_speed;

					new_forwardmove = stop.x;
					new_sidemove = stop.y;
				}
				else
				{
					new_forwardmove = 0;
					new_sidemove = 0;
				}
			}
		}

		if (ctx.m_local()->m_bDucking()
			|| ctx.m_local()->m_fFlags() & FL_DUCKING) {
			new_forwardmove = new_forwardmove / (((ctx.m_local()->m_flDuckAmount() * 0.34f) + 1.0f) - ctx.m_local()->m_flDuckAmount());
			new_sidemove = new_sidemove / (((ctx.m_local()->m_flDuckAmount() * 0.34f) + 1.0f) - ctx.m_local()->m_flDuckAmount());
		}

		cmd->sidemove = Math::clamp(new_sidemove, -450.f, 450.f);
		cmd->forwardmove = Math::clamp(new_forwardmove, -450.f, 450.f);

		ctx.did_stop_before = true;
		ctx.last_autostop_tick = cmd->command_number;

		ctx.do_autostop = false;
	}

	was_onground = (ctx.m_local()->m_fFlags() & FL_ONGROUND);
}

std::string hitbox_to_string(int h)
{
	switch (h)
	{
	case 0:
		return "head";
		break;
	case 1:
		return "neck";
		break;
	case HITBOX_RIGHT_FOOT:
	case HITBOX_RIGHT_CALF:
	case HITBOX_RIGHT_THIGH:
		return "right leg";
		break;
	case HITBOX_LEFT_FOOT:
	case HITBOX_LEFT_CALF:
	case HITBOX_LEFT_THIGH:
		return "left leg";
		break;
	case HITBOX_RIGHT_HAND:
	case HITBOX_RIGHT_UPPER_ARM:
	case HITBOX_RIGHT_FOREARM:
		return "right hand";
		break;
	case HITBOX_LEFT_HAND:
	case HITBOX_LEFT_FOREARM:
	case HITBOX_LEFT_UPPER_ARM:
		return "left hand";
		break;
	case HITBOX_CHEST:
		return "lower chest";
	case HITBOX_UPPER_CHEST:
		return "upper chest";
		break;
	default:
		return "body";
		break;
	}
}

Vector get_bone(int bone, matrix3x4_t mx[])
{
	return Vector(mx[bone][0][3], mx[bone][1][3], mx[bone][2][3]);
}

TargetListing_t::TargetListing_t(C_BasePlayer* ent)
{
	entity = ent;

	hp = min(100, entity->m_iHealth());
	idx = entity->entindex();
}

void c_aimbot::OnRoundStart(C_BasePlayer* player) {
	m_target = nullptr;
	m_hitboxes.clear();

	// IMPORTANT: DO NOT CLEAR LAST HIT SHIT.
}

void c_aimbot::SetupHitboxes(C_BasePlayer* ent, C_Tickrecord* record, bool history) {

	// reset hitboxes.
	m_hitboxes.clear();

	if (!record)
		return;

	if (m_weapon()->m_iItemDefinitionIndex() == WEAPON_TASER) {
		// hitboxes for the zeus.
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_CHEST, HitscanMode::NORMAL });
		return;
	}

	// prefer, always.
	if (ctx.m_settings.aimbot_prefer_body)
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER});

	auto can_baim = ctx.m_settings.aimbot_hitboxes[3];

	// prefer, lethal.
	if (can_baim)
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::LETHAL});

	// prefer, lethal x2.
	if (can_baim && ctx.exploit_allowed && ctx.has_exploit_toggled && ctx.main_exploit >= 2 && (ctx.ticks_allowed > 12 || ctx.force_aimbot > 0) && !ctx.fakeducking)
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::LETHAL2});

	for (auto h = 0; h < 6; h++) {
		// head.
		if (ctx.m_settings.aimbot_hitboxes[0] && h == 0)
			m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::NORMAL});

		// stomach.
		if (ctx.m_settings.aimbot_hitboxes[3] && h == 3)
			m_hitboxes.push_back({ HITBOX_BODY,   HitscanMode::NORMAL });

		// chest.
		if (ctx.m_settings.aimbot_hitboxes[1] && h == 1)
			m_hitboxes.push_back({ HITBOX_CHEST,       HitscanMode::NORMAL });

		if (ctx.m_settings.aimbot_hitboxes[2] && h == 2)
			m_hitboxes.push_back({ HITBOX_UPPER_CHEST, HitscanMode::NORMAL });

		if (ctx.m_settings.aimbot_hitboxes[5] && h == 5) {
			m_hitboxes.push_back({ HITBOX_LEFT_FOOT,  HitscanMode::NORMAL });
			m_hitboxes.push_back({ HITBOX_RIGHT_FOOT,  HitscanMode::NORMAL });
		}

		if (Engine::Prediction::Instance()->m_flFrameTime < csgo.m_globals()->interval_per_tick) {

			if (ctx.m_settings.aimbot_hitboxes[1] && h == 1)
				m_hitboxes.push_back({ HITBOX_THORAX,      HitscanMode::NORMAL });

			// arms.
			if (ctx.m_settings.aimbot_hitboxes[4] && h == 4) {
				m_hitboxes.push_back({ HITBOX_LEFT_UPPER_ARM, HitscanMode::NORMAL });
				m_hitboxes.push_back({ HITBOX_RIGHT_UPPER_ARM, HitscanMode::NORMAL });
			}

			// legs.
			if (ctx.m_settings.aimbot_hitboxes[5] && h == 5) {
				m_hitboxes.push_back({ HITBOX_LEFT_THIGH, HitscanMode::NORMAL });
				m_hitboxes.push_back({ HITBOX_RIGHT_THIGH, HitscanMode::NORMAL });
				m_hitboxes.push_back({ HITBOX_LEFT_CALF,  HitscanMode::NORMAL });
				m_hitboxes.push_back({ HITBOX_RIGHT_CALF,  HitscanMode::NORMAL });
			}
		}
		else
			continue;
	}
}

void c_aimbot::init() {
	// clear old targets.
	m_targets[0].entity = nullptr;
	m_targets_count = 0;

	m_target = nullptr;
	m_aim = Vector{};
	m_angle = QAngle{};
	m_damage = 0.f;
	//ctx.do_autostop = false;

	m_baim_key = false;
	m_damage_key = false;

	m_best_dist = FLT_MAX;
	m_best_fov = 180.f + 1.f;
	m_best_damage = 0.f;
	m_best_hp = 100 + 1;
	m_best_lag = FLT_MAX;
	m_best_height = FLT_MAX;
}

void c_aimbot::StripAttack(CUserCmd* cmd) {

}

bool c_aimbot::think(CUserCmd* cmd, bool* send_packet) {
	// do all startup routines.
	init();

	m_baim_key = ctx.get_key_press(ctx.m_settings.aimbot_bodyaim_key);

	if (m_baim_key)
		ctx.active_keybinds[5].mode = ctx.m_settings.aimbot_bodyaim_key.mode + 1;

	m_damage_key = ctx.get_key_press(ctx.m_settings.aimbot_min_damage_override);

	if (m_damage_key)
		ctx.active_keybinds[6].mode = ctx.m_settings.aimbot_min_damage_override.mode + 1;

	
	// we have no aimbot enabled.
	if (!ctx.m_settings.aimbot_enabled)
		return false;
	
	// sanity.
	if (!m_weapon() || cmd->weaponselect > 0) {
		ctx.m_last_shot_index = -1;
		return false;
	}

	if (abs(cmd->command_number - ctx.m_ragebot_shot_nr) > 40)
		ctx.m_last_shot_index = -1;

	// no grenades or bomb.
	if (!m_weapon()->IsGun() || ctx.air_stuck) {
		ctx.m_last_shot_index = -1;
		return false;
	}

	// no point in aimbotting if we cannot fire this tick.
	if (/*!m_weapon()->can_shoot() && */(ctx.fakeducking || ctx.m_settings.fake_lag_between_shots) && feature::anti_aim->did_shot_in_chocked_cycle)
		return false;

	if (!m_weapon()->can_shoot())
	{
		if (m_weapon()->m_reloadState() != 0) {
			return false;
		}

		ctx.shot_angles.clear();
	}

	// run knifebot.
	if (m_weapon()->is_knife() && m_weapon()->m_iItemDefinitionIndex() != WEAPON_TASER) {

		return false;
	}

	// setup bones for all valid targets.
	for (int i{ 1 }; i <= 64; ++i) {
		auto player = csgo.m_entity_list()->GetClientEntity(i);
		auto r_log = &feature::resolver->player_records[i - 1];

		if (!player ||
			player->IsDormant() ||
			!player->IsPlayer() ||
			player->m_iHealth() <= 0 ||
			player->m_iTeamNum() == ctx.m_local()->m_iTeamNum() ||
			player->m_bGunGameImmunity()) {
				continue;
		}

		auto data = &feature::lagcomp->records[i - 1];

		if (!data || data->player != player || data->records_count < 1) {
			continue;
		}

		if (ctx.shots_fired[i - 1] < 1)
			r_log->last_shot_missed = false;

		// store player as potential target this tick.
		m_targets[m_targets_count++] = player;
	}

	ctx.do_autostop = true;

	// scan available targets... if we even have any.
	find(cmd);

	if (ctx.m_settings.fake_lag_shooting && feature::anti_aim->did_shot_in_chocked_cycle) {
		cmd->buttons &= ~IN_ATTACK;
		ctx.do_autostop = false;
	}

	apply(cmd, send_packet);

	if (!m_target || m_damage == 0)
		ctx.do_autostop = false;

	return m_target && m_damage > 0 /*&& ctx.m_ragebot_shot_nr == cmd->command_number*/ && cmd->buttons & IN_ATTACK;
}

void c_aimbot::find(CUserCmd* cmd) {
	struct BestTarget_t { C_BasePlayer* player; Vector pos; float damage; int hitbox; };

	Vector       tmp_pos;
	float        tmp_damage;
	int				tmp_hitbox;
	BestTarget_t best;
	best.player = nullptr;
	best.damage = -1.f;
	best.pos = Vector{};
	best.hitbox = -1;

	int players_iterated = 0;

	if (m_targets[0].entity == nullptr || m_targets_count < 1)
		return;

	// iterate all targets.
	for (auto i = 0; i < m_targets_count; i++) {
		auto& target = m_targets[i];
		auto t = m_targets[i].entity;

		if (!t ||
			t->IsDormant() ||
			!t->IsPlayer() ||
			t->m_iHealth() <= 0 ||
			t->m_iTeamNum() == ctx.m_local()->m_iTeamNum() ||
			t->m_bGunGameImmunity()
			) continue;

		auto data = &feature::lagcomp->records[target.idx - 1];
		auto rdata = &feature::resolver->player_records[target.idx - 1];

		if (!data || data->records_count < 1 || data->player != t)
			continue;

		data->best_record = nullptr;

		C_Tickrecord* last = feature::resolver->find_first_available(t, data, false);

		if (!last || !last->data_filled || last->dormant)
			continue;

		if (last->animated && !data->saved_info.fakeplayer)
		{
			float right_dmg;
			float left_dmg;

			last->apply(t, false, true);
			memcpy(t->m_CachedBoneData().Base(), last->rightmatrixes, last->bones_count * sizeof(matrix3x4_t));
			t->m_bone_count() = last->bones_count;
			t->force_bone_cache();
			auto r_head = feature::ragebot->get_hitbox(t, HITBOX_HEAD, last->rightmatrixes);
			right_dmg = feature::autowall->CanHit(ctx.m_eye_position, r_head, ctx.m_local(), t, HITBOX_HEAD);
			memcpy(t->m_CachedBoneData().Base(), last->leftmatrixes, last->bones_count * sizeof(matrix3x4_t));
			t->force_bone_cache();
			auto l_head = feature::ragebot->get_hitbox(t, HITBOX_HEAD, last->leftmatrixes);

			left_dmg = feature::autowall->CanHit(ctx.m_eye_position, l_head, ctx.m_local(), t, HITBOX_HEAD);

			if (left_dmg > 0)
				rdata->freestand_left_tick++;

			if (right_dmg > 0)
				rdata->freestand_right_tick++;

			if (fmaxf(left_dmg, right_dmg) >= 1 && abs(left_dmg - right_dmg) > 10.f)//bool(right_dmg >= 1) != bool(left_dmg >= 1))
			{
				int new_freestand_resolver = 0;

				if (right_dmg > left_dmg)
					new_freestand_resolver = 1;
				else
					new_freestand_resolver = 2;

				last->freestanding_index = new_freestand_resolver;
				rdata->freestanding_update_time = csgo.m_globals()->realtime;
				rdata->last_tick_damageable = ctx.current_tickcount;

				//if (rdata->freestanding_side != new_freestand_resolver)
					rdata->freestanding_updates++;

				rdata->freestanding_side = new_freestand_resolver;
			}
		}

		C_Tickrecord* ideal = (last && last->shot_this_tick ? nullptr : feature::resolver->find_shot_record(t, data));

		if (ideal && ideal->data_filled && !ideal->breaking_lc)
		{
			SetupHitboxes(t, ideal, false);

			if (m_hitboxes.empty())
				continue;

			// try to select best record as target.
			if (GetBestAimPosition(t, tmp_pos, tmp_damage, tmp_hitbox, ideal, players_iterated) && SelectTarget(t, ideal, tmp_pos, tmp_damage)) {
				// if we made it so far, set shit.
				best.player = t;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				data->best_record = ideal;
				best.hitbox = tmp_hitbox;
				break;
			}
		}

		SetupHitboxes(t, last, false);

		if (m_hitboxes.empty())
			continue;

		players_iterated++;

		if (GetBestAimPosition(t, tmp_pos, tmp_damage, tmp_hitbox, last, players_iterated) && SelectTarget(t, last, tmp_pos, tmp_damage)) {
			// if we made it so far, set shit.
			best.player = t;
			best.pos = tmp_pos;
			best.damage = tmp_damage;
			data->best_record = last;
			best.hitbox = tmp_hitbox;
			break;
		}

		if (Engine::Prediction::Instance()->m_flFrameTime > csgo.m_globals()->interval_per_tick && ideal && ideal->data_filled && !ideal->breaking_lc || !m_weapon()->can_shoot())
			continue;

		C_Tickrecord* old = feature::resolver->find_first_available(t, data, true);

		if (!old || !old->data_filled || old->dormant || !old->breaking_lc)
			continue;

		SetupHitboxes(t, old, false);

		if (m_hitboxes.empty())
			continue;

		if (GetBestAimPosition(t, tmp_pos, tmp_damage, tmp_hitbox, old, players_iterated) && SelectTarget(t, old, tmp_pos, tmp_damage)) {
			// if we made it so far, set shit.
			best.player = t;
			best.pos = tmp_pos;
			best.damage = tmp_damage;
			data->best_record = old;
			best.hitbox = tmp_hitbox;
			break;
		}
	}

	// verify our target and set needed data.
	if (best.player && ctx.m_local()) {
		auto data = &feature::lagcomp->records[best.player->entindex() - 1];
		auto r_data = &feature::resolver->player_records[best.player->entindex() - 1];

		// calculate aim angle.
		Math::VectorAngles(best.pos - ctx.m_eye_position, m_angle);

		// set member vars.
		m_target = best.player;
		m_aim = best.pos;
		m_damage = best.damage;
		m_hitbox = best.hitbox;

		if (m_damage > 0) {
			// write data, needed for traces / etc.
			data->best_record->apply(m_target, false);

			const auto cur_mul = float(min(100, m_target->m_iHealth())) / m_damage;
			float ndmg = 1;

			auto velocity = Engine::Prediction::Instance()->GetVelocity();

			//ctx.last_aim_state = 1;
			
			if (auto animstate = ctx.m_local()->get_animation_state(); animstate != nullptr && animstate->m_player && m_weapon()->can_shoot()) {
				if (ctx.m_local()->should_fix_modify_eye_pos()) {
					
					const auto oldposeparam = *(float*)(uintptr_t(ctx.m_local()) + (Engine::Displacement::DT_CSPlayer::m_flPoseParameter + 48));
					auto eye_pitch = Math::normalize_angle(m_angle.x + ctx.m_local()->m_viewPunchAngle().x);

					auto angles = QAngle(0.f, ctx.m_local()->get_animation_state()->m_abs_yaw, 0);
					ctx.m_local()->set_abs_angles(angles);

					if (eye_pitch > 180.f)
						eye_pitch = eye_pitch - 360.f;

					eye_pitch = Math::clamp(eye_pitch, -89, 89);
					*(float*)(uintptr_t(ctx.m_local()) + (Engine::Displacement::DT_CSPlayer::m_flPoseParameter + 48)) = Math::clamp((eye_pitch + 89.f) / 176.f, 0.0f, 1.0f);

					ctx.m_local()->force_bone_rebuild();

					const auto absorg = ctx.m_local()->get_abs_origin();
					ctx.m_local()->set_abs_origin(ctx.m_local()->m_vecOrigin());
					ctx.m_local()->SetupBonesEx(0x100);
					ctx.m_local()->set_abs_origin(absorg);

					ctx.m_local()->force_bone_cache();

					ctx.m_eye_position = ctx.m_local()->GetEyePosition(); //call weapon_shootpos
					*(float*)(uintptr_t(ctx.m_local()) + (Engine::Displacement::DT_CSPlayer::m_flPoseParameter + 48)) = oldposeparam;
					
					Math::VectorAngles(best.pos - ctx.m_eye_position, m_angle);
				}
			}

			if (ctx.m_settings.aimbot_extra_scan_aim[5] && cur_mul >= 1.0f &&
				velocity.Length2D() > 0
				&& cmd->buttons & (IN_MOVERIGHT | IN_MOVELEFT | IN_BACK | IN_FORWARD) && !(cmd->buttons & IN_DUCK))
			{
				velocity.z = 0.f;
				auto pred = ctx.m_eye_position + (velocity * 0.2f);

				Ray_t ray;
				ray.Init(ctx.m_eye_position, pred, ctx.m_local()->OBBMins(), ctx.m_local()->OBBMaxs());

				CTraceFilter filter;
				filter.pSkip = ctx.m_local();
				trace_t tr;

				csgo.m_engine_trace()->TraceRay(ray, MASK_PLAYERSOLID, &filter, &tr);

				if (tr.DidHit())
				{
					const auto frac = (tr.fraction * 0.2f) * 0.95f;
					pred = ctx.m_eye_position + (velocity * frac);
				}

				ray.Init(ctx.m_eye_position, ctx.m_eye_position - Vector(0, 0, 500.f), Vector(-2, -2, -2), Vector(2, 2, 2));
				csgo.m_engine_trace()->TraceRay(ray, MASK_PLAYERSOLID, &filter, &tr);

				pred.z = ctx.m_eye_position.z;

				if (tr.DidHit())
				{
					pred.z = tr.endpos.z + (ctx.m_eye_position.z - ctx.m_local()->m_vecOrigin().z);
				}

				ctx.force_low_quality_autowalling = true;
				ndmg = feature::autowall->CanHit(pred, m_aim, ctx.m_local(), m_target, m_hitbox);

				ctx.force_low_quality_autowalling = false;
			}

			const auto new_mul = float(min(100, m_target->m_iHealth())) / ndmg;

			if (ctx.ticks_allowed < 13)
				ctx.last_speedhack_time = csgo.m_globals()->realtime;

			m_best_hc[0] = 0;
			m_best_hc[1] = 0;

			auto hc_result = !ctx.m_settings.aimbot_hitchance 
				|| ctx.m_settings.aimbot_hitchance && hit_chance(m_angle, m_aim, m_target, ctx.force_aimbot > 0 ? ctx.m_settings.aimbot_hitchance_val / 2 : ctx.m_settings.aimbot_hitchance_val, best.hitbox, 1/*max(min(5, m_damage * 0.2f), 1)*/, m_best_hc);

			bool hit = !ctx.m_settings.aimbot_extra_scan_aim[5] 
				|| ctx.force_aimbot > 0
				|| cur_mul <= new_mul 
				|| cur_mul <= 1.0f 
				|| ctx.exploit_allowed && ctx.main_exploit >= 2 && cur_mul <= 2.f;

			ctx.do_autostop = hit;
			ctx.shot_angles = m_angle;

			auto is_zoomable_weapon = (m_weapon()->m_iItemDefinitionIndex() == WEAPON_SSG08 
				|| m_weapon()->m_iItemDefinitionIndex() == WEAPON_AWP 
				|| m_weapon()->m_iItemDefinitionIndex() == WEAPON_SCAR20 
				|| m_weapon()->m_iItemDefinitionIndex() == WEAPON_G3SG1);

			auto sniper = (m_weapon()->m_iItemDefinitionIndex() == WEAPON_SSG08 || m_weapon()->m_iItemDefinitionIndex() == WEAPON_AWP);

			if (ctx.m_settings.aimbot_autoscope 
				&& is_zoomable_weapon 
				&& !ctx.m_local()->m_bIsScoped() 
				&& !ctx.m_local()->m_bResumeZoom())
			{
				cmd->buttons |= IN_ATTACK2;
				cmd->buttons &= ~IN_ATTACK;
				return;
			}

			if (hit && hc_result)
			{
				if (!m_weapon()->m_iClip1())
				{
					if (m_weapon()->m_iPrimaryReserveAmmoCount() > 0)
						cmd->buttons = cmd->buttons & ~1 | 0x2000;
					else
						ctx.do_autostop = false;

					return;
				}

				ctx.last_aim_state = 3;

				if (!(cmd->buttons & 0x2000) 
					&& (!ctx.m_settings.autostop_force_accuracy
						|| ctx.m_settings.autostop_force_accuracy && (ctx.m_local()->m_vecVelocity().Length2D() + 1) < (ctx.max_weapon_speed * 0.34f)))
				{
					if (!ctx.m_local()->m_bWaitForNoAttack() && m_weapon()->can_shoot()) {
						cmd->buttons |= IN_ATTACK;
					}
				}
			}
		}
	}
	else
		ctx.shot_angles.clear();
}

bool c_aimbot::GetBestAimPosition(C_BasePlayer* player, Vector& aim, float& damage, int& hitbox, C_Tickrecord* record, int players_iterated) {
	bool                  done, pen;
	float                 dmg, pendmg;
	HitscanData_t         scan;
	scan.m_hitchance = 0;
	std::vector< Vector > points;

	// get player hp.
	int hp = min(100, player->m_iHealth());

	scan.m_damage = 0;

	auto data = &feature::lagcomp->records[player->entindex() - 1];
	auto rdata = &feature::resolver->player_records[player->entindex() - 1];

	if (m_weapon()->m_iItemDefinitionIndex() == WEAPON_TASER) {
		dmg = pendmg = hp;
		pen = false;
	}
	else {
		dmg = min(hp, ctx.m_settings.aimbot_min_damage_viable);
		
		pendmg = min(hp, ctx.m_settings.aimbot_min_damage);
		
		if (m_damage_key)
			pendmg = dmg = min(ctx.m_settings.aimbot_min_damage_override_val, hp);

		pen = ctx.m_settings.aimbot_autowall;
	}

	auto resolved = feature::resolver->select_next_side(player, record);
	
	if (rdata->resolver_type != 0) {
		rdata->resolver_index = record->resolver_index;
		rdata->resolver_type = record->resolver_index;
	}

	// apply
	record->apply(player, false);
	data->is_restored = false;

	bool had_any_dmg = false;

	// find hitboxes.
	for (const auto& it : m_hitboxes) {
		done = false;

		bool retard;

		if (it.m_index == HITBOX_HEAD) {

			// nope we did not hit head..
			if (m_baim_key)
				continue;
		}

		// setup points on hitbox.
		if (!player->get_multipoints(it.m_index, points, player->m_CachedBoneData().Base(), retard))
			continue;

		int points_this_hitbox = 0;

		scan.m_safepoint[it.m_index] = false;

		// iterate points on hitbox.
		for (auto& point : points) {
			int safepoints = safe_point(player, ctx.m_eye_position, point, it.m_index, record);

			if (safepoints == 0)// || (!resolved || ctx.shots_fired[player->entindex()-1] > 2) && it.m_index == 0 && !record->shot_this_tick && record->desync_delta > 40.f && safepoints < 2/*((m_weapon()->m_iItemDefinitionIndex() != WEAPON_SCAR20 && m_weapon()->m_iItemDefinitionIndex() != WEAPON_G3SG1) ? safepoints < 2 : safepoints <= 2)*//*|| ctx.shots_fired[player->entindex() - 1] > 1 && safepoints <= 2 && ctx.m_settings.aimbot_extra_scan_aim[2]*/)
				continue;

			float wall_dmg = pendmg;
			float just_dmg = dmg;

			if (m_damage_key)
				just_dmg = wall_dmg = min(hp, ctx.m_settings.aimbot_min_damage_override_val);

			bool was_viable;
			float best_damage_per_hitbox = 0;
			float distance_to_center = 0;

			// we can hit p!
			if (float m_damage = feature::autowall->CanHit(ctx.m_eye_position, point, ctx.m_local(), player, it.m_index, &was_viable); m_damage > 0)
			{
				if (it.m_index == HITBOX_HEAD && ctx.last_hitgroup != HITGROUP_HEAD
					/*|| it.m_index != HITBOX_HEAD && ctx.last_hitgroup == HITGROUP_HEAD*/)
					continue;

				if (it.m_index == 3 && m_damage >= (hp * 0.5f))
					rdata->baim_tick = ctx.current_tickcount;

				if (!was_viable)
				{
					if (!pen || m_damage < wall_dmg)
						continue;
				}
				else
				{
					if (m_damage < just_dmg)
						continue;
				}

				auto good_for_safepoint = (!ctx.m_settings.aimbot_extra_scan_aim[4]
					|| !scan.m_safepoint[it.m_index]
					|| scan.m_safepoint[it.m_index] && safepoints > 2);

				bool good_for_baim = false;

				if (it.m_mode == HitscanMode::PREFER && (m_damage * 2.f) >= player->m_iHealth())
					good_for_baim = true;
				else if ((it.m_mode == HitscanMode::LETHAL || it.m_index == 3) && m_damage >= player->m_iHealth()) {
					good_for_baim = true;
				}
				// 2 shots will be sufficient to kill.
				else if ((it.m_mode == HitscanMode::LETHAL2 || ctx.m_local()->m_iShotsFired() >= 2 && it.m_index == 3) && (m_damage * 2.f) >= player->m_iHealth()) {
					good_for_baim = true;
				}

				if (good_for_baim) {
					done = true;//m_damage > scan.m_damage && good_for_safepoint || m_weapon()->m_iItemDefinitionIndex() != WEAPON_SSG08;
					rdata->baim_tick = ctx.current_tickcount;
				}

				auto distance = point.DistanceSquared(points.front());

				if (best_damage_per_hitbox > 0 && (m_damage >= best_damage_per_hitbox || (m_damage >= (best_damage_per_hitbox - 15)) && distance < distance_to_center) && good_for_safepoint)
				{
					scan.m_damage = m_damage;
					scan.m_pos = point;
					scan.m_hitbox = it.m_index;
					scan.m_safepoint[it.m_index] = safepoints > 2;
					best_damage_per_hitbox = m_damage;
					distance_to_center = distance;
				}
				else {
					if (((m_damage > scan.m_damage || (m_damage >= (scan.m_damage - 15)) && distance < distance_to_center) || done && good_for_baim)/* && (scan.m_hitchance * 0.9f) < point_chance */ && good_for_safepoint)
					{
						// save new best data.
						scan.m_damage = m_damage;
						scan.m_pos = point;
						scan.m_hitbox = it.m_index;
						scan.m_safepoint[it.m_index] = safepoints > 2;
						best_damage_per_hitbox = m_damage;
						distance_to_center = distance;
					}
				}

				if ((ctx.m_settings.aimbot_hitboxes[3] || ctx.main_exploit >= 2)
					&& (scan.m_safepoint[it.m_index] 
						|| scan.m_pos == points.front() && scan.m_hitbox == 3 
						|| scan.m_hitbox != 0 && distance <= distance_to_center)
					&& (scan.m_damage >= (hp * 0.5f)
						&& ctx.exploit_allowed
						&& ctx.main_exploit >= 2
						&& !ctx.fakeducking
						&& (ctx.ticks_allowed > 13 || ctx.force_aimbot > 1)
						|| scan.m_damage >= hp))
					done = true;
			}
		}

		// ghetto break out of outer loop.
		if (done)
			break;
	}

	if (scan.m_damage > 0.f) {
		aim = scan.m_pos;
		damage = scan.m_damage;
		hitbox = scan.m_hitbox;
		return true;
	}
	else
		rdata->damage_ticks = 0;

	return false;
}

float GetFOV(const QAngle& view_angles, const Vector& start, const Vector& end) {
	Vector dir, fw;

	// get direction and normalize.
	dir = (end - start).Normalized();

	// get the forward direction vector of the view angles.
	Math::AngleVectors(view_angles, &fw);

	// get the angle between the view angles forward directional vector and the target location.
	return max(RAD2DEG(std::acos(fw.Dot(dir))), 0.f);
}

bool c_aimbot::SelectTarget(C_BasePlayer* player, C_Tickrecord* record, const Vector& aim, float damage) {
	float dist, fov, height;
	int   hp;

	// fov check.
	if (ctx.m_settings.aimbot_fov_limit > 0 && ctx.m_settings.aimbot_fov_limit < 180) {
		// if out of fov, retn false.
		if (GetFOV(ctx.cmd_original_angles, ctx.m_eye_position, aim) > ctx.m_settings.aimbot_fov_limit)
			return false;
	}

	if (ctx.m_last_shot_index == player->entindex() && damage > 0)
		return true;

	switch (ctx.m_settings.aimbot_target_selection) {

		// distance.
	case 0:
		dist = (record->origin - ctx.m_eye_position).Length();

		if (dist < m_best_dist) {
			m_best_dist = dist;
			return true;
		}

		break;

		// crosshair.
	case 1:
		fov = Math::GetFov(Engine::Movement::Instance()->m_qRealAngles, Math::CalcAngle(ctx.m_eye_position, aim));

		if (fov < m_best_fov) {
			m_best_fov = fov;
			return true;
		}

		break;

		// damage.
	case 2:
		if (damage > m_best_damage) {
			m_best_damage = damage;
			return true;
		}

		break;

		// lowest hp.
	case 3:
		// fix for retarded servers?
		hp = min(100, player->m_iHealth());

		if (hp < m_best_hp) {
			m_best_hp = hp;
			return true;
		}

		break;

		// least lag.
	case 4:
		if (record->lag < m_best_lag) {
			m_best_lag = record->lag;
			return true;
		}

		break;

		// height.
	case 5:
		height = record->origin.z - ctx.m_local()->m_vecOrigin().z;

		if (height < m_best_height) {
			m_best_height = height;
			return true;
		}

		break;

	default:
		return false;
	}

	return false;
}

void c_aimbot::apply(CUserCmd* cmd, bool* send_packet) {
	bool attack;

	// attack states.
	attack = cmd->buttons & IN_ATTACK;
	// ensure we're attacking.
	if (attack) {
		// choke every shot.

		auto v89 = csgo.m_client_state()->m_iChockedCommands;

		if (ctx.m_settings.fake_lag_shooting)
		{
			if (v89 < 14)
				*send_packet = false;
		}
		else if (!ctx.fakeducking)
			*send_packet = true;

		if (m_target) {
			c_player_records* log = &feature::lagcomp->records[m_target->entindex()-1];
			auto r_log = &feature::resolver->player_records[m_target->entindex()-1];
			if (log->best_record && !log->best_record->exploit && !log->best_record->lc_exploit)
				cmd->tick_count = TIME_TO_TICKS(log->best_record->simulation_time + ctx.lerp_time);

			// set angles to target.
			cmd->viewangles = m_angle;

			ctx.fside *= -1;

			//csgo.m_debug_overlay()->AddLineOverlay(ctx.m_eye_position, m_aim, 255, 0, 0, true, 10.f);

			if (ctx.m_settings.misc_visuals_hitboxes)
				visualize_hitboxes(m_target, (log->best_record->resolver_index != 0 && !log->saved_info.fakeplayer ? (log->best_record->resolver_index == 1 ? log->best_record->leftmatrixes : log->best_record->rightmatrixes) : log->best_record->matrixes), ctx.flt2color(ctx.m_settings.misc_visuals_hitboxes_color), (float)ctx.m_settings.misc_visuals_hitboxes_time);

			if (log->best_record != nullptr) {
				if (ctx.shots_fired[m_target->entindex() - 1] < 1) {
					r_log->last_resolving_method = log->best_record->resolver_index;
					r_log->last_resolved_side = log->best_record->resolver_index;
				}
				else
					r_log->last_resolved_side = log->best_record->resolver_index;

				if (log->best_record->shot_this_tick) {

					if (r_log->missed_shots[R_SHOT] == 0)
						r_log->missed_side1st[R_SHOT] = log->best_record->resolver_index;

					r_log->missed_shots[R_SHOT]++;
				}
				else {
					if (log->best_record->resolver_delta_multiplier > .7f)
						r_log->missed_shots[R_60_DELTA]++;
					else
						r_log->missed_shots[R_40_DELTA]++;

					if (r_log->missed_shots[R_USUAL] == 0)
						r_log->missed_side1st[R_USUAL] = log->best_record->resolver_index;

					r_log->missed_shots[R_USUAL]++;
				}
			}

			if (!log->best_record)
				r_log->missed_shots[R_USUAL]++;

			ctx.shots_fired[m_target->entindex() - 1]++;


			r_log->last_shot_missed = false;
			ctx.autopeek_back = true;

			// if not silent aim, apply the viewangles.
			if (!ctx.m_settings.aimbot_silent_aim)
				csgo.m_engine()->SetViewAngles(m_angle);

			const auto did_shot = feature::resolver->add_shot(ctx.m_eye_position, m_aim, log->best_record, feature::misc->hitbox_to_hitgroup(m_hitbox), m_damage, m_target->entindex());

			// set that we fired.
			ctx.m_ragebot_shot_nr = cmd->command_number;
			ctx.m_last_shot_index = m_target->entindex();
		}
	}
}