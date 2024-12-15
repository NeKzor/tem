/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include "Memory.hpp"
#include "SDK.hpp"
#include <thread>
#include <map>

#define TEM_WELCOME "Tron Evolution Mod by NeKz :^)"
#define TEM_VERSION "Version 0.1.0 (" __TIMESTAMP__  ")"

#define MV_FORWARD (1 << 3)
#define MV_BACKWARD (1 << 4)
#define MV_LEFT (1 << 5)
#define MV_RIGHT (1 << 6)
#define MV_JUMP (1 << 7)
#define MV_DISC_ATTACK (1 << 8)
#define MV_SPRINT (1 << 9)
#define MV_DISC_POWER (1 << 10)
#define MV_MELEE_ATTACK (1 << 11)
#define MV_BLOCK (1 << 12)
#define MV_CAMERA_RESET (1 << 13)
#define MV_SWITCH_TO_HEAVY_DISC (1 << 14)
#define MV_SWITCH_TO_BOMB_DISC (1 << 15)
#define MV_SWITCH_TO_STASIS_DISC (1 << 16)
#define MV_SWITCH_TO_CORRUPTION_DISC (1 << 17)

struct TEM {
    HMODULE module_handle = 0;

    std::atomic_bool is_attached = false;
    std::atomic_bool is_hooked = false;
    std::atomic_bool is_detached = false;

    float tickrate = 0.0f;
    ULONGLONG last_tick = 0;

    bool is_super_user = false;
    bool want_weak_enemies = false;
    bool want_rgb_suit = false;
    bool is_noclipping = false;
    bool disabled_forced_window_minimize = false;

    int rgb_last_color = 0;
    float rgb_last_update = 0.0f;
    const float rgb_update_interval = 0.5f;

    auto find_name(FName name) -> std::string_view;
    auto find_name_index(const char* name) -> int;

    inline auto engine() -> UEngine*;
    inline auto player_controller() -> PgPlayerController*;
    inline auto pawn() -> PgPawn*;
    auto console_command(std::wstring command) -> void;

    std::map<std::string, std::pair<int, std::vector<int>>> command_to_key_move = {
        { "Axis aBaseY Speed=1.0", { MV_FORWARD, {} } }, // W
        { "Axis aStrafe Speed=-1.0 | ForceWallJump", { MV_LEFT, {} } }, // A
        { "Axis aBaseY Speed=-1.0", { MV_BACKWARD, {} } }, // S
        { "Axis aStrafe Speed=1.0 | ForceWallJump", { MV_RIGHT, {} } }, // D
        { "GB_Y", { MV_DISC_POWER, {} } }, // F
        { "GB_RightThumbstick", { MV_CAMERA_RESET, {} } }, // C
        { "GB_RightTrigger", { MV_SPRINT, {} } }, // LeftShift
        { "GB_LeftTrigger", { MV_BLOCK, {} } }, // LeftControl
        { "GB_A", { MV_JUMP, {} } }, // SpaceBar
        { "GB_X", { MV_DISC_ATTACK, {} } }, // LeftMouseButton
        { "GB_B | CANCELMATINEE", { MV_MELEE_ATTACK, {} } }, // RightMouseButton
        { "GB_DPad_Left | SwitchToPower HeavyDiscPower_INV", { MV_SWITCH_TO_HEAVY_DISC, {} } }, // 1
        { "GB_DPad_Right | SwitchToPower BombDiscPower_INV", { MV_SWITCH_TO_BOMB_DISC, {} } }, // 2
        { "GB_DPad_Up | SwitchToPower StasisDiscPower_INV", { MV_SWITCH_TO_STASIS_DISC, {} } }, // 3
        { "GB_DPad_Down | SwitchToPower CorruptionDiscPower_INV", { MV_SWITCH_TO_CORRUPTION_DISC, {} } }, // 4
    };
};

extern TEM tem;

extern auto tem_attach(HMODULE module) -> int;
extern auto tem_detach() -> void;
extern auto tem_init() -> void;
extern auto tem_shutdown() -> void;

extern auto patch_forced_window_minimize(bool enable) -> void;

// TODO: somebody come up with better level names
const std::vector<std::pair<std::string, std::string>> game_levels = {
    /* Chapter 1 - Reboot */
    { "1_01", "Ceremony" },
    { "1_03", "Clearing" },
    /* Chapter 2 - Shutdown */
    { "1_04", "Shutdown" },
    /* Chapter 3 - Arjia */
    { "2_01", "Arjia City" },
    { "2_02", "Escape" },
    /* Chapter 4 - The Combat */
    { "2_03", "Game Grid" },
    { "2_04", "Rescue" },
    /* Chapter 5 - Identification, Friend or Foe */
    { "2_05", "Kernel" },
    { "2_06", "Abraxas" },
    { "2_07", "Gibson" },
    { "2_08", "Outlands" },
    /* Chapter 6 - The Approach */
    { "2_09", "Blockade" },
    { "2_10", "Virus" },
    /* Chapter 7 - End of Line */
    { "3_01", "Regulator" },
    { "3_02", "Quorra" },
    { "3_03", "Finale" },
    /* Menu */
    { "FE_Main", "Main Menu" },
    /* Multiplayer */
    { "MP_CMI_C1_BIT", "Heat Sink" },
    { "MP_CMI_C2_BIT", "Hard Disc" },
    { "MP_CMI_C2_BIT", "Defrag" },
    { "MP_CMI_V1_BIT", "Circuit Board" },
    { "MP_CMI_V2_BIT", "User Plaza" },
    { "MP_CMI_V4_BIT", "Codestream Nexus" },
};

const Color rainbow_colors[] = {
    Color(255, 0, 0), // red
    Color(255, 12, 0),
    Color(255, 23, 0),
    Color(255, 35, 0),
    Color(255, 46, 0),
    Color(255, 58, 0),
    Color(255, 69, 0),
    Color(255, 81, 0),
    Color(255, 92, 0),
    Color(255, 104, 0),
    Color(255, 115, 0),
    Color(255, 127, 0), // orange
    Color(255, 139, 0),
    Color(255, 150, 0),
    Color(255, 162, 0),
    Color(255, 174, 0),
    Color(255, 185, 0),
    Color(255, 197, 0),
    Color(255, 208, 0),
    Color(255, 220, 0),
    Color(255, 232, 0),
    Color(255, 243, 0),
    Color(255, 255, 0), // yellow
    Color(232, 255, 0),
    Color(209, 255, 0),
    Color(185, 255, 0),
    Color(162, 255, 0),
    Color(139, 255, 0),
    Color(116, 255, 0),
    Color(93, 255, 0),
    Color(70, 255, 0),
    Color(46, 255, 0),
    Color(23, 255, 0),
    Color(0, 255, 0), // green
    Color(0, 232, 23),
    Color(0, 209, 46),
    Color(0, 185, 70),
    Color(0, 162, 93),
    Color(0, 139, 116),
    Color(0, 116, 139),
    Color(0, 93, 162),
    Color(0, 70, 185),
    Color(0, 46, 209),
    Color(0, 23, 232),
    Color(0, 0, 255), // blue
    Color(7, 0, 244),
    Color(14, 0, 232),
    Color(20, 0, 221),
    Color(27, 0, 210),
    Color(34, 0, 198),
    Color(41, 0, 187),
    Color(48, 0, 175),
    Color(55, 0, 164),
    Color(61, 0, 153),
    Color(68, 0, 141),
    Color(75, 0, 130), // indigo
    Color(82, 0, 137),
    Color(88, 0, 145),
    Color(95, 0, 152),
    Color(102, 0, 159),
    Color(108, 0, 167),
    Color(115, 0, 174),
    Color(121, 0, 182),
    Color(128, 0, 189),
    Color(135, 0, 196),
    Color(141, 0, 204),
    Color(148, 0, 211), // violet
    Color(158, 0, 192),
    Color(167, 0, 173),
    Color(177, 0, 153),
    Color(187, 0, 134),
    Color(197, 0, 115),
    Color(206, 0, 96),
    Color(216, 0, 77),
    Color(226, 0, 58),
    Color(236, 0, 38),
    Color(245, 0, 19),
};

const int max_rainbow_colors = sizeof(rainbow_colors) / sizeof(rainbow_colors[0]);
