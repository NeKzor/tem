/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <windows.h>
#include <atomic>

struct UI {
    HWND window_handle = nullptr;
    WNDPROC window_proc = nullptr;
    bool menu = true;
    bool hooked = false;
    bool initialized = false;
    bool show_fps = false;
    bool show_timer = false;
    bool show_position = true;
    bool show_angle = true;
    bool show_velocity = true;
    bool show_health = false;
    bool show_enemy_health = false;
    bool show_flags = false;
    bool show_inputs = false;
    std::atomic<bool> is_shutdown = false;

    inline auto game_window_is_focused() -> bool { return GetForegroundWindow() == this->window_handle; }
};

extern UI ui;

extern auto ui_init() -> void;
extern auto ui_shutdown() -> void;
