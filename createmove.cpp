#include "hooked.hpp"
#include "displacement.hpp"
#include "player.hpp"
#include "weapon.hpp"
#include "prediction.hpp"
#include "movement.hpp"
#include "anti_aimbot.hpp"
#include "misc.hpp"
#include <intrin.h>
#include "usercmd.hpp"
#include "lag_comp.hpp"
#include "rage_aimbot.hpp"
//#include "music_player.hpp"
#include "grenades.hpp"
#include "resolver.hpp"
#include "visuals.hpp"
#include "menu.hpp"
#include "aimbot.hpp"

void ApplyDTShift(int cmd_num, int commands_to_shift, int original_choke)
{
	auto shift_data = &Engine::Prediction::Instance()->m_tickbase_array[cmd_num % 150];

	auto latency_ticks = ctx.last_cmd_delta;

	if (ctx.last_4_deltas.size() >= 3) {
		latency_ticks = 0;
		int added = 0;


		for (auto d : ctx.last_4_deltas)
		{
			latency_ticks += d;
			++added;
			if (added >= 4)
				break;
		}

		latency_ticks /= 4;
	}

	auto serverTickcount = csgo.m_globals()->tickcount + latency_ticks;
	const int nCorrectionTicks = TIME_TO_TICKS(0.029999999f);

	int	nIdealFinalTick = serverTickcount + nCorrectionTicks;
	auto simulation_ticks = commands_to_shift + 1 + original_choke;
	int nCorrectedTick = nIdealFinalTick - simulation_ticks + 1;

	shift_data->command_num = cmd_num;
	shift_data->extra_shift = 0;
	shift_data->increace = false;
	shift_data->doubletap = true;
	shift_data->tickbase_original = nCorrectedTick;

	csgo.m_prediction()->m_previous_startframe = -1;
	csgo.m_prediction()->m_nCommandsPredicted = 0;
}

void CopyCommandSkeet(CUserCmd* from_cmd)
{
	auto viewangles = QAngle::Zero;

	auto net_channel = static_cast<INetChannel*>(csgo.m_client_state()->m_ptrNetChannel);

	csgo.m_engine()->GetViewAngles(viewangles);

	const auto o_chocked = csgo.m_client_state()->m_iChockedCommands;

	auto commands_to_shift = ctx.ticks_allowed - o_chocked - 1;

	if (commands_to_shift > 13)
		commands_to_shift = 13;

	if (commands_to_shift > 0 && net_channel)
	{
		const auto time = csgo.m_globals()->curtime;

		from_cmd->sidemove = Engine::Movement::Instance()->old_movement.y;
		from_cmd->forwardmove = Engine::Movement::Instance()->old_movement.x;

		if (!ctx.m_settings.aimbot_extra_doubletap_options[0] || ctx.m_local()->m_vecVelocity().Length2D() <= 2) {
			from_cmd->sidemove = 0.f;
			from_cmd->forwardmove = 0.f;
		}
		else
		{
			if (abs(from_cmd->sidemove) > 10.f)
				from_cmd->sidemove = copysignf(450.f, from_cmd->sidemove);
			else
				from_cmd->sidemove = 0.f;

			if (abs(from_cmd->forwardmove) > 10.f)
				from_cmd->forwardmove = copysignf(450.f, from_cmd->forwardmove);
			else
				from_cmd->forwardmove = 0.f;
		}

		auto m_nTickBase = &ctx.m_local()->m_nTickBase();

		*m_nTickBase -= commands_to_shift;

		csgo.m_globals()->curtime = TICKS_TO_TIME(*m_nTickBase);
		++* m_nTickBase;

		std::vector<CUserCmd> cmds = {};

		for (auto i = 0; i < commands_to_shift; i++)
		{
			auto command = &cmds.emplace_back();

			memcpy(command, from_cmd, 0x64);

			command->viewangles = viewangles;
			command->command_number += i;

			Engine::Prediction::Instance()->Predict(command);
			Engine::Movement::Instance()->FixMove(command, ctx.cmd_original_angles);

			command->viewangles.Clamp();
		}

		csgo.m_globals()->curtime = time;

		for (auto i = 0; i < commands_to_shift; i++)
		{
			auto command = csgo.m_input()->GetUserCmd(cmds[i].command_number);
			auto v8 = csgo.m_input()->GetVerifiedUserCmd(cmds[i].command_number);

			memcpy(command, &cmds[i], sizeof(CUserCmd));

			bool v6 = command->tick_count == 0x7F7FFFFF;
			command->hasbeenpredicted = v6;

			v8->m_cmd = *command;
			v8->m_crc = command->GetChecksum();

			++net_channel->choked_packets;
			++net_channel->out_sequence_nr;
			++csgo.m_client_state()->m_iChockedCommands;
		} 

		if (commands_to_shift <= 15)
			ApplyDTShift(from_cmd->command_number, commands_to_shift, o_chocked);

		ctx.force_aimbot = commands_to_shift + from_cmd->command_number;
		ctx.allow_shooting = commands_to_shift + from_cmd->command_number;

		cmds.clear();
	}
}

void CopyCommand(CUserCmd* from_cmd)
{
	auto net_channel = static_cast<INetChannel*>(csgo.m_client_state()->m_ptrNetChannel);
	auto viewangles = QAngle::Zero;

	csgo.m_engine()->GetViewAngles(viewangles);
	auto pred = &Engine::Prediction::Instance();

	from_cmd->sidemove = Engine::Movement::Instance()->old_movement.y;
	from_cmd->forwardmove = Engine::Movement::Instance()->old_movement.x;

	if (!ctx.m_settings.aimbot_extra_doubletap_options[0] || ctx.m_local()->m_vecVelocity().Length2D() <= 2) {
		from_cmd->sidemove = 0.f;
		from_cmd->forwardmove = 0.f;
	}
	else
	{
		if (abs(from_cmd->sidemove) > 10.f)
			from_cmd->sidemove = copysignf(450.f, from_cmd->sidemove);
		else
			from_cmd->sidemove = 0.f;

		if (abs(from_cmd->forwardmove) > 10.f)
			from_cmd->forwardmove = copysignf(450.f, from_cmd->forwardmove);
		else
			from_cmd->forwardmove = 0.f;
	}

	from_cmd->buttons &= ~(IN_ATTACK | IN_ATTACK2);

	ctx.started_speedhack = ctx.current_tickcount;
	ctx.shifted_cmd = from_cmd->command_number;

	ctx.tickbase_started_teleport = ctx.m_local()->m_nTickBase();

	auto commands_to_shift = 13;
	
	if (commands_to_shift <= 0 || ctx.ticks_allowed < 13)
		return;

	ctx.speedhack_choke = csgo.m_client_state()->m_iChockedCommands;

	int* tb = &ctx.m_local()->m_nTickBase();
	const auto btb = *tb;

	auto latency_ticks = ctx.last_cmd_delta;

	if (ctx.last_4_deltas.size() >= 3) {
		latency_ticks = 0;
		int added = 0;
		for (auto d : ctx.last_4_deltas)
		{
			latency_ticks += d;
			++added;
			if (added >= 4)
				break;
		}
		latency_ticks /= 4;
	}

	auto serverTickcount = ctx.current_tickcount + latency_ticks;
	auto simulation_ticks = 13 + csgo.m_client_state()->m_iChockedCommands;
	int nCorrectedTick = TIME_TO_TICKS(0.03f) + serverTickcount - simulation_ticks + 1;

	auto cmd_nr = csgo.m_client_state()->m_iLastOutgoingCommand + 1;

	auto shift_data = &pred->m_tickbase_array[cmd_nr % 150];

	if (shift_data)
	{
		shift_data->command_num = cmd_nr;
		shift_data->increace = true;
		shift_data->doubletap = true;
		shift_data->tickbase_original = nCorrectedTick;
	}

	net_channel->set_chocked();
	++csgo.m_client_state()->m_iChockedCommands;

	const auto time = csgo.m_globals()->curtime;

	csgo.m_prediction()->m_nCommandsPredicted = 0;
	csgo.m_prediction()->m_previous_startframe = -1;

	auto commands_to_add = 0;
	do
	{
		csgo.m_prediction()->Update(csgo.m_client_state()->m_iDeltaTick, true, csgo.m_client_state()->m_iLastCommandAck,
			csgo.m_client_state()->m_iLastOutgoingCommand + csgo.m_client_state()->m_iChockedCommands);

		const auto v2 = commands_to_add + from_cmd->command_number;
		auto command = csgo.m_input()->GetUserCmd(v2);
		auto v8 = csgo.m_input()->GetVerifiedUserCmd(v2);

		memcpy(command, from_cmd, 0x64);
		auto v7 = (command->tick_count == 0x7F7FFFFF);

		auto vel = sqrtf((ctx.m_local()->m_vecVelocity().z * ctx.m_local()->m_vecVelocity().z)
			+ (ctx.m_local()->m_vecVelocity().x * ctx.m_local()->m_vecVelocity().x)
			+ (ctx.m_local()->m_vecVelocity().y * ctx.m_local()->m_vecVelocity().y));

		command->command_number = v2;

		if (ctx.m_local()->m_fFlags() & FL_ONGROUND && ctx.m_settings.aimbot_extra_doubletap_options[1] && commands_to_add >= (4 * int((ctx.max_weapon_speed * 0.34f) > vel) + 7))
			Engine::Movement::Instance()->Quick_stop(command);

		if (!ctx.auto_peek_spot.IsZero())
		{
			auto angle = Math::CalcAngle(ctx.m_local()->m_vecAbsOrigin(), ctx.auto_peek_spot);
			viewangles.y = angle.y;

			command->forwardmove = 450.f;
			command->sidemove = 0.0f;
		}

		auto v64 = (ctx.m_local()->m_flDuckAmount() * 0.34f) + 1.0f;
		auto v47 = 1.0f / (v64 - ctx.m_local()->m_flDuckAmount());
		command->forwardmove = command->forwardmove * v47;
		command->sidemove = v47 * command->sidemove;

		if (command->sidemove > 450.f)
			command->sidemove = 450.f;
		else if (command->sidemove < -450.f)
			command->sidemove = -450.f;

		if (command->forwardmove > 450.f)
			command->forwardmove = 450.f;
		else if (command->forwardmove < -450.f)
			command->forwardmove = -450.f;

		Engine::Movement::Instance()->FixMove(command, viewangles);

		command->hasbeenpredicted = 0;

		command->viewangles.Clamp();
		memcpy(&v8->m_cmd, command, 0x64);
		v8->m_crc = command->GetChecksum();

		if ((commands_to_add + 1) < commands_to_shift) {
			net_channel->set_chocked();
			++csgo.m_client_state()->m_iChockedCommands;
		}
		++commands_to_add;
		ctx.ticks_allowed--;
	} while (commands_to_add < commands_to_shift);

	csgo.m_globals()->curtime = time;
	
	ctx.force_aimbot = from_cmd->command_number + 13;// -2;
	ctx.allow_shooting = from_cmd->command_number + 1;// - 2
	ctx.last_speedhack_time = csgo.m_globals()->realtime;
}

bool IsTickcountValid(int tick) {
	return (ctx.cmd_tickcount + 64 + 8) > tick;
}

void ApplyShift(CUserCmd* cmd, bool* bSendPacket) {
	if (*bSendPacket) {
		INetChannel* net_channel = (INetChannel*)(csgo.m_client_state()->m_ptrNetChannel);
		auto v7 = net_channel->choked_packets;
		if (v7 >= 0) {
			auto v8 = ctx.ticks_allowed;
			auto v9 = cmd->command_number - v7;
			do
			{
				auto v10 = &csgo.m_input()->m_pCommands[cmd->command_number - 150 * (v9 / 150) - v7];
				if (!v10
					|| IsTickcountValid(v10->tick_count))
				{
					if (--v8 <= 0)
						v8 = 0;
					ctx.ticks_allowed = v8;
				}
				++v9;
				--v7;
			}
			while (v7 >= 0);
		}
	}

	auto v14 = ctx.ticks_allowed;
	if (v14 > 16) {
		auto v15 = v14 - 1;
		ctx.ticks_allowed = Math::clamp(v15, 0, 16);
	}
}

void CheckPaused(CUserCmd* cmd)
{
	if (!csgo.m_engine()->IsPaused())
		return;
	cmd->buttons = 0;
	cmd->forwardmove = 0;
	cmd->sidemove = 0;
	cmd->upmove = 0;

	// Don't allow changing weapons while paused
	cmd->weaponselect = 0;
}

namespace Hooked
{
	bool __stdcall CreateMove(float flInputSampleTime, CUserCmd* cmd)
	{
#ifdef VIRTUALIZER
#endif // VIRTUALIZER

		ctx.fix_modify_eye_pos = false;
		auto& prediction = Engine::Prediction::Instance();
		auto& movement = Engine::Movement::Instance();

		bool cant_repredict = false;
		//ctx.doubletap_now = false;

		if (ctx.updated_skin)
		{
			if (csgo.m_engine()->IsInGame() && ctx.m_local()) {
				csgo.m_client_state()->m_iDeltaTick = -1;
			}
			ctx.updated_skin = false;

			return false;
		}

		if (!cmd || !ctx.m_local())
			return false;

		bool original = vmt.m_clientmode->VCall<bool(__stdcall*)(float, CUserCmd*)>(Index::IBaseClientDLL::CreateMove)(flInputSampleTime, cmd);

		if (original) {
			csgo.m_engine()->SetViewAngles(cmd->viewangles);
			csgo.m_prediction()->SetLocalViewAngles(cmd->viewangles);
		}

		if (cmd->command_number == 0)
			return false;

		feature::lagcomp->update_lerp();
		feature::lagcomp->update_network_info();

		if (ctx.buy_weapons && ctx.m_settings.misc_autobuy_enabled)
		{
			std::string buy;

			ctx.ticks_allowed = 0;

			if (ctx.m_settings.misc_autobuy_primary == 1)
			{
				buy += "buy ak-47; ";
			}
			else if (ctx.m_settings.misc_autobuy_primary == 2)
			{
				buy += "buy awp; ";
				
			}
			else if (ctx.m_settings.misc_autobuy_primary == 3) {
				buy += "buy scar20; ";
			
			}
			else if (ctx.m_settings.misc_autobuy_primary == 5) {
				buy += "buy ssg08; ";
				
			}

			if (ctx.m_settings.misc_autobuy_secondary == 1)
			{
				buy += "buy deagle; ";
				
			}
			else if (ctx.m_settings.misc_autobuy_secondary == 2)
			{
				buy += "buy tec9; ";
				
			}
			else if (ctx.m_settings.misc_autobuy_secondary == 3)
			{
				buy += "buy elite; ";

			}
			else if (ctx.m_settings.misc_autobuy_secondary == 4)
			{
				buy += "buy p250; ";
				
			}

			if (ctx.m_settings.misc_autobuy_etc[4]) {
				buy += "buy hegrenade; ";
				
			}
			if (ctx.m_settings.misc_autobuy_etc[7]) {
				buy += "buy molotov; ";
			
			}
			if (ctx.m_settings.misc_autobuy_etc[3]) {
				buy += "buy smokegrenade; ";
			
			}
			if (ctx.m_settings.misc_autobuy_etc[5]) {
				buy += "buy flashbang; ";
			
			}
			if (ctx.m_settings.misc_autobuy_etc[6]) {
				buy += "buy flashbang; ";
			
			}
			if (ctx.m_settings.misc_autobuy_etc[0]) {
				buy += "buy taser; ";
				
			}
			if (ctx.m_settings.misc_autobuy_etc[2]) {
				buy += "buy defuser; ";
			
			}
			if (ctx.m_settings.misc_autobuy_etc[1])
			{
				buy += "buy vesthelm; ";
				buy += "buy vest; ";
				
			}

			if (!buy.empty() && (ctx.m_settings.misc_autobuy_money_limit == 0 || ctx.m_local()->m_iAccount() > ctx.m_settings.misc_autobuy_money_limit))
				csgo.m_engine()->ClientCmd_Unrestricted(buy.c_str()); //TODO: maybe find another method how to autobuy? since this one is ghetto asf

			ctx.has_scar = false;
			ctx.buy_weapons = false;
		}

		ctx.fix_modify_eye_pos = true;
		ctx.applied_tickbase = false;
		bool send_packet = true;
		static bool did_shift_before = false;
		ctx.start_switching = false;
		ctx.did_shot = false;
		ctx.exploit_tickbase_shift = 0;
		ctx.shift_amount = 0;
		ctx.max_weapon_speed = 250.f;
		ctx.can_aimbot = true;
		bool skip_fakelags = false;
		feature::anti_aim->skip_fakelag_this_tick = false;
		Engine::Movement::Instance()->did_force = false;

		ctx.active_keybinds[14].mode = 0;

		ctx.air_stuck = ctx.get_key_press(ctx.m_settings.anti_aim_timestop_key);
		if (ctx.air_stuck)
			ctx.active_keybinds[14].mode = ctx.m_settings.anti_aim_timestop_key.mode + 1;

		if (csgo.m_game_rules()) {
			ctx.active_keybinds[0].mode = 0;
			if (ctx.fakeducking)
			{
				if (!csgo.m_game_rules()->IsValveDS())
					ctx.active_keybinds[0].mode = ctx.m_settings.anti_aim_fakeduck_key.mode + 1;
			}
		}

		ctx.doubletap_charged = false;
		ctx.is_cocking = false;
		ctx.is_able_to_shoot = false;

		ctx.cmd_original_angles = cmd->viewangles;
		ctx.cmd_tickcount = cmd->tick_count;
		 
		ctx.cmd_original_buttons = cmd->buttons;
		ctx.current_realtime = csgo.m_globals()->realtime;
		ctx.tickrate = 1.f / csgo.m_globals()->interval_per_tick;

		const auto is_switching_weapons = cmd->weaponselect != 0;

		if (feature::anti_aim->did_shot_in_chocked_cycle && csgo.m_client_state()->m_iChockedCommands == 0)
			feature::anti_aim->did_shot_in_chocked_cycle = false;

		if (is_switching_weapons || feature::menu->_menu_opened || ctx.m_local()->m_bWaitForNoAttack() || ctx.m_settings.fake_lag_shooting && feature::anti_aim->did_shot_in_chocked_cycle) {
			cmd->buttons &= ~IN_ATTACK;
			//cmd->buttons &= ~IN_ATTACK2;
		}

		if (ctx.m_local() && m_weapon() != nullptr) {
			ctx.latest_weapon_data = m_weapon()->GetCSWeaponData();
			ctx.max_weapon_speed = (!ctx.m_local()->m_bIsScoped() ? *(float*)(uintptr_t(ctx.latest_weapon_data) + 0x130) : *(float*)(uintptr_t(ctx.latest_weapon_data) + 0x134));
		}

		ctx.m_eye_position.clear();

		const auto aim_toggled = ctx.m_settings.aimbot_enabled;
		
		if (ctx.m_settings.aimbot_key != 0)
		{
			if (ctx.get_key_press(ctx.m_settings.aimbot_key))
				ctx.allows_aimbot = !ctx.allows_aimbot;

			ctx.m_settings.aimbot_enabled = ctx.allows_aimbot;
		}
		else
			ctx.allows_aimbot = ctx.m_settings.aimbot_enabled;

		if (ctx.m_local() && ctx.m_local()->GetClientClass() && !ctx.m_local()->IsDead())
		{
			if ((vmt.m_clientstate == nullptr || !vmt.m_clientstate->is_hooked) && csgo.m_client_state() != nullptr && (CClientState*)(uint32_t(csgo.m_client_state()) + 8) != nullptr && csgo.m_game_rules())// && ctx.m_local() != nullptr && !ctx.m_local()->IsDead())
			{
				vmt.m_clientstate = std::make_shared<Memory::VmtSwap>((CClientState*)(uint32_t(csgo.m_client_state()) + 8));

				if (vmt.m_clientstate) {
					vmt.m_clientstate->Hook(&Hooked::PacketEnd, 6);
					vmt.m_clientstate->Hook(&Hooked::PacketStart, 5);
				}
			}

			if ((vmt.m_net_channel == nullptr || !vmt.m_net_channel->is_hooked) && csgo.m_client_state() != nullptr && csgo.m_client_state()->m_ptrNetChannel != nullptr && csgo.m_game_rules())// && ctx.m_local() != nullptr && !ctx.m_local()->IsDead())
			{
				vmt.m_net_channel = std::make_shared<Memory::VmtSwap>((DWORD**)csgo.m_client_state()->m_ptrNetChannel);

				if (vmt.m_net_channel) {
					vmt.m_net_channel->Hook(&Hooked::SendNetMsg, 40);
					vmt.m_net_channel->Hook(&Hooked::Shutdown, 27);
					vmt.m_net_channel->Hook(&Hooked::SendDatagram, 46);
				}
			}

			if (ctx.last_frame_stage == 4) {
				csgo.m_prediction()->Update(csgo.m_client_state()->m_iDeltaTick, csgo.m_client_state()->m_iDeltaTick > 0, csgo.m_client_state()->m_iLastCommandAck,
					csgo.m_client_state()->m_iLastOutgoingCommand + csgo.m_client_state()->m_iChockedCommands);
			}

			ctx.last_usercmd = cmd; // we don't need usercmds from non-reliable ticks 
			auto exploit_toggled = (!ctx.has_exploit_toggled ? 0 : ctx.main_exploit);

			ctx.fps = 1 / csgo.m_globals()->frametime;

			feature::misc->pre_prediction(cmd);

			ctx.active_keybinds[4].mode = 0;
			if (ctx.doubletap_now && ctx.speed_hacking && ctx.shifted_cmd != cmd->command_number)
			{
				if (!ctx.m_settings.aimbot_extra_doubletap_options[0])
				{
					cmd->sidemove = 0.f;
					cmd->forwardmove = 0.f;
				}

				ctx.was_teleporting = true;

				cmd->buttons &= ~IN_ATTACK;
			}

			movement->Begin(cmd, send_packet);
			movement->did_force = false;

			prediction->PrePrediction(cmd);
			prediction->SetupMovement(cmd);

			if (ctx.m_settings.aimbot_enabled)
				cmd->tick_count += TIME_TO_TICKS(ctx.lerp_time);

			if (csgo.m_game_rules() != nullptr && !csgo.m_game_rules()->IsValveDS()/* && cmd->buttons & IN_DUCK*/ || ctx.fakeducking)
				cmd->buttons |= IN_BULLRUSH;

			ctx.fakeduck_will_choke = 0;

			if (ctx.fakeducking)
			{
				if (!ctx.fakeducking_prev_state)
				{
					if (ctx.m_local()->m_flDuckAmount() > 0)
						cmd->buttons |= IN_DUCK;

					send_packet = true;
				}
				else
				{
					if (csgo.m_client_state()->m_iChockedCommands <= 6)
						cmd->buttons &= ~IN_DUCK;
					else
						cmd->buttons |= IN_DUCK;

					// credits: onetap
					ctx.fakeduck_will_choke = (14 - csgo.m_client_state()->m_iChockedCommands);

					send_packet = (csgo.m_client_state()->m_iChockedCommands >= 14);
				}

				ctx.accurate_max_previous_chocked_amt = 14;

				ctx.next_shift_amount = 0;
				ctx.shift_amount = 0;
			}

			const Vector old_move = Vector(cmd->forwardmove, cmd->sidemove, cmd->upmove);
			const bool pressed_b4 = cmd->buttons & 0x20000u;

			if (ctx.was_teleporting && !ctx.doubletap_now && !ctx.speed_hacking)
				ctx.do_autostop = true;

			if (m_weapon()) {
				if (ctx.do_autostop)
					feature::ragebot->autostop(cmd, m_weapon());
			}
			
			movement->FixMove(cmd, movement->m_qAnglesView);
			prediction->Predict(cmd);

			C_BasePlayer* viewmodel = csgo.m_entity_list()->GetClientEntityFromHandle(
				reinterpret_cast<CBaseHandle>(ctx.m_local()->m_hViewModel()));

			prediction->m_hWeapon = 0;

			if (viewmodel)
			{
				prediction->m_hWeapon = ((C_BaseViewModel*)viewmodel)->get_viewmodel_weapon();
				prediction->m_nViewModelIndex = ((C_BaseViewModel*)viewmodel)->m_nViewModelIndex();
				prediction->m_nSequence = ((C_BaseViewModel*)viewmodel)->m_nSequence();

				prediction->networkedCycle = viewmodel->m_flCycle();
				prediction->m_nAnimationParity = ((C_BaseViewModel*)viewmodel)->m_nAnimationParity();
				prediction->animationTime = ((C_BaseViewModel*)viewmodel)->m_flModelAnimTime();
			}

	
			feature::grenades->think(cmd);

			ctx.active_keybinds[5].mode = 0;
			ctx.active_keybinds[6].mode = 0;
			ctx.active_keybinds[7].mode = 0;
			ctx.active_keybinds[10].mode = 0;
			ctx.active_keybinds[12].mode = 0;
			ctx.allow_freestanding = false;
			if (ctx.get_key_press(ctx.m_settings.anti_aim_freestanding_key)) {
				ctx.active_keybinds[12].mode = ctx.m_settings.anti_aim_freestanding_key.mode + 1;
				ctx.allow_freestanding = true;
			}

			ctx.active_keybinds[9].mode = 0;
			ctx.active_keybinds[8].mode = 0;

			if (m_weapon() != nullptr && ctx.m_local()->m_MoveType() != 10)
			{
				if (exploit_toggled == 0 || !ctx.exploit_allowed || exploit_toggled == 2 || /*ctx.double_tapped > 0*/ctx.m_local()->m_iShotsFired() > 0 && ctx.next_shift_amount > 0 && exploit_toggled == 3)
					ctx.next_shift_amount = 0;

				feature::lagcomp->backup_players(false);

				if (ctx.m_settings.aimbot_enabled)
					feature::anti_aim->fake_lagv2(cmd, &send_packet);
				else
				{
					if (ctx.m_settings.anti_aim_enabled)
					{
						if (csgo.m_client_state()->m_iChockedCommands == 0)
							send_packet = false;
					}
				}

				if (ctx.was_teleporting && !ctx.doubletap_now && !ctx.speed_hacking) {
					ctx.was_teleporting = false;
				}

				if (csgo.m_client_state()->m_iChockedCommands >= 15u)
					send_packet = 1;

				if (auto weapon = m_weapon(); weapon != nullptr && ctx.m_settings.aimbot_enabled && ctx.m_settings.aimbot_auto_revolver)
				{
					if (weapon->m_iItemDefinitionIndex() == 64)
					{
						auto v29 = /*(float)*/ctx.m_local()->m_nTickBase() * csgo.m_globals()->interval_per_tick;
						ctx.is_cocking = 1;
						cmd->buttons &= ~0x800u;

						if (weapon->can_cock() && !ctx.is_charging)
						{
							if (ctx.r8_timer <= v29)
							{
								if (weapon->m_flPostponeFireReadyTime() <= v29)
									ctx.r8_timer = v29 + 0.234375f;
								else
									cmd->buttons |= 0x800u;
							}
							else
								cmd->buttons |= 1u;

							ctx.is_cocking = v29 > ctx.r8_timer;
						}
						else
						{
							ctx.r8_timer = v29 + 0.234375f;
							ctx.is_cocking = 0;
							cmd->buttons &= ~1u;
						}
					}
				}

				if (ctx.doubletap_now && ctx.speed_hacking && ctx.shifted_cmd != cmd->command_number)
					send_packet = false;

				const auto did_shot = !ctx.m_settings.aimbot_enabled ? false : feature::ragebot->think(cmd, &send_packet);

				const auto can_firstattak = (cmd->buttons & IN_ATTACK && (did_shot || ctx.pressed_keys[1] && m_weapon()->m_iItemDefinitionIndex() == 64 || m_weapon()->m_iItemDefinitionIndex() != 64)) && !is_switching_weapons && !ctx.m_local()->m_bWaitForNoAttack();
				const auto can_secondattk = (cmd->buttons & IN_ATTACK2 && (did_shot && m_weapon()->m_iItemDefinitionIndex() == 64 || m_weapon()->is_knife())) && !is_switching_weapons;

				if (skip_fakelags)
					send_packet = true;

				if (!ctx.fakeducking && feature::anti_aim->did_shot_in_chocked_cycle && ctx.m_settings.anti_aim_typeyfake_shot > 0 && (cmd->command_number - ctx.m_ragebot_shot_nr) == 1) {
					send_packet = true;
					feature::anti_aim->did_shot_in_chocked_cycle = false;
				}

				if (/*ctx.allow_shooting <= cmd->command_number &&*/!ctx.speed_hacking && (can_firstattak || can_secondattk) && (did_shot || m_weapon()->can_shoot()) && (m_weapon()->IsGun() || m_weapon()->is_knife()))//&& ctx.last_shot_time_clientside + 0.05f < ctx.last_time_command_arrived)
				{
					if (ctx.m_settings.aimbot_enabled && ctx.m_settings.aimbot_no_spread)
					{
						auto weapon_inaccuracy = m_weapon()->GetInaccuracy();
						auto weapon_spread = m_weapon()->GetSpread();

						RandomSeed((cmd->random_seed & 255));

						auto rand1 = RandomFloat(0.0f, 1.0f);
						auto rand_pi1 = RandomFloat(0.0f, 2.0f * RadPi);
						auto rand2 = RandomFloat(0.0f, 1.0f);
						auto rand_pi2 = RandomFloat(0.0f, 2.0f * RadPi);

						int id = m_weapon()->m_iItemDefinitionIndex();
						auto recoil_index = m_weapon()->m_flRecoilIndex();

						if (id == 64)
						{
							if (cmd->buttons & IN_ATTACK2)
							{
								rand1 = 1.0f - rand1 * rand1;
								rand2 = 1.0f - rand2 * rand2;
							}
						}
						else if (id == 28 && recoil_index < 3.0f)
						{
							for (int i = 3; i <= recoil_index; i--)
							{
								rand1 *= rand1;
								rand2 *= rand2;
							}

							rand1 = 1.0f - rand1;
							rand2 = 1.0f - rand2;
						}

						auto rand_inaccuracy = rand1 * weapon_inaccuracy;
						auto rand_spread = rand2 * weapon_spread;

						Vector2D spread =
						{
							cosf(rand_pi1) * rand_inaccuracy + cosf(rand_pi2) * rand_spread,
							sinf(rand_pi1) * rand_inaccuracy + sinf(rand_pi2) * rand_spread,
						};


						Vector side, up;
						Vector forward = QAngle::Zero.ToVectors(&side, &up);

						Vector direction = forward + (side * spread.x) + (up * spread.y);
						//direction.Normalize();

						QAngle angles_spread;
						Math::VectorAngles(direction, angles_spread);

						angles_spread.x -= cmd->viewangles.x;
						angles_spread.Normalize();

						forward = angles_spread.ToVectorsTranspose(&side, &up);

						Math::VectorAngles(forward, up, angles_spread);
						angles_spread.Normalize();

						angles_spread.y += cmd->viewangles.y;
						angles_spread.Normalize();

						if (angles_spread.x != 0 || angles_spread.y != 0)
							cmd->viewangles = angles_spread;
					}

					ctx.start_switching = true;
					//cmd->viewangles.Normalize();

					if (!ctx.autopeek_back)
						ctx.autopeek_back = true;

					if (!send_packet)
						feature::anti_aim->did_shot_in_chocked_cycle = true;

					if (ctx.m_settings.aimbot_enabled && ctx.m_settings.aimbot_no_spread) {
						auto recoil = ctx.m_local()->m_aimPunchAngleScaled();

						if (recoil.x != 0 || recoil.y != 0) {
							cmd->viewangles.x = Math::normalize_angle(cmd->viewangles.x) - recoil.x;
							cmd->viewangles.y = Math::normalize_angle(cmd->viewangles.y) - recoil.y;
							cmd->viewangles.z = Math::normalize_angle(cmd->viewangles.z) - recoil.z;
						}
					}
					ctx.last_shot_time_clientside = csgo.m_globals()->realtime;
					ctx.m_ragebot_shot_nr = cmd->command_number;
					ctx.m_ragebot_shot_ang = cmd->viewangles;
					ctx.hold_angles = cmd->viewangles;

					ctx.hold_aim = true;
					ctx.hold_aim_ticks = 0;
					ctx.force_aimbot = 0;

					if (!did_shot)
						feature::resolver->add_shot(ctx.m_eye_position, Vector(0, 0, 0), nullptr, 0, -1, -1);
				}
				else
				{
					if (ctx.m_settings.aimbot_enabled && m_weapon()->m_iItemDefinitionIndex() != 64 && !m_weapon()->IsGrenade() && m_weapon()->IsGun())
						cmd->buttons &= ~IN_ATTACK;

					if (ctx.m_settings.aimbot_enabled || ctx.m_settings.anti_aim_enabled)
						feature::anti_aim->work(cmd, &send_packet);
				}

				if (ctx.doubletap_now && ctx.speed_hacking && ctx.shifted_cmd != cmd->command_number)
					send_packet = false;

				feature::usercmd->cmd_info[cmd->command_number % 150].store(cmd->command_number, ctx.cmd_original_angles, Vector(cmd->forwardmove, cmd->sidemove, cmd->upmove), cmd->viewangles, ctx.m_local()->m_flSimulationTime(), ctx.m_local()->m_fFlags(), ctx.m_local()->m_flDuckAmount(), ctx.m_local()->m_flLowerBodyYawTarget(), ctx.m_local()->m_nTickBase(), ctx.m_local()->m_vecVelocity());

				movement->End(cmd);

				if (ctx.m_settings.aimbot_enabled &&/*send_packet &&*/ ctx.m_local()->m_fFlags() & FL_ONGROUND)
					feature::misc->end(cmd);

				//prediction->Predict(cmd);

				feature::lagcomp->backup_players(true);

				ctx.last_angles = cmd->viewangles;

				if (exploit_toggled != 0 && ctx.exploit_allowed && !ctx.fakeducking && m_weapon()->IsGun() && m_weapon()->m_iItemDefinitionIndex() != WEAPON_REVOLVER && !is_switching_weapons && ctx.ticks_allowed > 12 && exploit_toggled > 0)
				{
					ctx.next_shift_amount = 0;
					ctx.shift_amount = 0;

					if (exploit_toggled == 2)
					{
						//auto can_shift_shot = m_weapon()->can_exploit(14);

						if (cmd->buttons & IN_ATTACK && m_weapon()->can_exploit(13)) {

							ctx.speed_hacking = true;
							ctx.doubletap_now = true;

							ctx.started_speedhack = ctx.current_tickcount;
							ctx.shifted_cmd = cmd->command_number;

							//*tb = btb;

							//*tb += commands_to_shift;

							ctx.last_speedhack_time = csgo.m_globals()->realtime;

							send_packet = false;
							//send_packet = false;
							//ctx.force_next_packet_choke = true;

							//ctx.m_local()->AdjustPlayerTimeBase(ctx.m_local()->DetermineSimulationTicks());
						}
						else
						{
							/*if (ctx.m_settings.aimbot_extra_doubletap_options[2])
							{
								if (ctx.ticks_allowed > 12 && std::fabsf(csgo.m_globals()->realtime - ctx.last_speedhack_time) > 0.5f && ctx.allow_shooting <= cmd->command_number && ctx.force_aimbot < 1) {
									ctx.next_shift_amount = 12;
									ctx.shift_amount = ctx.next_shift_amount;
									send_packet = true;
								}
							}*/
						}
					}
					else if (exploit_toggled == 3)
					{
						static auto fast_recovery = false;
						const auto can_shift_shot = m_weapon()->can_exploit(13);
						const auto can_shot = m_weapon()->can_exploit(-1 - ctx.next_shift_amount);

						if (can_shift_shot || (!can_shot || !fast_recovery) && !did_shift_before)
						{
							ctx.next_shift_amount = 13;
							ctx.double_tapped = 0;
						}
						else {
							ctx.double_tapped++;

							/*if (exploit_toggled == 3 && !can_shift_shot && ctx.m_local()->m_vecVelocity().Length() > 10.f) {
								ctx.speed_hack = 13;
							}*/
							ctx.shift_amount = 0;
							ctx.next_shift_amount = 0;
						}

						/*if (can_shift_shot)
							ctx.can_aimbot = true;
						else
						{
							if (ctx.double_tapped > 10) {
								ctx.can_aimbot = false;
								cmd->buttons &= ~IN_ATTACK;
							}
						}*/
					}
					else
						ctx.next_shift_amount = 11;

					if (exploit_toggled != 2)
					{
						if (ctx.next_shift_amount > 0 && ctx.ticks_allowed > 11) {
							if (m_weapon()->can_exploit(ctx.next_shift_amount) && cmd->buttons & IN_ATTACK && !(cmd->buttons & IN_ATTACK2)) {
								ctx.shift_amount = ctx.next_shift_amount;

								auto shift_data = &prediction->m_tickbase_array[cmd->command_number % 150];

								if (ctx.shift_amount > 0) {
									shift_data->command_num = cmd->command_number;

									shift_data->extra_shift = ~ctx.shift_amount;

									auto latency_ticks = 1;

									if (ctx.last_4_deltas.size() >= 2) {
										latency_ticks = 0;
										int added = 0;
										for (auto d : ctx.last_4_deltas)
										{
											latency_ticks += d;
											++added;
											if (added >= 4)
												break;
										}
										latency_ticks /= 4;
									}

									auto serverTickcount = csgo.m_globals()->tickcount;

									auto simulation_ticks = ctx.shift_amount + csgo.m_client_state()->m_iChockedCommands;
									int nCorrectedTick = TIME_TO_TICKS(0.03f) - simulation_ticks + serverTickcount + latency_ticks;

									shift_data->tickbase_original = nCorrectedTick;

									shift_data->increace = true;
								}

								send_packet = true;
							}
						}

						did_shift_before = ctx.shift_amount != 0;
					}

					cant_repredict = true;
				}
				else
					ctx.next_shift_amount = 0;
			}

			//if (!cant_repredict)
			//	prediction->Predict(cmd);

			// createmove
			if (ctx.shift_amount > 0 /*|| ctx.doubletap_now*/)
			{
				ctx.original_tickbase = ctx.m_local()->m_nTickBase();
				ctx.exploit_tickbase_shift = ctx.doubletap_now ? 14 : abs(ctx.shift_amount);
				ctx.tickbase_shift_nr = cmd->command_number;
			}
			//const auto t = ctx.m_local()->m_nTickBase();

			if (ctx.m_local()->m_vecVelocity().Length2D() > 0.1f || ctx.m_local()->get_animation_state()->m_velocity_length_xy > 100)
				feature::anti_aim->lby_timer = TICKS_TO_TIME(ctx.m_local()->m_nTickBase()) + 0.22f;
			else
			{
				if (TICKS_TO_TIME(ctx.m_local()->m_nTickBase()) > feature::anti_aim->lby_timer) {
					feature::anti_aim->lby_timer = TICKS_TO_TIME(ctx.m_local()->m_nTickBase()) + 1.1f;
				}
			}

			if (!ctx.air_stuck) {
				const auto bk = ctx.m_local()->m_flThirdpersonRecoil();

				const auto movestate = ctx.m_local()->m_iMoveState();
				const auto iswalking = ctx.m_local()->m_bIsWalking();

				ctx.m_local()->m_iMoveState() = 0;
				ctx.m_local()->m_bIsWalking() = false;

				auto m_forward = cmd->buttons & IN_FORWARD;
				auto m_back = cmd->buttons & IN_BACK;
				auto m_right = cmd->buttons & IN_MOVERIGHT;
				auto m_left = cmd->buttons & IN_MOVELEFT;
				auto m_walking = cmd->buttons & IN_SPEED;

				bool m_walk_state = m_walking ? true : false;

				if (cmd->buttons & IN_DUCK || ctx.m_local()->m_bDucking() || ctx.m_local()->m_fFlags() & FL_DUCKING)
					m_walk_state = false;
				else if (m_walking)
				{
					float m_max_speed = ctx.m_local()->m_flMaxSpeed() * 0.52f;

					if (m_max_speed + 25.f > ctx.m_local()->m_vecVelocity().Length())
						ctx.m_local()->m_bIsWalking() = true;
				}

				auto move_buttons_pressed = cmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_RUN);

				bool holding_forward_and_back;
				bool holding_right_and_left;

				if (!m_forward)
					holding_forward_and_back = false;
				else
					holding_forward_and_back = m_back;

				if (!m_right)
					holding_right_and_left = false;
				else
					holding_right_and_left = m_left;

				if (move_buttons_pressed)
				{
					if (holding_forward_and_back)
					{
						if (holding_right_and_left) // if pressing two keys you get stopped
							ctx.m_local()->m_iMoveState() = 0;
						else if (m_right || m_left)
							ctx.m_local()->m_iMoveState() = 2;
						else //none of keys pressed.
							ctx.m_local()->m_iMoveState() = 0;
					}
					else
					{
						if (holding_forward_and_back) // if pressing two keys you get stopped
							ctx.m_local()->m_iMoveState() = 0;
						else if (m_back || m_forward)
							ctx.m_local()->m_iMoveState() = 2;
						else  //none of keys pressed.
							ctx.m_local()->m_iMoveState() = 0;
					}
				}

				if (ctx.m_local()->m_iMoveState() == 2 && m_walk_state)
					ctx.m_local()->m_iMoveState() = 1;

				feature::lagcomp->update_local_animations(cmd, &send_packet);

				ctx.m_local()->m_iMoveState() = movestate;
				ctx.m_local()->m_bIsWalking() = iswalking;
				ctx.m_local()->m_flThirdpersonRecoil() = bk;
			}

			feature::anti_aim->previous_velocity = feature::anti_aim->animation_velocity;//ctx.m_local()->m_vecAbsVelocity().Length2D() > 4.f ? ctx.m_local()->m_vecAbsVelocity() : Vector::Zero;
			//ctx.m_local()->m_nTickBase() = t;
			/*if (ctx.m_local()->get_animation_state() != nullptr
				&& (ctx.m_local()->get_animation_state()->hitgr_anim || ctx.m_local()->get_animation_state()->on_ground && (ctx.m_local()->get_animation_state()->duck_amt > 0.f || ctx.m_local()->get_animation_state()->landing_duck > 0.f) && !(cmd->buttons & IN_DUCK))
				&& !(cmd->buttons & IN_JUMP)
				&& !was_jumping)
				feature::anti_aim->visual_real_angle.x = -10.f;*/

			if (send_packet) {
				ctx.breaks_lc = (ctx.m_local()->m_vecOrigin() - feature::anti_aim->sent_data.m_vecOrigin).LengthSquared() > 4096;

				feature::anti_aim->last_unchoke_time = csgo.m_globals()->curtime;
				feature::anti_aim->visual_real_angle = cmd->viewangles;
				feature::anti_aim->sent_data.store(ctx.m_local()->m_flSimulationTime(), ctx.m_local()->m_vecOrigin(), ctx.m_local()->m_vecVelocity(), ctx.m_eye_position, cmd->viewangles, ctx.m_local()->m_nTickBase(), cmd->command_number, ctx.m_local()->m_fFlags());

				//feature::usercmd->command_numbers.clear();
				feature::anti_aim->did_shot_in_chocked_cycle = false;
				ctx.last_sent_tick = csgo.m_globals()->tickcount;
				ctx.hold_aim = false;

				//feature::usercmd->command_numbers.clear();
				feature::usercmd->cmd_info[cmd->command_number % 150].command_number = 0;
			}
			
			if (!send_packet)
				ctx.skip_communication = false;

			if (!skip_fakelags)
				ApplyShift(cmd, &send_packet);

			ctx.force_next_packet_choke = false;

			if (send_packet && ctx.m_local()->m_fFlags() & FL_ONGROUND && ctx.m_local()->m_MoveType() != MOVETYPE_LADDER && ctx.m_settings.slidewalk)
			{
				//Desync the feet for other players
			
				if (cmd->forwardmove < 0.0f)
					cmd->buttons |= IN_FORWARD;
				else if (cmd->forwardmove > 0.0f)
					cmd->buttons |= IN_BACK;
			
				if (cmd->sidemove < 0.0f)
					cmd->buttons |= IN_MOVERIGHT;
				else if (cmd->sidemove > 0.0f)
					cmd->buttons |= IN_MOVELEFT;
			}
		}
		else
			prediction->m_tickbase_array[cmd->command_number % 150].tickbase_original = ctx.m_local()->m_nTickBase();

		ctx.m_settings.aimbot_enabled = aim_toggled;

		prediction->End();

		ctx.previous_tickcount = ctx.current_tickcount;
		ctx.previous_buttons = cmd->buttons;

		/*static auto previous_org = ctx.m_local()->m_vecOrigin();
		if (send_packet)
		{
			if (GetAsyncKeyState('V'))
			{
				auto choke = TICKS_TO_TIME(csgo.m_client_state()->m_iChockedCommands + 1);
				auto velocity = (ctx.m_local()->m_vecOrigin() - previous_org) / choke;
				printf("%.2f calc | %.2f real | %.2f diff\n", velocity.Length(), ctx.m_local()->m_vecVelocity().Length(),
					std::fabsf(velocity.Length() - ctx.m_local()->m_vecVelocity().Length()));
			}

			previous_org = ctx.m_local()->m_vecOrigin();
		}*/

		ctx.fix_modify_eye_pos = false;

		if (ctx.m_local() != nullptr) {
			ctx.m_corrections_data.emplace_front(ctx.m_local()->m_nTickBase(), ctx.doubletap_now ? 13 : ctx.shift_amount, cmd->command_number, csgo.m_client_state()->m_clockdrift_manager.m_nServerTick);
			ctx.local_spawntime = ctx.m_local()->m_flSpawnTime();
		}

		while (int(ctx.m_corrections_data.size()) > int(1.0f / csgo.m_globals()->interval_per_tick)) {
			ctx.m_corrections_data.pop_back();
		}

		if (send_packet)
		{
			auto cmd_num = csgo.m_client_state()->m_iLastOutgoingCommand + 1;
			auto v84 = cmd->command_number;
			if (v84 >= cmd_num && (v84 - cmd_num) <= 150)
			{
				do
				{
					auto& v85 = Engine::Prediction::Instance()->m_tick_history[cmd_num % 150];
					v85.command_num = cmd_num++;
					v85.m_tick = csgo.m_engine()->GetTick();//csgo.m_globals()->tickcount;//csgo.m_client_state()->m_clockdrift_manager.m_nServerTick;
				} while (cmd_num <= v84);
			}
		}

		//if (send_packet)
		//{
		//	ctx.command_numbers.push_front(cmd->command_number);

		
		auto& out = ctx.command_numbers.emplace_back();
		out.is_outgoing = send_packet;
		out.command_nr = cmd->command_number;
		out.is_used = false;
		out.prev_command_nr = /*ctx.command_numbers.size() > 1 ? ctx.command_numbers.at(1).command_nr : */0;

		while (int(ctx.command_numbers.size()) > int(1.0f / csgo.m_globals()->interval_per_tick)) {
			ctx.command_numbers.pop_front();
		}
		//}
		
		/*if (ctx.air_stuck) {
			cmd->tick_count = 0x7FFFFFFF;
			cmd->command_number = 0x7FFFFFFF;
		}*/

		//if (!send_packet && ctx.m_settings.aimbot_enabled)// && (!ctx.exploit_allowed || !ctx.has_exploit_toggled || ctx.ticks_allowed < 13 && ctx.allow_shooting <= 0 || ctx.fakeducking))
		//{
		//	if (csgo.m_client_state() != nullptr /*&& (!csgo.m_game_rules() || !csgo.m_game_rules()->IsValveDS())*/) {
		//		INetChannel* net_channel = (INetChannel*)(csgo.m_client_state()->m_ptrNetChannel);
		//		if (net_channel != nullptr && net_channel->choked_packets > 0 && !(net_channel->choked_packets % 4)) {
		//			const auto current_choke = net_channel->choked_packets;
		//			net_channel->choked_packets = 0;
		//			const auto v4 = net_channel->send_datagram(0);
		//			//ctx.fix_senddatagram[v4] = v4;
		//			--net_channel->out_sequence_nr;
		//			net_channel->choked_packets = current_choke;
		//		}
		//	}
		//}

		*reinterpret_cast<bool*>(*reinterpret_cast<uintptr_t*>(static_cast<char*>(_AddressOfReturnAddress()) - 0x4) - 0x1C) = send_packet;

#ifdef VIRTUALIZER
#endif // VIRTUALIZER

		return false;
	}
}