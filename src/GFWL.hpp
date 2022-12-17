/*
 * Copyright (c) 2022 NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

extern auto patch_gfwl() -> void;
extern auto unpatch_gfwl() -> void;
extern auto change_gfwl_main_thread(bool suspend) -> bool;

extern bool suspended_gfwl_main_thread;
