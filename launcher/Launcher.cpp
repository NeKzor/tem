/*
 * Copyright (c) 2022 - 2023 NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#include "Memory.hpp"
#include <Windows.h>
#include <conio.h>
#include <format>
#include <iostream>
#include <stdio.h>
#include <string>
#include <tchar.h>
#include <vector>

template <typename... Args> inline auto println(const std::string format, Args&&... args) -> void
{
    auto msg = std::vformat(format, std::make_format_args(args...));
    std::cout << msg << std::endl;
    OutputDebugStringA(("[game] " + msg + "\n").c_str());
}

auto launcher_patch = Memory::Patch();

#define GAME_EXE L"GridGame.exe"
#define BUFFER_SIZE 1723
#define TR2NPC_PATH L"Software\\Disney Interactive Studios\\tr2npc"

auto main() -> int
{
    println("Creating file mapping...");

    // I wonder what SMS means. SecuROM's Mapping Signature?
    auto file = std::format(L"-=[SMS_{}_SMS]=-", GAME_EXE);

    auto handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUFFER_SIZE, file.c_str());
    if (!handle) {
        println("Could not create file mapping object {}", GetLastError());
        return 1;
    }

    auto buffer = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, BUFFER_SIZE);
    if (!buffer) {
        println("Could not map view of file {}", GetLastError());
        CloseHandle(handle);
        return 1;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    WCHAR install_path[MAX_PATH] = {};
    auto install_path_size = DWORD(sizeof(install_path));

    auto result = RegGetValueW(
        HKEY_LOCAL_MACHINE, TR2NPC_PATH, L"InstallPath", RRF_RT_ANY, nullptr, &install_path, &install_path_size);

    if (result != ERROR_SUCCESS) {
        println("Failed to get registry key value for install path");
        return 1;
    }

    auto cmd = std::format(L"{}\\{}", install_path, GAME_EXE);
    CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    println("launched");

    println("hProcess = {:x}", uintptr_t(pi.hProcess));
    println("hThread = {:x}", uintptr_t(pi.hThread));
    println("dwProcessId = {:x}", uintptr_t(pi.dwProcessId));
    println("dwThreadId = {:x}", uintptr_t(pi.dwThreadId));

    WaitForSingleObject(pi.hProcess, INFINITE);

    println("exited");

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    UnmapViewOfFile((LPCVOID)buffer);
    CloseHandle(handle);

    return 0;
}
