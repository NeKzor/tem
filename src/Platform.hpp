/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include "lib/minhook/MinHook.h"

#define DECL_DETOUR(name, firstparam, ...)                                                                             \
    using _##name = int(__thiscall*)(firstparam, ##__VA_ARGS__);                                                       \
    static _##name name;                                                                                               \
    static uintptr_t name##_Target;                                                                                    \
    static int __fastcall name##_Hook(firstparam, int edx, ##__VA_ARGS__)
#define DECL_DETOUR_T(type, name, firstparam, ...)                                                                     \
    using _##name = type(__thiscall*)(firstparam, ##__VA_ARGS__);                                                      \
    static _##name name;                                                                                               \
    static uintptr_t name##_Target;                                                                                    \
    static type __fastcall name##_Hook(firstparam, int edx, ##__VA_ARGS__)
#define DECL_DETOUR_B(name, firstparam, ...)                                                                           \
    using _##name = int(__thiscall*)(firstparam, ##__VA_ARGS__);                                                       \
    static _##name name;                                                                                               \
    static _##name name##Base;                                                                                         \
    static uintptr_t name##_Target;                                                                                    \
    static int __fastcall name##_Hook(firstparam, int edx, ##__VA_ARGS__)
#define DECL_DETOUR_STD(type, name, firstparam, ...)                                                                   \
    using _##name = type(__stdcall*)(firstparam, ##__VA_ARGS__);                                                       \
    static _##name name;                                                                                               \
    static uintptr_t name##_Target;                                                                                    \
    static type __stdcall name##_Hook(firstparam, ##__VA_ARGS__)
#define DECL_DETOUR_API(type, cc, name, ...)                                                                           \
    using _##name = type(cc*)(##__VA_ARGS__);                                                                          \
    static _##name name;                                                                                               \
    static uintptr_t name##_Target;                                                                                    \
    static type cc name##_Hook(##__VA_ARGS__)
#define DECL_DETOUR_S(type, cc, name, ...)                                                                             \
    using _##name = type (*)(##__VA_ARGS__);                                                                           \
    static _##name name;                                                                                               \
    static uintptr_t name##_Target;                                                                                    \
    static type cc name##_Hook(##__VA_ARGS__)
#define DECL_DETOUR_MID(name)                                                                                          \
    static uintptr_t name;                                                                                             \
    static uintptr_t name##_Target;                                                                                    \
    static void name##_Hook()

#define DETOUR(name, firstparam, ...) int __fastcall name##_Hook(firstparam, int edx, ##__VA_ARGS__)
#define DETOUR_T(type, name, firstparam, ...) type __fastcall name##_Hook(firstparam, int edx, ##__VA_ARGS__)
#define DETOUR_B(name, firstparam, ...) int __fastcall name##_Hook(firstparam, int edx, ##__VA_ARGS__)
#define DETOUR_STD(type, name, firstparam, ...) type __stdcall name##_Hook(firstparam, ##__VA_ARGS__)
#define DETOUR_API(type, cc, name, ...) type cc name##_Hook(##__VA_ARGS__)
#define DETOUR_MID(name) __declspec(naked) void name##_Hook()

namespace Hooks {
static auto initialize() -> void
{
    auto status = MH_Initialize();
    if (status != MH_STATUS::MH_OK) {
        return println("[hooks] Failed to initialize hooking engine | MH_STATUS = {}", int(status));
    }

    println("[hooks] Initialized hooking engine");
}
static auto uninitialize() -> void
{
    auto status = MH_Uninitialize();
    if (status != MH_STATUS::MH_OK) {
        return println("[hooks] Failed to uninitialize hooking engine | MH_STATUS = {}", int(status));
    }

    println("[hooks] Uninitialized hooking engine");
}
static auto apply_queued() -> void
{
    auto status = MH_ApplyQueued();
    if (status != MH_STATUS::MH_OK) {
        return println("[hooks] Failed to apply all queued hooks | MH_STATUS = {}", int(status));
    }

    println("[hooks] Applied all queued hooks");
}

template <typename Original, typename Hook>
static auto hook(const char* name, Original* trampoline, Hook hook, uintptr_t original) -> void
{
    auto create_status = MH_CreateHook(LPVOID(original), LPVOID(hook), reinterpret_cast<LPVOID*>(trampoline));
    if (create_status != MH_STATUS::MH_OK) {
        return println(
            "[hooks] Failed to create hook {} at 0x{:x} | MH_STATUS = {}", name, original, int(create_status));
    }

    auto enable_status = MH_EnableHook(LPVOID(original));
    if (enable_status != MH_STATUS::MH_OK) {
        return println(
            "[hooks] Failed to enable hook {} at 0x{:x} | MH_STATUS = {}", name, original, int(enable_status));
    }

    println("[hooks] Hooked {} at 0x{:x}", name, original);
}

template <typename Trampoline, typename Hook>
static auto queue(const char* name, Trampoline* trampoline, Hook hook, uintptr_t original) -> void
{
    auto create_status = MH_CreateHook(LPVOID(original), LPVOID(hook), reinterpret_cast<LPVOID*>(trampoline));
    if (create_status != MH_STATUS::MH_OK) {
        return println(
            "[hooks] Failed to create hook {} at 0x{:x} | MH_STATUS = {}", name, original, int(create_status));
    }

    auto queue_status = MH_QueueEnableHook(LPVOID(original));
    if (queue_status != MH_STATUS::MH_OK) {
        return println("[hooks] Failed to queue hook {} at 0x{:x} | MH_STATUS = {}", name, original, int(queue_status));
    }

    println("[hooks] Hooking {} at 0x{:x}", name, original);
}

}
