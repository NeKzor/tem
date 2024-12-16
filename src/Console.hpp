/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <windows.h>
#include <format>
#include <string>

#define CONSOLE_PREFIX "[tem] "

template <typename... Args> inline auto print(const std::string format, Args&&... args) -> void
{
    printf("%s", (CONSOLE_PREFIX + std::vformat(format, std::make_format_args(args...))).c_str());
}
template <typename... Args> inline auto println(const std::string format, Args&&... args) -> void
{
    printf("%s", (CONSOLE_PREFIX + std::vformat(format, std::make_format_args(args...)) + "\n").c_str());
}
template <typename... Args> inline auto wprint(const std::wstring format, Args&&... args) -> void
{
    wprintf("%s", (TEXT(CONSOLE_PREFIX) + std::vformat(format, std::make_wformat_args(args...))).c_str());
}
template <typename... Args> inline auto wprintln(const std::wstring format, Args&&... args) -> void
{
    wprintf("%s", (TEXT(CONSOLE_PREFIX) + std::vformat(format, std::make_wformat_args(args...)) + L"\n").c_str());
}
