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

#define DETOUR(name, firstparam, ...) int __fastcall name##_Hook(firstparam, int edx, ##__VA_ARGS__)
#define DETOUR_T(type, name, firstparam, ...) type __fastcall name##_Hook(firstparam, int edx, ##__VA_ARGS__)
#define DETOUR_B(name, firstparam, ...) int __fastcall name##_Hook(firstparam, int edx, ##__VA_ARGS__)
#define DETOUR_STD(type, name, firstparam, ...) type __stdcall name##_Hook(firstparam, ##__VA_ARGS__)
#define DETOUR_API(type, cc, name, ...) type cc name##_Hook(##__VA_ARGS__)

namespace {
bool mhInitialized = false;
}
#define MH_HOOK(name, target)                                                                                          \
    if (!mhInitialized) {                                                                                              \
        MH_Initialize();                                                                                               \
        mhInitialized = true;                                                                                          \
    }                                                                                                                  \
    name##_Target = uintptr_t(target);                                                                                 \
    auto name##_create_status                                                                                          \
        = MH_CreateHook(reinterpret_cast<LPVOID>(target), name##_Hook, reinterpret_cast<LPVOID*>(&name));              \
    auto name##_enable_status = MH_EnableHook(reinterpret_cast<LPVOID>(target));
#define MH_HOOK_MID(name, target)                                                                                      \
    if (!mhInitialized) {                                                                                              \
        MH_Initialize();                                                                                               \
        mhInitialized = true;                                                                                          \
    }                                                                                                                  \
    name##_Target = uintptr_t(target);                                                                                 \
    auto name##_create_status                                                                                          \
        = MH_CreateHook(reinterpret_cast<LPVOID>(target), name##_Hook, reinterpret_cast<LPVOID*>(&name));              \
    auto name##_enable_status = MH_EnableHook(reinterpret_cast<LPVOID>(target));
#define MH_QUEUE_HOOK(name, target)                                                                                    \
    if (!mhInitialized) {                                                                                              \
        MH_Initialize();                                                                                               \
        mhInitialized = true;                                                                                          \
    }                                                                                                                  \
    name##_Target = uintptr_t(target);                                                                                 \
    auto name##_create_status                                                                                          \
        = MH_CreateHook(reinterpret_cast<LPVOID>(target), name##_Hook, reinterpret_cast<LPVOID*>(&name));              \
    auto name##_enable_status = MH_EnableHook(reinterpret_cast<LPVOID>(target));
#define MH_QUEUE_HOOK_MID(name, target)                                                                                \
    if (!mhInitialized) {                                                                                              \
        MH_Initialize();                                                                                               \
        mhInitialized = true;                                                                                          \
    }                                                                                                                  \
    name##_Target = uintptr_t(target);                                                                                 \
    auto name##_create_status                                                                                          \
        = MH_CreateHook(reinterpret_cast<LPVOID>(target), name##_Hook, reinterpret_cast<LPVOID*>(&name));              \
    auto name##_enable_status = MH_EnableHook(reinterpret_cast<LPVOID>(target));
#define MH_APPLY_QUEUED() MH_ApplyQueued()
#define MH_UNHOOK(name)                                                                                                \
    auto name##_disable_status = MH_DisableHook(reinterpret_cast<LPVOID>(name##_Target));                              \
    auto name##_remove_status = MH_RemoveHook(reinterpret_cast<LPVOID>(name##_Target));
#define MH_UNHOOK_TARGET(target)                                                                                       \
    auto disable_status = MH_DisableHook(reinterpret_cast<LPVOID>(target));                                            \
    auto remove_status = MH_RemoveHook(reinterpret_cast<LPVOID>(target));

#define DECL_DETOUR_MID(name)                                                                                          \
    static uintptr_t name;                                                                                             \
    static uintptr_t name##_Target;                                                                                    \
    static void name##_Hook()

#define DETOUR_MID(name) __declspec(naked) void name##_Hook()
