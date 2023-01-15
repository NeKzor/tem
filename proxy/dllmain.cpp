/*
 * Copyright (c) 2022-2023 NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#include "pch.h"
#include <intrin.h>
#include <map>

#define EXPORT __declspec(dllexport)
#define ORIGINAL(name) name##_original

#define DECL_DETOUR_API(type, cc, name, ...)  \
    using _##name = type(cc*)(##__VA_ARGS__); \
    _##name ORIGINAL(name);                   \
    EXPORT type cc name(##__VA_ARGS__)

#define DETOUR_API(type, cc, name, ...) EXPORT type cc name(##__VA_ARGS__)

DECL_DETOUR_API(HRESULT, __stdcall, DirectInput8Create, HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, void* punkOuter);

DETOUR_API(HRESULT, __stdcall, DirectInput8Create, HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, void* punkOuter)
{
    auto function = ORIGINAL(DirectInput8Create);
    auto result = function(hinst, dwVersion, riidltf, ppvOut, punkOuter);
    println(
        "[0x{:06x}] [0x{:06x}] {}({:x}, {:x}, {:x}, {:x}, {:x}) -> {:x} | {}",
        uintptr_t(function),
        uintptr_t(_ReturnAddress()),
        "DirectInput8Create",
        uintptr_t(hinst),
        uintptr_t(dwVersion),
        uintptr_t(riidltf.Data1),
        uintptr_t(ppvOut),
        uintptr_t(punkOuter),
        uintptr_t(result),
        uintptr_t(result));

    /*while(!IsDebuggerPresent()) {
        Sleep(420);
    }*/

    return result;
}

auto init_proxy() -> void
{
    println("Proxying...");

    char system_dir[MAX_PATH] = {};
    GetSystemDirectoryA(system_dir, sizeof(system_dir));

    auto dinput8_path = std::string(system_dir) + "\\dinput8.dll";
    auto dinput8 = LoadLibraryA(dinput8_path.c_str());

    println("Orginal dinput8.dll {:x} in {}", int(dinput8), system_dir);

    if (!dinput8) {
        return;
    }

#define PROXY_IMPORT(name)                                                   \
    auto name##_address = uintptr_t(GetProcAddress(dinput8, LPCSTR(#name))); \
    if (name##_address) {                                                    \
        ORIGINAL(name) = decltype(ORIGINAL(name))(name##_address);           \
        println("Proxied {} at {:x}", #name, name##_address);                \
    } else {                                                                 \
        println("Unable to proxy " #name);                                   \
    }

    PROXY_IMPORT(DirectInput8Create);

#undef PROXY_IMPORT
  
    auto tem = LoadLibraryA("tem.dll");
    println("Loaded tem.dll {:x}", uintptr_t(tem));
}

auto once = false;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
        if (!once) {
            once = true;
            init_proxy();
        }
        break;
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
