#include "ImGuiImpl.h"
#include "Cvar.h"

#include "chaos_commands.h"
#include "chaos_commands_macros.h"
#include "chaos_utils.h"

#include <z64.h>
#include <variables.h>
#undef Polygon

#include <functional>
#include <map>
#include <vector>
#include <queue>
#include <ctime>
#include <string>

#if defined(__linux__)
#include "chaos_linux.h"
#elif !defined(__APPLE__)
#include "chaos_win.h"
#endif

bool g_link_is_ready_this_frame = true;

template<typename T>
T Read(const std::vector<uint8_t>& bytes, size_t start_index) {
	return *((T*)(bytes.data() + start_index));
}

struct CommandCreator {
	std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)> read_payload_;
	std::function<std::unique_ptr<ChaosCommand>(const std::vector<uint8_t>&)> create_;
};

uint8_t CMD_ID = 0x11;
static std::map<uint8_t, CommandCreator> kCommands {
	CMD_ONE_SHOT(0x00, PL_NONE(), { push_pending_ice_trap(); }),

	CMD(0x01, PL_NONE(), 
		CR_PRED(
			[&]() { return g_link_is_ready_this_frame; },
			CR_ONE_SHOT({ void_out(); }))),

	// Don't change age until there are no force-equipped boots so that we don't got to adult,
	// immediately try to equip F boots, and crash
	CMD(0x02, PL_NONE(), 
		CR_PRED(
			[&]() { return g_link_is_ready_this_frame && !CVar_GetS32("gChaosForcedBoots", 0); },
			CR_ONE_SHOT({ toggle_age(); }))),

	CMD_ONE_SHOT(0x03, PL_NONE(), { gSaveContext.health = 0; }),
	CMD_ONE_SHOT(0x04, PL_NONE(), { scale(&(GET_PLAYER(gGlobalCtx)->actor), 2.f, 2.f, 2.f); }),
	CMD_ONE_SHOT(0x05, PL_NONE(), { scale(&(GET_PLAYER(gGlobalCtx)->actor), 0.5f, 0.5f, 0.5f); }),

	CMD_TIMED_BOOL_CVAR(0x06, "gChaosOHKO"),
	CMD_TIMED_BOOL_CVAR(0x07, "gChaosNoHud"),
	CMD_TIMED_BOOL_CVAR(0x08, "gChaosNoZ"),
	CMD_TIMED_BOOL_CVAR(0x09, "gChaosTurbo"),
	CMD_TIMED_BOOL_CVAR(0x0A, "gChaosInvertControls"),

	CMD(0x0B, PL_NONE(), 
		CR_PRED(
			[&]() { return g_link_is_ready_this_frame; },
			CR_ONE_SHOT({ spawn_on_link(0x013B, 0x0000); }))),

	CMD(0x0C, PL_BYTES(2 * sizeof(int16_t)), 
		CR_PRED(
			[&]() { return g_link_is_ready_this_frame; },
			CR_ONE_SHOT({ spawn_on_link(
				Read<int16_t>(payload, 0),
				Read<int16_t>(payload, sizeof(int16_t))); 
	}))),

	CMD_ONE_SHOT(0x0D, PL_BYTES(sizeof(uint32_t)), { 
		gSaveContext.health = s_add(gSaveContext.health, Read<uint32_t>(payload, 0), gSaveContext.healthCapacity); 
	}),
	CMD_ONE_SHOT(0x0E, PL_BYTES(sizeof(uint32_t)), { 
		gSaveContext.health = s_sub(gSaveContext.health, Read<uint32_t>(payload, 0), 16); 
	}),

	CMD_ONE_SHOT(0x0F, PL_BYTES(sizeof(uint32_t)), { 
		gSaveContext.rupees = s_add(gSaveContext.rupees, Read<uint32_t>(payload, 0), CUR_CAPACITY(UPG_WALLET)); 
	}),
	CMD_ONE_SHOT(0x10, PL_BYTES(sizeof(uint32_t)), { 
		gSaveContext.rupees = s_sub(gSaveContext.rupees, Read<uint32_t>(payload, 0), 16); 
	}),

	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gEnemyHealthBar"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gDisableFPSView"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gForceNormalArrows"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gDisableLedgeClimb"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gFloorIsLava"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gExplodingRolls"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gFreezingRolls"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gDisableTargeting"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gMegaLetterbox"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gDisableTurning"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gJailTime"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gOnHold"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gSonicRoll"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gNaviSpam"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gScuffedLink"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gRaveMode"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gInvisPlayer"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gSlipperyFloor"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gIceDamage"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gElectricDamage"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gKnockbackDamage"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gFireDamage"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gForwardJump"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gBigHead"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gTinyHead"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gDarkenArea"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gChaosSpin"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gDisableMeleeAttacks"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gDisableEnemyDraw"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gSandstorm"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gSinkingFloor"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gCowRitual"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gFireRockRain"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gCuccoAttack"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gExplodingRupeeChallenge"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gBanItemDropPickup"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gBrokenBombchus"),
	CMD_TIMED_BOOL_CVAR(CMD_ID++, "gAnnoyingGetItems"),

	// Gravity (- down, + up)
	CMD(CMD_ID++, PL_NONE(), 
		CR_PRED(
			[]() { return !CVar_GetS32("gPlayerGravity", 0); },
			CR_TIMED_CVAR("gPlayerGravity", 0, 9))),
	CMD(CMD_ID++, PL_NONE(), 
		CR_PRED(
			[]() { return !CVar_GetS32("gPlayerGravity", 0); },
			CR_TIMED_CVAR("gPlayerGravity", 0, -12))),

	// Climb speed (slow)
	CMD(CMD_ID++, PL_NONE(), 
		CR_PRED(
			[]() { return !CVar_GetS32("gChaosClimbSpeed", 0); },
			CR_TIMED_CVAR("gChaosClimbSpeed", 0, 1))),

	// HS Length (short)
	CMD(CMD_ID++, PL_NONE(), 
		CR_PRED(
			[]() { return !CVar_GetS32("gHookshotLengthRemove", 0); },
			CR_TIMED_CVAR("gHookshotLengthRemove", 0, 3))),

	CMD_ONE_SHOT_CVAR(CMD_ID++, "gSpawnExplosion"),
	CMD_ONE_SHOT_CVAR(CMD_ID++, "gRestrainLink"),
	CMD_ONE_SHOT_CVAR(CMD_ID++, "gTripToSpace"),

	CMD_TAKE_AMMO(0x80, ITEM_BOMBCHU),
	CMD_TAKE_AMMO(0x81, ITEM_STICK),
	CMD_TAKE_AMMO(0x82, ITEM_NUT),
	CMD_TAKE_AMMO(0x83, ITEM_BOMB),
	CMD_TAKE_AMMO(0x84, ITEM_BOW),
	CMD_TAKE_AMMO(0x85, ITEM_SLINGSHOT),

	// You can carry 50 chus if you can carry bombs, 0 otherwise
	CMD_ONE_SHOT(0xC0, PL_BYTES(sizeof(uint32_t)), { 
		uint32_t amt = Read<uint32_t>(payload, 0);
		auto cap = CUR_CAPACITY(UPG_BOMB_BAG) > 0 ? 50 : 0;
		AMMO(ITEM_BOMBCHU) = s_add(AMMO(ITEM_BOMBCHU), amt, cap);
	}),
	CMD_GIVE_AMMO(0xC1, ITEM_STICK, UPG_STICKS),
	CMD_GIVE_AMMO(0xC2, ITEM_NUT, UPG_NUTS),
	CMD_GIVE_AMMO(0xC3, ITEM_BOMB, UPG_BOMB_BAG),
	CMD_GIVE_AMMO(0xC4, ITEM_BOW, UPG_QUIVER),
	CMD_GIVE_AMMO(0xC5, ITEM_SLINGSHOT, UPG_BULLET_BAG),

	CMD(0xE2, PL_BYTES(sizeof(uint32_t)), 
		CR_PRED(
			[]() { return LINK_IS_ADULT && !CVar_GetS32("gChaosForcedBoots", 0); },
			CR_TIMED_CVAR("gChaosForcedBoots", 0, 2))),
	CMD(0xE3, PL_BYTES(sizeof(uint32_t)), 
		CR_PRED(
			[]() { return LINK_IS_ADULT && !CVar_GetS32("gChaosForcedBoots", 0); },
			CR_TIMED_CVAR("gChaosForcedBoots", 0, 3))),
	CMD(0xEF, PL_BYTES(sizeof(uint32_t)), 
		CR_PRED(
			[]() { return LINK_IS_CHILD && !CVar_GetS32("gChaosForcedBoots", 0); },
			CR_TIMED_CVAR("gChaosForcedBoots", 0, 0xF))),
};

static bool g_is_enabled = false;
static CommandStorage g_command_storage;

void Start() {
    PlatformStart();
	SohImGui::overlay->TextDrawNotification(10.0f, true, "Chaos Mode Enabled");
}

void DisplayCommandMessage(const std::vector<uint8_t>& bytes, size_t start_index) {
	std::string msg(bytes.begin() + start_index, bytes.end());
	SohImGui::overlay->TextDrawNotification(5.0f, true, msg.c_str());
}

void EnqueueCommand(const std::vector<uint8_t>& bytes) {
	size_t message_start_index = 1; // Account for the command idx itself

	auto it = kCommands.find(bytes[0]);
	if (it == kCommands.end()) {
		SohImGui::overlay->TextDrawNotification(10.0f, true, "Unrecognized command");
		return;
	}

	auto payload = it->second.read_payload_(bytes);
	g_command_storage.AddCommand(it->second.create_(payload));

	DisplayCommandMessage(bytes, message_start_index + payload.size());
}

bool ReadBytes(size_t num, std::vector<uint8_t>* buf) {
    return PlatformReadBytes(num, buf);
}

void Stop() {
    PlatformStop();
    SohImGui::overlay->TextDrawNotification(10.0f, true, "Chaos Mode Disabled");
}

void EachFrameCallback() {
	bool new_val = CVar_GetS32("gChaosEnabled", 0);
	if (g_is_enabled && !new_val) {
		// TODO: disable? close pipe?
	}

	if (!g_is_enabled && new_val) {
		Start();
	}

	g_is_enabled = new_val;

	if (!g_is_enabled) return;

	// Try to read a command from the buffer. If there isn't one, try again next frame.
	// First byte is the size of the command, in bytes (excluding the size byte).
	std::vector<uint8_t> buf(1);
	if (ReadBytes(1, &buf)){
		std::vector<uint8_t> current_command_buffer;
		size_t bytes_to_read = buf[0];
		current_command_buffer.resize(bytes_to_read);

		if (!ReadBytes(bytes_to_read, &current_command_buffer)) {
			std::string msg = "Error reading command, turning off Chaos Mode";
			SohImGui::overlay->TextDrawNotification(10.0f, true, msg.c_str());
            Stop();
			CVar_SetS32("gChaosEnabled", 0);
			g_is_enabled = false;
			return;
		}

		EnqueueCommand(current_command_buffer);
	}

	g_link_is_ready_this_frame = link_is_ready();
	if (g_link_is_ready_this_frame && ice_trap_is_pending()) {
		give_ice_trap();
		g_link_is_ready_this_frame = false;
	}

	g_command_storage.Tick();

	apply_ongoing_effects();
}

extern "C" {
	void Chaos_Init() {
    	CVar_RegisterS32("gChaosEnabled", 0);

    	CVar_RegisterS32("gChaosForcedBoots", 0);
    	CVar_RegisterS32("gChaosOHKO", 0);
    	CVar_RegisterS32("gChaosNoHud", 0);
    	CVar_RegisterS32("gChaosNoZ", 0);
    	CVar_RegisterS32("gChaosTurbo", 0);
    	CVar_RegisterS32("gChaosInvertControls", 0);
	}

	void Chaos_EachFrame() {
		EachFrameCallback();
	}
}

