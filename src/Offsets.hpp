/*
* Copyright (c) 2022-2023, NeKz
*
* SPDX-License-Identifier: MIT

*/
#pragma once

namespace Offsets {

typedef uint32_t vtable_offset_t, object_offset_t, function_offset_t;

// UE3
const object_offset_t g_Objects = 0x2278E7C;
const object_offset_t g_Names = 0x2278E4C;
const object_offset_t g_Engine = 0x2286CA8;
const object_offset_t g_Renderer = 0x227DDF0;
const vtable_offset_t ProcessEvent = 62;
const function_offset_t GetTeamColor = 0x1439DF0;
const vtable_offset_t ConsoleCommand = 71;

// GFWL
const function_offset_t xlive_memory_check = 0xF370C;
const function_offset_t xlive_main_thread_start_address = 0x146AB8;

// GridGame
const function_offset_t forced_window_minimize = 0x13587B8;
const object_offset_t save_filename = 0x21AA580;
const object_offset_t pg_save_load = 0x22B571C;

// D3D9
const vtable_offset_t Reset = 16;
const vtable_offset_t Present = 17;
}
