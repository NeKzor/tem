/*
 * Copyright (c) 2022-2023 NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <format>
#include <string>
#include <windows.h>

template <typename... Args>
inline auto println(const std::string format, Args&&... args) -> void
{
    auto msg = std::vformat(format, std::make_format_args(args...));
    OutputDebugStringA(("[dinput8.dll] " + msg + "\n").c_str());
}
