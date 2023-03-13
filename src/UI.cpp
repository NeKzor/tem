/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#include "UI.hpp"
#include "Console.hpp"
#include "Dumper.hpp"
#include "GFWL.hpp"
#include "Memory.hpp"
#include "Offsets.hpp"
#include "Platform.hpp"
#include "TEM.hpp"
#include "lib/imgui/imgui.h"
#include "lib/imgui/imgui_impl_dx9.h"
#include "lib/imgui/imgui_impl_win32.h"
#include <Windows.h>
#include <d3d9.h>
#include <d3dcompiler.h>
#include <format>
#include <fstream>
#include <map>
#include <tchar.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

UI ui = {};

struct URenderer {
    uintptr_t vtable; // 0
    IDirect3DDevice9** device; // 4
};

DECL_DETOUR_STD(HRESULT, Reset, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters);
DECL_DETOUR_STD(HRESULT, Present, IDirect3DDevice9* device, RECT* pSourceRect, RECT* pDestRect,
    HWND hDestWindowOverride, RGNDATA* pDirtyRegion);

BOOL CALLBACK enum_window(HWND handle, LPARAM pid)
{
    auto proc_id = DWORD();
    GetWindowThreadProcessId(handle, &proc_id);

    if (proc_id != pid) {
        return true;
    }

    char title[256];
    GetWindowTextA(handle, title, sizeof(title));

    auto found = strcmp(title, "Tron: Evolution") == 0;
    if (found) {
        println(
            "[ui] enum_window: title = {}, proc_id = {}, handle = {}, pid = {}", title, proc_id, int(handle), int(pid));
        ui.window_handle = handle;
        return false;
    }

    return true;
}

auto init_ui() -> void
{
    //auto g_Renderer = Memory::Deref<URenderer*>(Offsets::g_Renderer);
    //println("[ui] g_Renderer: 0x{:04x}", uintptr_t(g_Renderer));

    //auto device = g_Renderer->device;
    //println("[ui] device: 0x{:04x}", uintptr_t(device));

    ui.window_handle = nullptr;

    EnumWindows(enum_window, GetCurrentProcessId());

    if (!ui.window_handle) {
        return println("[ui] Unable to find window handle :(");
    }

    auto d3d9dll = LoadLibraryA("d3d9.dll");
    if (!d3d9dll) {
        return println("[ui] Unable to find d3d9.dll module :(");
    }

    auto D3DCreate9 = GetProcAddress(d3d9dll, "Direct3DCreate9");
    if (!D3DCreate9) {
        return println("[ui] Unable to find Direct3DCreate9 :(");
    }

    typedef IDirect3D9*(__stdcall * DIRECT3DCREATE9)(UINT version);

    auto d3d9 = DIRECT3DCREATE9(D3DCreate9)(D3D_SDK_VERSION);
    if (!d3d9) {
        return println("[ui] Direct3DCreate9 returns null :(");
    }

    IDirect3DDevice9* device = nullptr;

    D3DPRESENT_PARAMETERS d3dpp = {
        .SwapEffect = D3DSWAPEFFECT_DISCARD,
        .hDeviceWindow = ui.window_handle,
    };

    while (!ui.is_shutdown) {
        d3dpp.Windowed = true;

        auto res = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);

        //if (res != D3D_OK) {
        //    d3dpp.Windowed = false;
        //    res = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow,
        //        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
        //}

        if (res != D3D_OK) {
            println("[ui] CreateDevice failed with error code {}", res);
            Sleep(1000);
            continue;
        }

        if (!device) {
            println("[ui] CreateDevice did not set a device :(");
            break;
        }

        auto reset = Memory::VMT(device, Offsets::Reset);
        auto present = Memory::VMT(device, Offsets::Present);

        Hooks::hook("IDirect3DDevice9::Reset", &Reset, Reset_Hook, reset);
        Hooks::hook("IDirect3DDevice9::Present", &Present, Present_Hook, present);

        device->Release();

        ui.hooked = true;
        break;
    }
}

auto destroy_ui() -> void
{
    ui.is_shutdown = true;
    ui.hooked = false;

    if (ui.initialized) {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        println("[ui] ImGui Shutdown");

        if (ui.window_proc) {
            SetWindowLongPtr(ui.window_handle, GWLP_WNDPROC, LONG_PTR(ui.window_proc));
            println("[ui] Restored WindowLongPtr");
        }

        ui.initialized = false;
    }
}

DETOUR_STD(HRESULT, Reset, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    ImGui_ImplDX9_CreateDeviceObjects();

    return Reset(device, pPresentationParameters);
}

LRESULT STDMETHODCALLTYPE wnd_proc_handler(HWND window, UINT message_type, WPARAM w_param, LPARAM l_param)
{
    if (ui.menu && !ui.is_shutdown) {
        ImGui_ImplWin32_WndProcHandler(window, message_type, w_param, l_param);
    }

    return CallWindowProc(ui.window_proc, window, message_type, w_param, l_param);
}

auto create_hover_tooltip(const char* help_text) -> void
{
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(help_text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

DETOUR_STD(HRESULT, Present, IDirect3DDevice9* device, RECT* pSourceRect, RECT* pDestRect, HWND hDestWindowOverride,
    RGNDATA* pDirtyRegion)
{
    if (!ui.initialized && !ui.is_shutdown) {
        if (ui.window_handle != NULL) {
            ui.window_proc = WNDPROC(SetWindowLongPtr(ui.window_handle, GWLP_WNDPROC, LONG_PTR(wnd_proc_handler)));

            ImGui::CreateContext();
            auto& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ImGui_ImplDX9_Init(device);
            ImGui_ImplWin32_Init(ui.window_handle);

            println("[ui] Intialized");

            ui.initialized = true;
        }
    } else if (!ui.is_shutdown) {
        if ((GetAsyncKeyState(VK_NUMPAD0) & 1) && ui.game_window_is_focused()) {
            ui.menu = !ui.menu;
        }

        ImGui_ImplWin32_NewFrame();
        ImGui_ImplDX9_NewFrame();
        ImGui::NewFrame();

        auto x = 10.0f;
        auto y = 90.0f;
        auto padding_between_elements = 20.0f;

        if (ui.show_fps) {
            y += padding_between_elements;
            ImGui::SetNextWindowPos(ImVec2(x, y));
            ImGui::Begin("fps", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
                    | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
            ImGui::Text("fps: %.2f", ImGui::GetIO().Framerate);
            ImGui::End();
        }

        auto player = tem.pawn();
        if (player) {
            if (ui.show_timer) {
                char timer[16] = {};

                auto sec = int(std::floor(player->timer));
                auto ms = int(std::ceil((player->timer - sec) * 1000));

                if (sec >= 60) {
                    auto min = sec / 60;
                    sec = sec % 60;
                    if (min >= 60) {
                        auto hrs = min / 60;
                        min = min % 60;
                        snprintf(timer, sizeof(timer), "%i:%02i:%02i.%03i", hrs, min, sec, ms);
                    } else {
                        snprintf(timer, sizeof(timer), "%i:%02i.%03i", min, sec, ms);
                    }
                } else {
                    snprintf(timer, sizeof(timer), "%i.%03i", sec, ms);
                }

                y += padding_between_elements;
                ImGui::SetNextWindowPos(ImVec2(x, y));
                ImGui::SetNextWindowSize(ImVec2(135, 30));
                ImGui::Begin("timer", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
                        | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
                ImGui::Text("timer: %s", timer);
                ImGui::End();
            }
            if (ui.show_position) {
                y += padding_between_elements;
                ImGui::SetNextWindowPos(ImVec2(x, y));
                ImGui::SetNextWindowSize(ImVec2(265, 30));
                ImGui::Begin("position", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
                        | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
                ImGui::Text("pos: %.3f %.3f %.3f", player->position.x, player->position.y, player->position.z);
                ImGui::End();
            }
            if (ui.show_angle) {
                y += padding_between_elements;
                ImGui::SetNextWindowPos(ImVec2(x, y));
                ImGui::SetNextWindowSize(ImVec2(110, 30));
                ImGui::Begin("angle", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
                        | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
                ImGui::Text("ang: %.3f", player->rotation.degree());
                ImGui::End();
            }
            if (ui.show_velocity) {
                y += padding_between_elements;
                ImGui::SetNextWindowPos(ImVec2(x, y));
                ImGui::SetNextWindowSize(ImVec2(265, 30));
                ImGui::Begin("velocity", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
                        | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
                ImGui::Text("vel: %.3f %.3f", player->velocity.length_2d(), player->velocity.length());
                ImGui::End();
            }
            if (ui.show_health) {
                y += padding_between_elements;
                ImGui::SetNextWindowPos(ImVec2(x, y));
                ImGui::SetNextWindowSize(ImVec2(110, 30));
                ImGui::Begin("health", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
                        | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
                ImGui::Text("hp: %i", player->health);
                ImGui::End();
            }
            if (ui.show_enemy_health) {
                y += padding_between_elements;
                ImGui::SetNextWindowPos(ImVec2(x, y));
                ImGui::SetNextWindowSize(ImVec2(265, 30));
                ImGui::Begin("enemy_health", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
                        | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
                if (tem.player_controller() && tem.player_controller()->enemy) {
                    ImGui::Text("enemy hp: %i", tem.player_controller()->enemy->health);
                } else {
                    ImGui::Text("enemy hp: -");
                }
                ImGui::End();
            }
        }

        //if (tem.engine()) {
        //    y += padding_between_elements;
        //    ImGui::SetNextWindowPos(ImVec2(x, y));
        //    ImGui::SetNextWindowSize(ImVec2(200, 30));
        //    ImGui::Begin("transition", nullptr,
        //        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);
        //    auto get_transition = [&]() -> const char* {
        //        switch (tem.engine()->transition_type) {
        //        case TT_None:
        //            return "None";
        //        case TT_Paused:
        //            return "Paused";
        //        case TT_Loading:
        //            return "Loading";
        //        case TT_Saving:
        //            return "Saving";
        //        case TT_Connecting:
        //            return "Connecting";
        //        case TT_Precaching:
        //            return "Precaching";
        //        case TT_MAX:
        //            return "MAX";
        //        default:
        //            return "unknown";
        //        }
        //    };
        //    ImGui::Text("transiton: %s", get_transition());
        //    ImGui::End();
        //}

        if (ui.show_inputs && tem.player_controller() && tem.player_controller()->player_input
            && !tem.engine()->is_paused()) {
            auto mode = 3;
            auto buttons = 0;

            auto player_input = tem.player_controller()->player_input;

            foreach_item(key_name, player_input->pressed_keys)
            {
                for (auto& mappingIter : tem.command_to_key_move) {
                    auto& mapping = mappingIter.second;
                    auto& flag = mapping.first;
                    auto& key = mapping.second;

                    if (std::find(key.begin(), key.end(), key_name.index) != key.end()) {
                        buttons |= flag;
                    }
                }
            }

            auto mv_forward = buttons & MV_FORWARD;
            auto mv_backward = buttons & MV_BACKWARD;
            auto mv_left = buttons & MV_LEFT;
            auto mv_right = buttons & MV_RIGHT;
            auto mv_jump = buttons & MV_JUMP;
            auto mv_disc_attack = buttons & MV_DISC_ATTACK;
            auto mv_sprint = buttons & MV_SPRINT;
            auto mv_disc_power = buttons & MV_DISC_POWER;
            auto mv_melee_attack = buttons & MV_MELEE_ATTACK;
            auto mv_block = buttons & MV_BLOCK;

            auto size = 60.0f;
            auto padding = 2.0f;

            auto x_offset = x;
            auto y_offset = ImGui::GetIO().DisplaySize.y - (4 * size) - (3 * padding);

            auto color = IM_COL32(0, 0, 0, 235);
            auto shadow_color = IM_COL32(0, 0, 0, 100);
            auto font_color = IM_COL32(255, 255, 255, 255);
            auto font_shadow_color = IM_COL32(255, 255, 255, 64);

            static const char* symbols[] = { "W", "A", "S", "D", "S", "C", "S", "F", "L", "R" };
            auto shadow = true;

            ImGui::SetNextWindowPos(ImVec2(x, y));
            ImGui::Begin("inputs", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
                    | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
            auto draw_list = ImGui::GetWindowDrawList();

            auto element = 0;
            auto max_x = 0.0f;
            auto max_y = 0.0f;

            auto DrawElement = [x_offset, y_offset, mode, shadow, color, size, shadow_color, draw_list, padding,
                                   font_color, font_shadow_color, &element, &max_x,
                                   &max_y](int value, bool button, int col, int row, int length = 1) {
                auto x = x_offset + (col * size) + ((col + 1) * padding);
                auto y = y_offset + (row * size) + ((row + 1) * padding);
                if (mode >= value && (button || shadow)) {
                    auto x0 = x + ((col + 1) * padding);
                    auto y0 = y + ((row + 1) * padding);
                    auto x1 = x + ((((col + 1) * padding) + size) * length);
                    auto y1 = y + ((row + 1) * padding + size);

                    draw_list->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), button ? color : shadow_color);

                    auto text = symbols[element];
                    auto text_size = ImGui::CalcTextSize(text);

                    auto xc = x0 + ((x1 - x0) / 2);
                    auto yc = y0 + ((y1 - y0) / 2);

                    draw_list->AddText(ImVec2(xc - (text_size.x / 2), yc - (text_size.y / 2)),
                        button ? font_color : font_shadow_color, text);

                    if (x1 > max_x) {
                        max_x = x1;
                    }

                    if (y1 > max_y) {
                        max_y = y1;
                    }
                }
                ++element;
            };

            /*
                Layout:
                    row|col0|1|2|3|4|5|6|7|8
                    ---|---------------------
                      0|       w
                      1|shft|a|s|d|f
                      2|ctrl|spacebar   |l|r
            */

            const int row0 = 0;
            const int row1 = 1;
            const int row2 = 2;

            const int col0 = 0;
            const int col1 = 1;
            const int col2 = 2;
            const int col3 = 3;
            const int col4 = 4;
            const int col5 = 5;
            const int col6 = 6;
            const int col7 = 7;
            const int col8 = 8;

            DrawElement(1, mv_forward, col2, row0); // w
            DrawElement(1, mv_left, col1, row1); // a
            DrawElement(1, mv_backward, col2, row1); // s
            DrawElement(1, mv_right, col3, row1); // d

            DrawElement(2, mv_sprint, col0, row1); // shft
            DrawElement(2, mv_block, col0, row2); // ctrl
            DrawElement(2, mv_jump, col1, row2, col6); // spacebar
            DrawElement(2, mv_disc_power, col4, row1); // f

            DrawElement(3, mv_disc_attack, col7, row2); // l
            DrawElement(3, mv_melee_attack, col8, row2); // r

            ImGui::Dummy(ImVec2(max_x, max_y));

            ImGui::End();
        }

        if (ui.menu && ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("TEM")) {
                if (ImGui::MenuItem("Superuser", nullptr, tem.is_super_user)) {
                    tem.is_super_user = !tem.is_super_user;
                }
                create_hover_tooltip("Give player unlimted health and energy.");

                if (ImGui::MenuItem("RGB Suit", nullptr, tem.want_rgb_suit)) {
                    tem.want_rgb_suit = !tem.want_rgb_suit;
                }
                create_hover_tooltip("Change player color.");

#if !USE_XDEAD
                ImGui::Separator();
                if (ImGui::MenuItem("Anti Anti Debugger", nullptr, suspended_gfwl_main_thread)) {
                    if (change_gfwl_main_thread(!suspended_gfwl_main_thread)) {
                        suspended_gfwl_main_thread = !suspended_gfwl_main_thread;
                    }
                }
                create_hover_tooltip("Suspend GFWL main thread. This will make GFWL unresponsible.");
#endif

                //ImGui::Separator();
                //if (ImGui::MenuItem("Settings")) {
                //}
                ImGui::Separator();
                if (ImGui::MenuItem("Shutdown")) {
                    CreateRemoteThread(GetCurrentProcess(), 0, 0, LPTHREAD_START_ROUTINE(shutdown_thread), 0, 0, 0);
                }
                create_hover_tooltip("Unload TEM module.");

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("HUD")) {
                if (ImGui::MenuItem("FPS", nullptr, ui.show_fps)) {
                    ui.show_fps = !ui.show_fps;
                }
                if (ImGui::MenuItem("Timer", nullptr, ui.show_timer)) {
                    ui.show_timer = !ui.show_timer;
                }
                if (ImGui::MenuItem("Position", nullptr, ui.show_position)) {
                    ui.show_position = !ui.show_position;
                }
                if (ImGui::MenuItem("Angle", nullptr, ui.show_angle)) {
                    ui.show_angle = !ui.show_angle;
                }
                if (ImGui::MenuItem("Velocity", nullptr, ui.show_velocity)) {
                    ui.show_velocity = !ui.show_velocity;
                }
                if (ImGui::MenuItem("Health", nullptr, ui.show_health)) {
                    ui.show_health = !ui.show_health;
                }
                if (ImGui::MenuItem("Enemy Health", nullptr, ui.show_enemy_health)) {
                    ui.show_enemy_health = !ui.show_enemy_health;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Level") && tem.engine()) {
                auto current_level = tem.engine()->get_level_name();

                for (auto& [level_name, display_name] : game_levels) {
                    if (display_name == "Main Menu") {
                        ImGui::Separator();
                    }

                    auto level_name_w = std::wstring(level_name.begin(), level_name.end());
                    auto is_on_current_level = lstrcmpW(level_name_w.c_str(), current_level) == 0;

                    if (ImGui::MenuItem(display_name.c_str(), nullptr, is_on_current_level)) {
                        auto command = std::wstring(L"open TronGame_P") + std::wstring(L"?chapter=" + level_name_w) +
                            //std::wstring(L"?MaxPlayers=1") +
                            //std::wstring(L"?XP=33089") +
                            //std::wstring(L"?ELO=1600") +
                            //std::wstring(L"?SKP=16") +
                            //std::wstring(L"?PlayerName=\"NeKz\"") +
                            //std::wstring(L"?DLCmaps=0") +
                            //std::wstring(L"?PawnArch=0") +
                            //std::wstring(L"?VehArch=0") +
                            //std::wstring(L"?PlayerSkin=-1") +
                            //std::wstring(L"?UnlockSystem=3_5_8_13_19_26_") +
                            //std::wstring(L"?ChapterPoints=true") +
                            std::wstring(L"?Game=GridGame.PgGameInfo");

                        tem.console_command(command);
                    }
                    create_hover_tooltip(level_name.c_str());
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Experimental")) {
                if (ImGui::MenuItem("Inputs", nullptr, ui.show_inputs)) {
                    ui.show_inputs = !ui.show_inputs;

                    auto player_input = tem.player_controller() ? tem.player_controller()->player_input : nullptr;
                    if (player_input) {
                        for (auto& [key, value] : tem.command_to_key_move) {
                            auto& keys = value.second;
                            keys.clear();
                        }

                        foreach_item(binding, player_input->bindings)
                        {
                            auto key = tem.find_name(binding.name);
                            auto command_str = binding.command.str();
                            auto mapping_iter = tem.command_to_key_move.find(command_str);

                            if (mapping_iter != tem.command_to_key_move.end()) {
                                auto& mapping = mapping_iter->second;
                                mapping.second.push_back(binding.name.index);
                                println("[ui] mapping command {} to key = {}", command_str, key);
                            }
                        }
                    }
                }
                create_hover_tooltip("Show game inputs.");

                if (ImGui::MenuItem("Weak Enemies", nullptr, tem.want_weak_enemies)) {
                    tem.want_weak_enemies = !tem.want_weak_enemies;
                }
                create_hover_tooltip("Enemies in range will be super weak.");

                if (tem.engine() && tem.engine()->get_local_player() && tem.engine()->get_local_player()->actor
                    && tem.engine()->get_local_player()->actor->cheat_manager) {
                    if (ImGui::MenuItem("PgCheatManager::OnScreenWarnings()")) {
                        auto cheat_manager = tem.engine()->get_local_player()->actor->cheat_manager;
                        Memory::VMT<int(__stdcall*)()>(cheat_manager, 71)();
                        println("Called PgCheatManager::OnScreenWarnings()");
                    }
                    create_hover_tooltip("Show Kismet debug warnings.");

                    if (ImGui::MenuItem("PgCheatManager::DoApplyXP(69420)")) {
                        auto cheat_manager = tem.engine()->get_local_player()->actor->cheat_manager;
                        Memory::VMT<int(__stdcall*)(int xp)>(cheat_manager, 82)(69'420);
                        println("Called PgCheatManager::DoApplyXP(69420)");
                    }
                    create_hover_tooltip("Give yourself XP :)");

                    if (ImGui::MenuItem("Dump Engine Data")) {
                        dump_engine();
                        //dump_engine_to_json();
                        dump_engine_to_markdown();
                        //dump_console_commands();
                    }
                    create_hover_tooltip("Dump engine names and objects. Game will freeze for a few seconds.");

                    // PgUnlockSystem::SetPlayerSkin
                    //if (ImGui::MenuItem("PgUnlockSystem::SetPlayerSkin")) {
                    //    struct PgUnlockItemPlayerSkin {};

                    //    auto controller = tem.player_controller();
                    //    auto unlock_system = controller->unlock_system;
                    //    println("PgUnlockSystem 0x{:04x}", uintptr_t(unlock_system));

                    //    auto blackguard_skin_addr = uintptr_t(0x18220000);
                    //    auto blackguard_skin = reinterpret_cast<PgUnlockItemPlayerSkin*>(&blackguard_skin_addr);
                    //    println("BG Skin 0x{:04x}", uintptr_t(blackguard_skin));

                    //    auto SetPlayerSkin = Memory::VMT<void(__thiscall*)(PgUnlockSystem * thisptr, PgUnlockItemPlayerSkin* skin)>(unlock_system, 106);

                    //    SetPlayerSkin(unlock_system, blackguard_skin);
                    //    println("CALLED PgUnlockSystem::SetPlayerSkin 0x{:04x}", uintptr_t(SetPlayerSkin));
                    //}
                    //create_hover_tooltip("Test.");
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }

    return Present(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}
