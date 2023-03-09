/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

namespace Offsets {

// UE3
int g_Objects = 0x2278E7C;
int g_Names = 0x2278E4C;
int g_Engine = 0x2286CA8;
int g_Renderer = 0x227DDF0;
int ProcessEvent = 62;
int GetTeamColor = 0x1439DF0;
int ConsoleCommand = 71;

// GFWL
int xlive_memory_check = 0xF370C;
int xlive_main_thread_start_address = 0x146AB8;

// GridGame
int forced_window_minimize = 0x13587B8;
int save_filename = 0x21AA580;
int pg_save_load = 0x22B571C;

// D3D9
int Reset = 16;
int Present = 17;
}
