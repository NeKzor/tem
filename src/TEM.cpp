/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#include "TEM.hpp"
#include "Console.hpp"
#include "Dumper.hpp"
#include "GFWL.hpp"
#include "Memory.hpp"
#include "Offsets.hpp"
#include "Platform.hpp"
#include "SDK.hpp"
#include "SpotChecks.hpp"
#include "UI.hpp"
#include <intrin.h>

TEM tem = {};

auto forcedWindowMinimizePatch = Memory::Patch();

DECL_DETOUR_T(void, ProcessEvent, UObject* object, UFunction* func, void* params, int result);
DECL_DETOUR_T(Color*, GetTeamColor, PgTeamInfo* team, Color* color, int team_color_index);
DECL_DETOUR_T(FString*, ConsoleCommand, UGameViewportClient* client, const FString& output, const FString& command);

/*
 * This gets called once the module loads.
 * Here we immediately patch GFWL and all spot checks.
 */
auto __stdcall tem_attach(HMODULE module) -> int
{
    if (tem.is_attached) {
        return 0;
    }

    tem.is_attached = true;
    tem.module_handle = module;

    println("[tem] Initializing...");

    Hooks::initialize();

    patch_gfwl();
    bypass_spot_checks();

#if _DEBUG
    patch_forced_window_minimize(true);
#endif

    // Actual intialization happens in a hooked GFWL function.
    // This check here is only for DLL injection.
    if (tem.engine()) {
        tem_init();
    } else {
        Hooks::apply_queued();
    }

    println(TEM_WELCOME);
    println(TEM_VERSION);

    return 0;
}

/*
 * This gets called once the module unloads.
 * Unhook all functions and restore all patches.
 */
auto tem_detach() -> void
{
    if (tem.is_detached) {
        return;
    }

    tem.is_detached = true;
    ui.is_shutdown = true;

    println("[tem] Shutdown module {}", uintptr_t(tem.module_handle));

    Hooks::uninitialize();

    ui_shutdown();
    patch_forced_window_minimize(false);

    auto controller = tem.player_controller();
    if (tem.is_super_user && controller) {
        controller->set_god_mode(false);
    }

    unpatch_gfwl();

    println("Cya :^)");
}

/*
 * This intializes all engine hooks and the UI.
 * This should be called once the pointers for the engine and the D3D
 * device are set. The GFWL method XHVCreateEngine is perfect for this.
 */
auto tem_init() -> void
{
    if (tem.is_hooked) {
        return;
    }

    auto engine = tem.engine();
    println("[tem] g_Engine: 0x{:x}", uintptr_t(engine));

    if (!engine) {
        return println("[tem] Unable to get engine :(");
    }

    auto processEvent = Memory::VMT(engine, Offsets::ProcessEvent);

    Hooks::queue("UEngine::ProcessEvent", &ProcessEvent, ProcessEvent_Hook, processEvent);
    Hooks::queue("PgTeamInfo::GetTeamColor", &GetTeamColor, GetTeamColor_Hook, Offsets::GetTeamColor);

    auto viewport_client = engine->viewport_client;
    if (viewport_client) {
        auto consoleCommand = Memory::VMT(viewport_client, Offsets::ConsoleCommand);
        Hooks::queue("UGameViewportClient::ConsoleCommand", &ConsoleCommand, ConsoleCommand_Hook, consoleCommand);
    }

    ui_init();

    Hooks::apply_queued();

    tem.is_hooked = true;
}

/*
 * This signals the process to unload the module.
 */
auto __stdcall tem_shutdown() -> void { FreeLibraryAndExitThread(tem.module_handle, 0); }

auto patch_forced_window_minimize(bool enable) -> void
{
    // Skip call to user32!ShowWindow with SW_MINIMIZE
    //     Original: 0F 84 (jz)
    //     Patch:    90 E9 (nop jmp)

    if (enable) {
        unsigned char nop_jmp[2] = { 0x90, 0xE9 };
        if (!forcedWindowMinimizePatch.Execute(Offsets::forced_window_minimize, nop_jmp, sizeof(nop_jmp))) {
            tem.disabled_forced_window_minimize = false;
            return println("[tem] Unable to patch forced window minimize :(");
        }

        tem.disabled_forced_window_minimize = true;

        println(
                "[tem] Patched GridGame.exe forced window minimize at 0x{:04x}", forcedWindowMinimizePatch.GetLocation());
    } else {
        if (!forcedWindowMinimizePatch.Restore()) {
            return println("[tem] Unable to restore forced window minimize :(");
        }

        tem.disabled_forced_window_minimize = false;
        println("[tem] Restored forced window minimize patch");
    }
}

auto TEM::find_name(FName name) -> std::string_view
{
    auto g_Names = reinterpret_cast<TArray<FNameEntry*>*>(Offsets::g_Names);
    auto names = g_Names->data;

    for (auto i = 0u; i < g_Names->size; ++i) {
        auto item = names[i];
        if (item && item->index == i << 1 && item->index == name.index << 1) {
            return item->name;
        }
    }

    return "";
}
auto TEM::find_name_index(const char* name) -> int
{
    auto g_Names = reinterpret_cast<TArray<FNameEntry*>*>(Offsets::g_Names);
    auto names = g_Names->data;

    for (auto i = 0u; i < g_Names->size; ++i) {
        auto item = names[i];
        if (item && item->index == i << 1 && strcmp(item->name, name) == 0) {
            return item->index >> 1;
        }
    }

    return -1;
}
auto TEM::console_command(std::wstring command) -> void
{
    if (!this->engine() || !this->engine()->viewport_client) {
        return;
    }

    auto viewport_client = this->engine()->viewport_client;

    using _ConsoleCommand
        = FString*(__thiscall*)(UGameViewportClient * client, const FString& output, const FString& command);
    auto ConsoleCommand = Memory::VMT<_ConsoleCommand>(viewport_client, Offsets::ConsoleCommand);

    auto output = FString(L"");
    auto input = FString(command.c_str());
    ConsoleCommand(viewport_client, output, input);
}

DETOUR_T(void, ProcessEvent, UObject* object, UFunction* func, void* params, int result)
{
#if 0
    auto log_event = [&]() {
        println("{}::{} this = 0x{:04x} func = 0x{:04x} params = 0x{:04x}", tem.find_name(object->name),
            tem.find_name(func->name), uintptr_t(object), uintptr_t(func), uintptr_t(params));
    };

    if ((GetAsyncKeyState(VK_NUMPAD1) < 0) && ui.game_window_is_focused() && tem.engine() && !tem.engine()->is_paused()) {
        log_event();
    }
#endif

    //// TODO: Find a way to explore multiplayer maps in single player
    //if (object->is(PG_PLAYER_CONTROLLER) && func->is(SET_CAMERA_TARGET_TIMER)) {
    //    println("Ignoring: PgPlayerController::SetCameraTargetTimer");
    //    return;
    //}

    if (object->is(PG_PAWN)) {
        if (func->is(POST_INIT_ANIM_TREE) && !tem.pawn()) {
            println("PAWN SPAWNED 0x{:04x}", uintptr_t(object));

            auto controller = tem.player_controller();
            if (tem.is_super_user && controller) {
                controller->set_god_mode(true);
            }
        } else if (func->is(DESTROYED) && object->as<PgPawn>()->equals(tem.pawn())) {
            println("PAWN DESTROYED 0x{:04x}", uintptr_t(object));
        }
#if 0
        if (GetAsyncKeyState(VK_NUMPAD2) < 0) {
            log_event();
        }
#endif
    } else if (object->is(CONSOLE) && func->is(TICK)) {
        auto delta = GetTickCount64() - tem.last_tick;
        tem.tickrate = delta != 0 ? 1.0f / (delta / 1'0000.0f) : 0.0f;
        tem.last_tick = GetTickCount64();

        auto pawn = tem.pawn();
        auto controller = tem.player_controller();

        if (pawn) {
            if (tem.is_super_user) {
                pawn->health = pawn->max_health;

                if (pawn->is_vehicle()) {
                    auto player_pawn = pawn->get_outer_pawn();
                    player_pawn->energy = player_pawn->max_energy;
                    player_pawn->powerup_attacking_damage_scaling = 999.0f;
                } else {
                    pawn->energy = pawn->max_energy;
                    pawn->powerup_attacking_damage_scaling = 999.0f;
                }

                if (controller) {
                    controller->set_god_mode(true);
                }
            }

            //if (GetAsyncKeyState('N') & 1) {
            //    tem.is_noclipping = !tem.is_noclipping;
            //}

#if 0
            const auto noclip_units = 10;

            if (GetAsyncKeyState(VK_NUMPAD7) < 0) {
                player->position.x += noclip_units;
            }
            if (GetAsyncKeyState(VK_NUMPAD4) < 0) {
                player->position.x -= noclip_units;
            }
            if (GetAsyncKeyState(VK_NUMPAD8) < 0) {
                player->position.y += noclip_units;
            }
            if (GetAsyncKeyState(VK_NUMPAD5) < 0) {
                player->position.y -= noclip_units;
            }
            if (GetAsyncKeyState(VK_NUMPAD9) < 0) {
                player->position.z += noclip_units;
            }
            if (GetAsyncKeyState(VK_NUMPAD6) < 0) {
                player->position.z -= noclip_units;
            }
#endif
        }

        if (tem.want_weak_enemies && controller && controller->enemy) {
            controller->enemy->health = 1;
        }
    }

    ProcessEvent(object, func, params, result);
}

DETOUR_T(Color*, GetTeamColor, PgTeamInfo* team, Color* color, int team_color_index)
{
    if (tem.want_rgb_suit && team_color_index == -1) {
        auto pawn = tem.pawn();

        if (pawn && pawn->is_in_team(team)) {
            if (tem.rgb_last_update + tem.rgb_update_interval < pawn->timer) {
                tem.rgb_last_update = pawn->timer;
                tem.rgb_last_color = (tem.rgb_last_color + 1) < max_rainbow_colors ? tem.rgb_last_color + 1 : 0;
            }

            *color = rainbow_colors[tem.rgb_last_color];

            return color;
        }
    }

    return GetTeamColor(team, color, team_color_index);
}

DETOUR_T(FString*, ConsoleCommand, UGameViewportClient* client, const FString& output, const FString& command)
{
    println("[command] [{:x}] {}", uintptr_t(__builtin_extract_return_addr(__builtin_return_address(0))), command.str().c_str());

    return ConsoleCommand(client, output, command);
}
