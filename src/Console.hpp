/*
 * Copyright (c) 2022-2023 NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include "Memory.hpp"
#include <format>

class Console {
private:
    HANDLE output = nullptr;
    HANDLE input = nullptr;

public:
    auto Init() -> bool;
    auto Shutdown() -> void;
    auto Tick() -> void;
    template <typename... Args> inline auto Print(const std::string format, Args&&... args) -> void
    {
        this->PrintInternal(std::vformat(format, std::make_format_args(args...)));
    }
    template <typename... Args> inline auto Println(const std::string format, Args&&... args) -> void
    {
        this->PrintInternal(std::vformat(format, std::make_format_args(args...)) + "\n");
    }
    template <typename... Args> inline auto PrintW(const std::wstring format, Args&&... args) -> void
    {
        this->PrintInternalW(std::vformat(format, std::make_wformat_args(args...)));
    }
    template <typename... Args> inline auto PrintlnW(const std::wstring format, Args&&... args) -> void
    {
        this->PrintInternalW(std::vformat(format, std::make_wformat_args(args...)) + L"\n");
    }

private:
    auto PrintInternal(std::string&& message) -> void;
    auto PrintInternalW(std::wstring&& message) -> void;
};

extern Console* console;
