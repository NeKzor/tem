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
extern bool is_using_xdead;

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

struct XLiveLeaderboardStats {
    char unk_0000[40]; // 0x00
    uint32_t xp; // 0x28
    char unk_002c[20]; // 0x2c
    uint32_t derezz; // 0x40
    char unk_0044[20]; // 0x44
    uint32_t wipes; // 0x58
    char unk_005c[20]; // 0x5c
    uint32_t assists; // 0x70
    char unk_0074[20]; // 0x74
    uint32_t bits_nodes; // 0x88
    char unk_008c[20]; // 0x8c
    uint32_t played; // 0xa0
    char unk_00a4[28]; // 0xa4
};
static_assert(offsetof(XLiveLeaderboardStats, xp) == 0x28);
static_assert(offsetof(XLiveLeaderboardStats, derezz) == 0x40);
static_assert(offsetof(XLiveLeaderboardStats, wipes) == 0x58);
static_assert(offsetof(XLiveLeaderboardStats, assists) == 0x70);
static_assert(offsetof(XLiveLeaderboardStats, bits_nodes) == 0x88);
static_assert(offsetof(XLiveLeaderboardStats, played) == 0xa0);
static_assert(sizeof(XLiveLeaderboardStats) == 0xc0);

struct XLiveLeaderboardEntry {
    ULONGLONG xuid; // 0x00
    uint32_t rank; // 0x0x8
    char unk_000c[12]; // 0x0c
    char gamer_tag[16]; // 0x18
    uint32_t columns; // 0x28
    XLiveLeaderboardStats* stats; // 0x2c
};
static_assert(offsetof(XLiveLeaderboardEntry, xuid) == 0x00);
static_assert(offsetof(XLiveLeaderboardEntry, rank) == 0x08);
static_assert(offsetof(XLiveLeaderboardEntry, gamer_tag) == 0x18);
static_assert(offsetof(XLiveLeaderboardEntry, columns) == 0x28);
static_assert(offsetof(XLiveLeaderboardEntry, stats) == 0x2c);
static_assert(sizeof(XLiveLeaderboardEntry) == 0x30);

struct XLiveLeaderboard {
    uint32_t unk_0000; // 0x00
    uint32_t unk_0004; // 0x04
    uint32_t unk_0008; // 0x08
    uint32_t unk_000c; // 0x0c
    uint32_t count; // 0x10
    XLiveLeaderboardEntry* first_entry; // 0x14
    XLiveLeaderboardEntry entries[22]; // 0x18
};
static_assert(offsetof(XLiveLeaderboard, count) == 0x10);
static_assert(offsetof(XLiveLeaderboard, first_entry) == 0x14);
static_assert(offsetof(XLiveLeaderboard, entries) == 0x18);

struct XUSER_STATS_SPEC {
	DWORD view_id;
	DWORD num_column_ids;
	WORD rgw_column_ids[64];
};

namespace xdead {
    #define XDEAD_CALLBACK(type, name, ...) type __stdcall name##_callback(##__VA_ARGS__)
    #define XDEAD_FUNCTION_NAME(name, ordinal) static constexpr auto name = ordinal

    /// <summary>
    /// Return status code.
    /// </summary>
    enum class ListenerStatus {
        /// <summary>
        /// Function call completed without problems.
        /// </summary>
        Ok = 0,
        /// <summary>
        /// Provided ordinal value could not been found.
        /// </summary>
        OrdinalNotFound = 1,
        /// <summary>
        /// Provided callback value is NULL.
        /// </summary>
        InvalidCallback = 2,
        /// <summary>
        /// Provided callback value could not been found.
        /// </summary>
        CallbackNotFound = 3,
    };

    XDEAD_FUNCTION_NAME(XSocketCreate, 3);
    XDEAD_FUNCTION_NAME(XSocketClose, 4);
    XDEAD_FUNCTION_NAME(XSocketIOCTLSocket, 6);
    XDEAD_FUNCTION_NAME(XSocketSetSockOpt, 7);
    XDEAD_FUNCTION_NAME(XSocketGetSockOpt, 8);
    XDEAD_FUNCTION_NAME(XSocketGetSockName, 9);
    XDEAD_FUNCTION_NAME(XSocketBind, 11);
    XDEAD_FUNCTION_NAME(XSocketConnect, 12);
    XDEAD_FUNCTION_NAME(XSocketListen, 13);
    XDEAD_FUNCTION_NAME(XSocketAccept, 14);
    XDEAD_FUNCTION_NAME(XSocketSelect, 15);
    XDEAD_FUNCTION_NAME(XSocketRecv, 18);
    XDEAD_FUNCTION_NAME(XSocketSend, 22);
    XDEAD_FUNCTION_NAME(XSocketSendTo, 24);
    XDEAD_FUNCTION_NAME(XNetStartup, 51);
    XDEAD_FUNCTION_NAME(XNetCleanup, 52);
    XDEAD_FUNCTION_NAME(XNetRandom, 53);
    XDEAD_FUNCTION_NAME(XNetRegisterKey, 55);
    XDEAD_FUNCTION_NAME(XNetUnregisterKey, 56);
    XDEAD_FUNCTION_NAME(XNetXnAddrToInAddr, 57);
    XDEAD_FUNCTION_NAME(XNetServerToInAddr, 58);
    XDEAD_FUNCTION_NAME(XNetUnregisterInAddr, 63);
    XDEAD_FUNCTION_NAME(XNetDnsLookup, 67);
    XDEAD_FUNCTION_NAME(XNetDnsRelease, 68);
    XDEAD_FUNCTION_NAME(XNetQosListen, 69);
    XDEAD_FUNCTION_NAME(XNetQosLookup, 70);
    XDEAD_FUNCTION_NAME(XNetQosServiceLookup, 71);
    XDEAD_FUNCTION_NAME(XNetQosRelease, 72);
    XDEAD_FUNCTION_NAME(XNetGetTitleXnAddr, 73);
    XDEAD_FUNCTION_NAME(XNetGetEthernetLinkStatus, 75);
    XDEAD_FUNCTION_NAME(XNetQosGetListenStats, 77);
    XDEAD_FUNCTION_NAME(XNetSetSystemLinkPort, 84);
    XDEAD_FUNCTION_NAME(XNotifyPositionUI, 652);
    XDEAD_FUNCTION_NAME(XGetOverlappedExtendedError, 1082);
    XDEAD_FUNCTION_NAME(XLiveUninitialize, 5003);
    XDEAD_FUNCTION_NAME(XLiveOnDestroyDevice, 5006);
    XDEAD_FUNCTION_NAME(XLiveOnResetDevice, 5007);
    XDEAD_FUNCTION_NAME(XHVCreateEngine, 5008);
    XDEAD_FUNCTION_NAME(XLivePBufferAllocate, 5016);
    XDEAD_FUNCTION_NAME(XLivePBufferFree, 5017);
    XDEAD_FUNCTION_NAME(XLiveSetDebugLevel, 5031);
    XDEAD_FUNCTION_NAME(XShowCustomPlayerListUI, 5212);
    XDEAD_FUNCTION_NAME(XShowGuideUI, 5215);
    XDEAD_FUNCTION_NAME(XShowKeyboardUI, 5216);
    XDEAD_FUNCTION_NAME(XCloseHandle, 5251);
    XDEAD_FUNCTION_NAME(XShowGamerCardUI, 5252);
    XDEAD_FUNCTION_NAME(XEnumerate, 5256);
    XDEAD_FUNCTION_NAME(XLiveSignout, 5258);
    XDEAD_FUNCTION_NAME(XUserGetSigninState, 5262);
    XDEAD_FUNCTION_NAME(XUserGetName, 5263);
    XDEAD_FUNCTION_NAME(XUserCheckPrivilege, 5265);
    XDEAD_FUNCTION_NAME(XUserGetSigninInfo, 5267);
    XDEAD_FUNCTION_NAME(XNotifyCreateListener, 5270);
    XDEAD_FUNCTION_NAME(XShowPlayersUI, 5271);
    XDEAD_FUNCTION_NAME(XUserAwardGamerPicture, 5274);
    XDEAD_FUNCTION_NAME(XShowFriendsUI, 5275);
    XDEAD_FUNCTION_NAME(XUserSetContext, 5277);
    XDEAD_FUNCTION_NAME(XUserReadAchievementPicture, 5279);
    XDEAD_FUNCTION_NAME(XUserCreateAchievementEnumerator, 5280);
    XDEAD_FUNCTION_NAME(XUserReadStats, 5281);
    XDEAD_FUNCTION_NAME(XUserCreateStatsEnumeratorByRank, 5284);
    XDEAD_FUNCTION_NAME(XUserCreateStatsEnumeratorByXuid, 5286);
    XDEAD_FUNCTION_NAME(XUserSetContextEx, 5292);
    XDEAD_FUNCTION_NAME(XUserSetPropertyEx, 5293);
    XDEAD_FUNCTION_NAME(XLivePBufferGetByteArray, 5294);
    XDEAD_FUNCTION_NAME(XStringVerify, 5303);
    XDEAD_FUNCTION_NAME(XStorageUploadFromMemory, 5305);
    XDEAD_FUNCTION_NAME(XStorageEnumerate, 5306);
    XDEAD_FUNCTION_NAME(XOnlineStartup, 5310);
    XDEAD_FUNCTION_NAME(XOnlineCleanup, 5311);
    XDEAD_FUNCTION_NAME(XFriendsCreateEnumerator, 5312);
    XDEAD_FUNCTION_NAME(XPresenceInitialize, 5313);
    XDEAD_FUNCTION_NAME(XUserMuteListQuery, 5314);
    XDEAD_FUNCTION_NAME(XInviteGetAcceptedInfo, 5315);
    XDEAD_FUNCTION_NAME(XInviteSend, 5316);
    XDEAD_FUNCTION_NAME(XSessionSearchByID, 5320);
    XDEAD_FUNCTION_NAME(XSessionSearch, 5321);
    XDEAD_FUNCTION_NAME(XSessionModify, 5322);
    XDEAD_FUNCTION_NAME(XOnlineGetNatType, 5324);
    XDEAD_FUNCTION_NAME(XSessionJoinRemote, 5326);
    XDEAD_FUNCTION_NAME(XSessionJoinLocal, 5327);
    XDEAD_FUNCTION_NAME(XSessionGetDetails, 5328);
    XDEAD_FUNCTION_NAME(XSessionFlushStats, 5329);
    XDEAD_FUNCTION_NAME(XSessionDelete, 5330);
    XDEAD_FUNCTION_NAME(XUserReadProfileSettings, 5331);
    XDEAD_FUNCTION_NAME(XSessionArbitrationRegister, 5333);
    XDEAD_FUNCTION_NAME(XTitleServerCreateEnumerator, 5335);
    XDEAD_FUNCTION_NAME(XSessionLeaveRemote, 5336);
    XDEAD_FUNCTION_NAME(XUserWriteProfileSettings, 5337);
    XDEAD_FUNCTION_NAME(XPresenceSubscribe, 5338);
    XDEAD_FUNCTION_NAME(XPresenceCreateEnumerator, 5340);
    XDEAD_FUNCTION_NAME(XSessionModifySkill, 5342);
    XDEAD_FUNCTION_NAME(XSessionCalculateSkill, 5343);
    XDEAD_FUNCTION_NAME(XStorageBuildServerPath, 5344);
    XDEAD_FUNCTION_NAME(XStorageDownloadToMemory, 5345);
    XDEAD_FUNCTION_NAME(XLiveContentCreateAccessHandle, 5350);
    XDEAD_FUNCTION_NAME(XLiveContentGetPath, 5355);
    XDEAD_FUNCTION_NAME(XLiveContentGetDisplayName, 5356);
    XDEAD_FUNCTION_NAME(XLiveContentCreateEnumerator, 5360);
    XDEAD_FUNCTION_NAME(XShowMarketplaceUI, 5365);
    XDEAD_FUNCTION_NAME(XContentGetMarketplaceCounts, 5367);
    XDEAD_FUNCTION_NAME(XMarketplaceCreateOfferEnumerator, 5372);
    XDEAD_FUNCTION_NAME(XSocketNTOHS, 38);
    XDEAD_FUNCTION_NAME(XShowGameInviteUI, 5208);
    XDEAD_FUNCTION_NAME(XShowMessageComposeUI, 5209);
    XDEAD_FUNCTION_NAME(XShowFriendRequestUI, 5210);
    XDEAD_FUNCTION_NAME(XShowPlayerReviewUI, 5214);
    XDEAD_FUNCTION_NAME(XShowAchievementsUI, 5250);
    XDEAD_FUNCTION_NAME(XLiveSignin, 5259);
    XDEAD_FUNCTION_NAME(XShowSigninUI, 5260);
    XDEAD_FUNCTION_NAME(XSessionCreate, 5300);
    XDEAD_FUNCTION_NAME(XSessionStart, 5318);
    XDEAD_FUNCTION_NAME(XLiveProtectData, 5034);
    XDEAD_FUNCTION_NAME(XLiveUnprotectData, 5035);
    XDEAD_FUNCTION_NAME(XLiveCreateProtectedDataContext, 5036);
    XDEAD_FUNCTION_NAME(XLiveCloseProtectedDataContext, 5038);
    XDEAD_FUNCTION_NAME(XShowMessagesUI, 5206);
    XDEAD_FUNCTION_NAME(XUserAreUsersFriends, 5264);
    XDEAD_FUNCTION_NAME(XUserWriteAchievements, 5278);
    XDEAD_FUNCTION_NAME(XLiveInitializeEx, 5297);
    XDEAD_FUNCTION_NAME(XSessionWriteStats, 5317);
    XDEAD_FUNCTION_NAME(XSessionEnd, 5332);
    XDEAD_FUNCTION_NAME(XLiveSetSponsorToken, 5026);

    auto __stdcall add_listener(uint32_t ordinal, void* callback, uint32_t index) -> ListenerStatus;
    auto __stdcall remove_listener(uint32_t ordinal, void* callback) -> ListenerStatus;
    auto __stdcall remove_all_listeners() -> ListenerStatus;

    using add_listener_t = decltype(&add_listener);
    using remove_listener_t = decltype(&remove_listener);
    using remove_all_listeners_T = decltype(&remove_all_listeners);
}
