/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <Windows.h>
#include <cinttypes>
#include <format>

extern auto patch_gfwl() -> void;
extern auto unpatch_gfwl() -> void;
extern auto change_gfwl_main_thread(bool suspend) -> bool;

extern bool suspended_gfwl_main_thread;

#define PROPERTY_NUMPLAYERS 0x10000001
#define PROPERTY_SKILLRATING 0x10000006
#define PROPERTY_PRIVATE 0x10000013
#define PROPERTY_DLCFLAGS 0x10000015
#define PROPERTY_TIMELIMIT 0x10000017
#define PROPERTY_TIMECURRENT 0x10000018
#define PROPERTY_VERSION 0x1000001A
#define PROPERTY_SESSIONID 0x20000016
#define CONTEXT_GAME_MODE_MULTIPLAYER 0
#define CONTEXT_MODE 3
#define CONTEXT_MAP 4
#define X_CONTEXT_GAME_TYPE_RANKED 0
#define X_CONTEXT_GAME_TYPE_STANDARD 1
#define X_CONTEXT_PRESENCE 0x8001
#define X_CONTEXT_GAME_TYPE 0x800A
#define X_CONTEXT_GAME_MODE 0x800B
#define XPRIVILEGE_PRESENCE_FRIENDS_ONLY 0xF3
#define XPRIVILEGE_PRESENCE 0xF4
#define XPRIVILEGE_PURCHASE_CONTENT 0xF5
#define XPRIVILEGE_USER_CREATED_CONTENT_FRIENDS_ONLY 0xF6
#define XPRIVILEGE_USER_CREATED_CONTENT 0xF7
#define XPRIVILEGE_PROFILE_VIEWING_FRIENDS_ONLY 0xF8
#define XPRIVILEGE_PROFILE_VIEWING 0xF9
#define XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY 0xFB
#define XPRIVILEGE_COMMUNICATIONS 0xFC
#define XPRIVILEGE_MULTIPLAYER_SESSIONS 0xFE

enum class XUSER_SIGNIN_STATE : uint32_t {
    NotSignedIn = 0,
    SignedInLocally = 1,
    SignedInToLive = 2,
};

template <class CharT> struct std::formatter<XUSER_SIGNIN_STATE, CharT> : std::formatter<uint32_t, CharT> {
    template <class FormatContext> auto format(XUSER_SIGNIN_STATE temp, FormatContext& context) const
    {
        return std::formatter<uint32_t, CharT>::format(uint32_t(temp), context);
    }
};

struct XUSER_SIGNIN_INFO {
    ULONGLONG xuid;
    DWORD flags;
    XUSER_SIGNIN_STATE user_signin_state;
    DWORD guest_number;
    DWORD sponsor_user_index;
    CHAR user_name[16];
};
