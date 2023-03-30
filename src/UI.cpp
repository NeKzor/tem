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

BOOL CALLBACK ui_window(HWND handle, LPARAM pid)
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
        println("[ui] Window: title = {}, proc_id = {}, handle = {}, pid = {}", title, proc_id, int(handle), int(pid));
        ui.window_handle = handle;
        return false;
    }

    return true;
}

auto ui_init() -> void
{
    if (ui.hooked) {
        return;
    }

    ui.window_handle = nullptr;

    EnumWindows(ui_window, GetCurrentProcessId());

    if (!ui.window_handle) {
        return println("[ui] Unable to find window handle :(");
    }

    auto g_pD3DDevice = Memory::Deref<void*>(Offsets::g_pD3DDevice);
    if (!g_pD3DDevice) {
        return println("[ui] g_pD3DDevice is null :(");
    }

    println("[ui] g_pD3DDevice: 0x{:x}", uintptr_t(g_pD3DDevice));

    auto reset = Memory::VMT(g_pD3DDevice, Offsets::Reset);
    auto present = Memory::VMT(g_pD3DDevice, Offsets::Present);

    Hooks::queue("IDirect3DDevice9::Reset", &Reset, Reset_Hook, reset);
    Hooks::queue("IDirect3DDevice9::Present", &Present, Present_Hook, present);

    ui.hooked = true;
}

auto ui_shutdown() -> void
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
            typedef uint8_t mode;
            typedef uint8_t row;
            typedef uint8_t col;
            typedef uint8_t len;

            struct IHudElement {
                mode required_mode;
                col col;
                row row;
                len length;
                const char* text;
                uint32_t move;
            };

            struct IHudContext {
                mode mode = 4;
                int buttons = 0;
                float size = 40.0f;
                float padding = 2.0f;
                float x_offset = 0.0f;
                float y_offset = 0.0f;
                ImU32 color = IM_COL32(0, 0, 0, 235);
                ImU32 shadow_color = IM_COL32(0, 0, 0, 100);
                ImU32 font_color = IM_COL32(255, 255, 255, 255);
                ImU32 font_shadow_color = IM_COL32(255, 255, 255, 64);
                bool draw_shadow = true;
                int element = 0;
                float max_x = 0.0f;
                float max_y = 0.0f;
                int max_rows = 6;
            } ctx;

            ctx.x_offset = x;
            ctx.y_offset
                = ImGui::GetIO().DisplaySize.y - (ctx.max_rows * ctx.size) - ((ctx.max_rows - 1) * ctx.padding);

            auto player_input = tem.player_controller()->player_input;

            foreach_item(key_name, player_input->pressed_keys)
            {
                for (auto& mappingIter : tem.command_to_key_move) {
                    auto& mapping = mappingIter.second;
                    auto& flag = mapping.first;
                    auto& key = mapping.second;

                    if (std::find(key.begin(), key.end(), key_name.index) != key.end()) {
                        ctx.buttons |= flag;
                    }
                }
            }

            ImGui::SetNextWindowPos(ImVec2(x, y));
            ImGui::Begin("inputs", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings
                    | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);

            auto draw_list = ImGui::GetWindowDrawList();

            auto draw_element = [&ctx, &draw_list](IHudElement element) {
                auto x = ctx.x_offset + (element.col * ctx.size) + ((element.col + 1) * ctx.padding);
                auto y = ctx.y_offset + (element.row * ctx.size) + ((element.row + 1) * ctx.padding);

                auto enabled = ctx.mode >= element.required_mode;
                auto is_pressed = ctx.buttons & element.move;
                auto draw_button = is_pressed || ctx.draw_shadow;

                if (enabled && draw_button) {
                    auto x0 = x + ((element.col + 1) * ctx.padding);
                    auto y0 = y + ((element.row + 1) * ctx.padding);
                    auto x1 = x + ((((element.col + 1) * ctx.padding) + ctx.size) * element.length);
                    auto y1 = y + ((element.row + 1) * ctx.padding + ctx.size);

                    draw_list->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), is_pressed ? ctx.color : ctx.shadow_color);

                    auto text_size = ImGui::CalcTextSize(element.text);
                    auto text_color = is_pressed ? ctx.font_color : ctx.font_shadow_color;

                    auto xc = x0 + ((x1 - x0) / 2);
                    auto yc = y0 + ((y1 - y0) / 2);
                    auto tx = xc - (text_size.x / 2);
                    auto ty = yc - (text_size.y / 2);

                    draw_list->AddText(ImVec2(tx, ty), text_color, element.text);

                    if (x1 > ctx.max_x) {
                        ctx.max_x = x1;
                    }

                    if (y1 > ctx.max_y) {
                        ctx.max_y = y1;
                    }
                }

                ++ctx.element;
            };

            /*
                Layout:
                    row|col0|1|2|3|4|5|6|7|8|9
                    ---|----------------------
                      0|    |1|2|3|4
                      2|       w
                      3|    |a|s|d|f
                      4|shft|   |c
                      5|ctrl|spacebar     |l|r
            */

            draw_element(IHudElement{ mode(4), col(1), row(0), len(1), "1", MV_SWITCH_TO_HEAVY_DISC });
            draw_element(IHudElement{ mode(4), col(2), row(0), len(1), "2", MV_SWITCH_TO_BOMB_DISC });
            draw_element(IHudElement{ mode(4), col(3), row(0), len(1), "3", MV_SWITCH_TO_STASIS_DISC });
            draw_element(IHudElement{ mode(4), col(4), row(0), len(1), "4", MV_SWITCH_TO_CORRUPTION_DISC });

            draw_element(IHudElement{ mode(1), col(2), row(1), len(1), "W", MV_FORWARD });
            draw_element(IHudElement{ mode(1), col(1), row(2), len(1), "A", MV_LEFT });
            draw_element(IHudElement{ mode(1), col(2), row(2), len(1), "S", MV_BACKWARD });
            draw_element(IHudElement{ mode(1), col(3), row(2), len(1), "D", MV_RIGHT });

            draw_element(IHudElement{ mode(2), col(0), row(3), len(1), "S", MV_SPRINT });
            draw_element(IHudElement{ mode(2), col(0), row(4), len(1), "C", MV_BLOCK });
            draw_element(IHudElement{ mode(2), col(1), row(4), len(7), "S", MV_JUMP });
            draw_element(IHudElement{ mode(2), col(4), row(2), len(1), "F", MV_DISC_POWER });
            draw_element(IHudElement{ mode(2), col(3), row(3), len(1), "C", MV_CAMERA_RESET });

            draw_element(IHudElement{ mode(3), col(8), row(4), len(1), "L", MV_DISC_ATTACK });
            draw_element(IHudElement{ mode(3), col(9), row(4), len(1), "R", MV_MELEE_ATTACK });

            ImGui::Dummy(ImVec2(ctx.max_x, ctx.max_y));

            ImGui::End();
        }

        if (ui.menu && ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("TEM")) {
                if (ImGui::MenuItem("Superuser", nullptr, tem.is_super_user)) {
                    tem.is_super_user = !tem.is_super_user;

                    if (!tem.is_super_user) {
                        auto controller = tem.player_controller();
                        if (controller) {
                            controller->set_god_mode(false);
                        }
                    }
                }
                create_hover_tooltip("Give player unlimted health and energy.");

                if (ImGui::MenuItem("RGB Suit", nullptr, tem.want_rgb_suit)) {
                    tem.want_rgb_suit = !tem.want_rgb_suit;
                }
                create_hover_tooltip("Change player color.");

                if (ImGui::MenuItem("Disable Window Minimize", nullptr, tem.disabled_forced_window_minimize)) {
                    patch_forced_window_minimize(!tem.disabled_forced_window_minimize);
                }
                create_hover_tooltip("This makes it possible to keep the game open even when tabbed out.");

                if (!is_using_xdead) {
                    ImGui::Separator();
                    if (ImGui::MenuItem("Anti Anti Debugger", nullptr, suspended_gfwl_main_thread)) {
                        if (change_gfwl_main_thread(!suspended_gfwl_main_thread)) {
                            suspended_gfwl_main_thread = !suspended_gfwl_main_thread;
                        }
                    }
                    create_hover_tooltip("Suspend GFWL main thread. This will make GFWL unresponsible.");
                }

                //ImGui::Separator();
                //if (ImGui::MenuItem("Settings")) {
                //}
                ImGui::Separator();
                if (ImGui::MenuItem("Shutdown")) {
                    CreateRemoteThread(GetCurrentProcess(), 0, 0, LPTHREAD_START_ROUTINE(tem_shutdown), 0, 0, 0);
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
