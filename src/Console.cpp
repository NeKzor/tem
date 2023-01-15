/*
 * Copyright (c) 2022-2023 NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#include "Console.hpp"
#include "TEM.hpp"

auto Console::Init() -> bool
{
    //AllocConsole();
    //SetConsoleTitleA("TEM " TEM_VERSION);

    //this->output = GetStdHandle(STD_OUTPUT_HANDLE);
    //this->input = GetStdHandle(STD_INPUT_HANDLE);

    return true;
}
auto Console::Shutdown() -> void
{
    //FreeConsole();

    this->output = nullptr;
    this->input = nullptr;
}
auto Console::Tick() -> void {}
auto Console::PrintInternal(std::string&& message) -> void
{
    OutputDebugStringA(("[tem] " + message).c_str());

    if (!this->output) {
        return;
    }

    WriteConsoleA(this->output, message.c_str(), static_cast<DWORD>(message.length()), nullptr, nullptr);
}
auto Console::PrintInternalW(std::wstring&& message) -> void
{
    OutputDebugStringW((L"[tem] " + message).c_str());

    if (!this->output) {
        return;
    }

    WriteConsoleW(this->output, message.c_str(), static_cast<DWORD>(message.length()), nullptr, nullptr);
}

Console* console;
