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
#include "UI.hpp"

TEM tem = {};

auto forcedWindowMinimizePatch = Memory::Patch();

auto patch_forced_window_minimize() -> void;
auto hook_process_event() -> void;

DECL_DETOUR_T(void, ProcessEvent, UObject* object, UFunction* func, void* params, int result);
DECL_DETOUR_T(Color*, GetTeamColor, PgTeamInfo* team, Color* color, int team_color_index);
DECL_DETOUR_T(FString*, ConsoleCommand, UGameViewportClient* client, const FString& output, const FString& command);

auto tem_attach(HMODULE module) -> int
{
    if (tem.is_attached) {
        return 0;
    }

    tem.module_handle = module;

    console = new Console();
    console->Init();
    console->Println("[tem] Initializing...");

    patch_gfwl();
    patch_forced_window_minimize();
    hook_process_event();

    tem.ui_init_thread = CreateThread(0, 0, LPTHREAD_START_ROUTINE(init_ui), 0, 0, 0);

    console->Println(TEM_WELCOME);
    console->Println(TEM_VERSION);

    tem.is_attached = true;

    return 0;
}

auto tem_detach() -> void
{
    if (tem.is_detached) {
        return;
    }

    console->Println("[tem] Shutdown module {}", uintptr_t(tem.module_handle));

    ui.is_shutdown = true;
    WaitForSingleObject(tem.ui_init_thread, 2000);

    destroy_ui();

    if (forcedWindowMinimizePatch.Restore()) {
        console->Println("[tem] Restored forced window minimize patch");
    }

    MH_UNHOOK(ProcessEvent);
    console->Println("[tem] Unhooked UEngine::ProcessEvent");
    MH_UNHOOK(GetTeamColor);
    console->Println("[tem] Unhooked PgTeamInfo::GetTeamColor");
    MH_UNHOOK(ConsoleCommand);
    console->Println("[tem] Unhooked UGameViewportClient::ConsoleCommand");

    auto controller = tem.player_controller();
    if (tem.is_super_user && controller) {
        controller->set_god_mode(false);
    }

    unpatch_gfwl();

    console->Println("Cya :^)");
    console->Shutdown();

    tem.is_detached = true;
}

auto shutdown_thread() -> void { FreeLibraryAndExitThread(tem.module_handle, 0); }

auto patch_forced_window_minimize() -> void
{
    // Skip call to user32!ShowWindow with SW_MINIMIZE
    //     Original: 0F 84 (jz)
    //     Patch:    90 E9 (nop jmp)

    unsigned char nop_jmp[2] = { 0x90, 0xE9 };
    if (forcedWindowMinimizePatch.Execute(Offsets::forced_window_minimize, nop_jmp, sizeof(nop_jmp))) {
        console->Println(
            "[tem] Patched GridGame.exe forced window minimize at 0x{:04x}", forcedWindowMinimizePatch.GetLocation());
    } else {
        console->Println("[tem] Unable to patch forced window minimize :(");
    }
}

auto hook_process_event() -> void
{
    tem.engine = Memory::Deref<UEngine*>(Offsets::g_Engine);
    console->Println("[tem] g_Engine: 0x{:04x}", uintptr_t(tem.engine));

    auto processEventAddress = Memory::VMT(tem.engine, Offsets::ProcessEvent);
    console->Println("[tem] ProcessEvent: 0x{:04x}", processEventAddress);

    MH_HOOK(ProcessEvent, processEventAddress);
    console->Println("[tem] Hooked UEngine::ProcessEvent at 0x{:04x}", processEventAddress);

    MH_HOOK(GetTeamColor, Offsets::GetTeamColor);
    console->Println("[tem] Hooked PgTeamInfo::GetTeamColor at 0x{:04x}", Offsets::GetTeamColor);

    auto viewport_client = tem.engine->viewport_client;
    if (viewport_client) {
        auto consoleCommandAddr = Memory::VMT(viewport_client, Offsets::ConsoleCommand);
        MH_HOOK(ConsoleCommand, consoleCommandAddr);
        console->Println("[tem] Hooked UGameViewportClient::ConsoleCommand at 0x{:04x}", consoleCommandAddr);
    }
}

auto TEM::find_name(FName name) -> std::string_view
{
    auto g_Names = reinterpret_cast<TArray<FNameEntry*>*>(Offsets::g_Names);
    auto names = g_Names->data;

    for (auto i = 0u; i < g_Names->size; ++i) {
        auto item = names[i];
        if (item && item->index == i << 1 && item->index == name.index << 1 && item->name) {
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
        if (item && item->index == i << 1 && item->name && strcmp(item->name, name) == 0) {
            return item->index >> 1;
        }
    }

    return -1;
}
inline auto TEM::player_controller() -> PgPlayerController*
{
    if (!engine || !engine->get_local_player()) {
        return nullptr;
    }

    return engine->get_local_player()->actor;
}
inline auto TEM::pawn() -> PgPawn*
{
    if (!player_controller()) {
        return nullptr;
    }

    return player_controller()->pawn;
}
auto TEM::console_command(std::wstring command) -> void
{
    if (!engine || !engine->viewport_client) {
        return;
    }

    auto viewport_client = tem.engine->viewport_client;

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
        console->Println("{}::{} this = 0x{:04x} func = 0x{:04x} params = 0x{:04x}", tem.find_name(object->name),
            tem.find_name(func->name), uintptr_t(object), uintptr_t(func), uintptr_t(params));
    };

    if ((GetAsyncKeyState(VK_NUMPAD1) < 0) && ui.game_window_is_focused() && !tem.engine->is_paused()) {
        log_event();
    }
#endif

    //// TODO: Find a way to explore multiplayer maps in single player
    //if (object->is(PG_PLAYER_CONTROLLER) && func->is(SET_CAMERA_TARGET_TIMER)) {
    //    console->Println("Ignoring: PgPlayerController::SetCameraTargetTimer");
    //    return;
    //}

    if (object->is(PG_PAWN)) {
        if (func->is(POST_INIT_ANIM_TREE) && !tem.pawn()) {
            console->Println("PAWN SPAWNED 0x{:04x}", uintptr_t(object));

            auto controller = tem.player_controller();
            if (tem.is_super_user && controller) {
                controller->set_god_mode(true);
            }
        } else if (func->is(DESTROYED) && object->as<PgPawn>()->equals(tem.pawn())) {
            console->Println("PAWN DESTROYED 0x{:04x}", uintptr_t(object));
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

        console->Tick();

        auto pawn = tem.pawn();
        auto controller = tem.player_controller();

        if (pawn) {
            if (tem.is_super_user) {
                pawn->health = pawn->max_health;

                if (pawn->is_vehicle()) {
                    auto player_pawn = pawn->get_outer_pawn();
                    player_pawn->energy = player_pawn->max_energy;
                } else {
                    pawn->energy = pawn->max_energy;
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
    console->Println("[command] {}", command.str().c_str());

    return ConsoleCommand(client, output, command);
}
