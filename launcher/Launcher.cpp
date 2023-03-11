/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 * 
 * 
 * This is a partial reimplementation of the SecuROM launcher which
 * just starts the game in a new process. This effectively bypasses
 * the USELESS and SLOW registry checks AFTER production activation.
 * 
 * NOTE: The game has to be patched beforehand:
 *       a.) Statically by removing all spot checks + patching GFWL for
 *           offline usage since GFWL does a signature check.
 *       b.) Or dynamically but make sure that spot checks are never run.
 * 
 * Read for more information: https://tem.nekz.me/reversed/securom.html#launcher
 */

#include <Windows.h>
#include <conio.h>
#include <format>
#include <iostream>
#include <stdio.h>
#include <string>
#include <tchar.h>
#include <vector>

#define GAME_EXE L"GridGame.exe"
#define SECUROM_BUFFER_SIZE 1723
#define TR2NPC_PATH L"Software\\Disney Interactive Studios\\tr2npc"

#define LOG_PREFIX "[launcher] "
#define LOG_PREFIX_L L"[launcher] "

template <typename... Args> inline auto println(const std::string format, Args&&... args) -> void
{
    auto msg = std::vformat(format, std::make_format_args(args...));
    std::cout << LOG_PREFIX << msg << std::endl;
    OutputDebugStringA((std::string(LOG_PREFIX) + msg + "\n").c_str());
}
template <typename... Args> inline auto wprintln(const std::wstring format, Args&&... args) -> void
{
    auto msg = std::vformat(format, std::make_wformat_args(args...));
    std::wcout << LOG_PREFIX_L << msg << std::endl;
    OutputDebugStringW((std::wstring(LOG_PREFIX_L) + msg + L"\n").c_str());
}

auto main() -> int
{
    // I wonder what SMS means. SecuROM's Mapping Signature?
    auto file = std::format(L"-=[SMS_{}_SMS]=-", GAME_EXE);

    auto handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SECUROM_BUFFER_SIZE, file.c_str());
    if (!handle) {
        println("Could not create file mapping object {}", GetLastError());
        return 1;
    }

    auto buffer = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, SECUROM_BUFFER_SIZE);
    if (!buffer) {
        println("Could not map view of file {}", GetLastError());
        CloseHandle(handle);
        return 1;
    }

    println("Created file mapping");

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
        println("Failed to find installation path");
        return 1;
    }

    auto cmd = std::format(L"{}\\{}", install_path, GAME_EXE);
    CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    wprintln(L"Launched {}", cmd);

    println("hProcess = {:x}", uintptr_t(pi.hProcess));
    println("hThread = {:x}", uintptr_t(pi.hThread));
    println("dwProcessId = {:x}", uintptr_t(pi.dwProcessId));
    println("dwThreadId = {:x}", uintptr_t(pi.dwThreadId));

    WaitForSingleObject(pi.hProcess, INFINITE);

    println("exited");

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    UnmapViewOfFile(LPCVOID(buffer));
    CloseHandle(handle);

    return 0;
}
