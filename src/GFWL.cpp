/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#include "GFWL.hpp"
#include "Console.hpp"
#include "Memory.hpp"
#include "Offsets.hpp"
#include "Platform.hpp"
#include "SDK.hpp"
#include <Windows.h>
#include <intrin.h>
#include <map>
#include <processthreadsapi.h>
#include <tlhelp32.h>
#include <winternl.h>

#define ThreadQuerySetWin32StartAddress 9
#define GFWL_MAIN_THREAD_INDEX 1

auto gfwlMemCheckPatch = Memory::Patch();
std::map<uint32_t, std::tuple<uintptr_t, const char*, uint32_t, uintptr_t>> addressesToUnhook = {};
bool suspended_gfwl_main_thread = false;

auto hook_gfwl(Memory::ModuleInfo& xlive) -> void;

DECL_DETOUR_API(signed int, __stdcall, xlive_3, int a1, signed int a2, signed int a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_4, int a1);
DECL_DETOUR_API(signed int, __stdcall, xlive_6, int a1, int a2, signed int* a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_7, int a1, int a2, signed int a3, int* a4, signed int a5);
DECL_DETOUR_API(int, __stdcall, xlive_8, int a1, int a2, int a3, void* a4, int a5);
DECL_DETOUR_API(int, __stdcall, xlive_9, int a1, int a2, int a3);
DECL_DETOUR_API(int, __stdcall, xlive_11, int a1, int a2, int a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_12, int a1, int a2, int a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_13, int a1, signed int a2);
DECL_DETOUR_API(uintptr_t, __stdcall, xlive_14, void* a1, int a2, int a3);
DECL_DETOUR_API(int, __stdcall, xlive_15, int a1, int a2, int a3, int a4, DWORD a5);
DECL_DETOUR_API(int, __stdcall, xlive_18, int a1, int a2, int a3, int a4);
DECL_DETOUR_API(signed int, __stdcall, xlive_20, int a1, int a2, int a3, char a4, int a5, DWORD* a6);
DECL_DETOUR_API(int, __stdcall, xlive_22, int a1, int a2, int a3, int a4);
DECL_DETOUR_API(signed int, __stdcall, xlive_24, int a1, int a2, int a3, int a4, int a5, int a6);
DECL_DETOUR_S(DWORD, __stdcall, xlive_27);
DECL_DETOUR_API(int, __stdcall, xlive_51, int a1);
DECL_DETOUR_S(int, __stdcall, xlive_52);
DECL_DETOUR_API(int, __stdcall, xlive_53, BYTE* a1, HCRYPTPROV a2);
DECL_DETOUR_API(signed int, __stdcall, xlive_55, int a1, void* a2);
DECL_DETOUR_API(signed int, __stdcall, xlive_56, const void* a1);
DECL_DETOUR_API(signed int, __stdcall, xlive_57, int a1, const void* a2, DWORD* a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_58, int a1, int a2, DWORD* a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_63, unsigned int a1);
DECL_DETOUR_API(signed int, __stdcall, xlive_67, int a1, int a2, DWORD* a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_68, int a1);
DECL_DETOUR_API(int, __stdcall, xlive_69, int a1, void* a2, int a3, int a4, int a5);
DECL_DETOUR_API(int, __fastcall, xlive_70, int ecx0, int edx0, int a1, int a2, int a3, int a4, int a5, int a6, int a7,
    int a8, int a9, int a10, int a11, int a12);
DECL_DETOUR_API(int, __fastcall, xlive_71, int ecx0, int edx0, int a1, int a2, int a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_72, int a1);
DECL_DETOUR_API(int, __stdcall, xlive_73, int a1);
DECL_DETOUR_S(unsigned int, __stdcall, xlive_75);
DECL_DETOUR_API(signed int, __stdcall, xlive_77, const void* a1, int a2);
DECL_DETOUR_API(int, __stdcall, xlive_84, int a1);
DECL_DETOUR_API(int, __stdcall, xlive_651, int a1, int a2, DWORD* a3, DWORD* a4);
DECL_DETOUR_API(int, __stdcall, xlive_652, LONG Value);
DECL_DETOUR_API(DWORD, __stdcall, xlive_1082, int a1);
DECL_DETOUR_API(DWORD, __stdcall, xlive_1083, int a1, DWORD* a2, int a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_5001, int a1);
DECL_DETOUR_S(unsigned int, __stdcall, xlive_5002);
DECL_DETOUR_API(int, __stdcall, xlive_5003);
DECL_DETOUR_S(signed int, __stdcall, xlive_5006);
DECL_DETOUR_API(signed int, __stdcall, xlive_5007, int a1);
DECL_DETOUR_API(signed int, __stdcall, xlive_5008, int a1, int a2, DWORD* a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_5016, int a1, unsigned int a2);
DECL_DETOUR_API(signed int, __stdcall, xlive_5017, int a1);
DECL_DETOUR_API(int, __stdcall, xlive_5030, HIMC);
DECL_DETOUR_API(int, __stdcall, xlive_5031, unsigned int a1, DWORD* a2);
DECL_DETOUR_API(int, __stdcall, xlive_5212, int, int, int, int, int, int, int, int, void* Src, void*, int, int);
DECL_DETOUR_API(signed int, __stdcall, xlive_5215, int a1);
DECL_DETOUR_API(signed int, __stdcall, xlive_5216, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
DECL_DETOUR_API(int, __stdcall, xlive_5251, HANDLE hObject);
DECL_DETOUR_API(signed int, __stdcall, xlive_5252, int a1, __int64 a2);
DECL_DETOUR_API(int, __stdcall, xlive_5256, int a1, int a2, int a3, DWORD* a4, int a5);
DECL_DETOUR_API(signed int, __stdcall, xlive_5258, int a1);
DECL_DETOUR_API(XUSER_SIGNIN_STATE, __stdcall, xlive_5262, int a1);
DECL_DETOUR_API(int, __stdcall, xlive_5263, uint32_t user_index, CHAR* user_name, uint32_t user_name_length);
DECL_DETOUR_API(int, __stdcall, xlive_5265, unsigned int a1, int a2, DWORD* a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_5267, uint32_t user_index, uint32_t flags, XUSER_SIGNIN_INFO* signin_info);
DECL_DETOUR_API(int, __stdcall, xlive_5270, __int64 a1);
DECL_DETOUR_API(signed int, __stdcall, xlive_5271, int a1);
DECL_DETOUR_API(signed int, __stdcall, xlive_5274, int a1, int a2, int a3, int a4);
DECL_DETOUR_API(signed int, __stdcall, xlive_5275, int a1);
DECL_DETOUR_API(signed int, __stdcall, xlive_5277, int a1, int a2, int a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_5279, int a1, int a2, int a3, int a4, int a5, int a6, int a7);
DECL_DETOUR_API(signed int, __stdcall, xlive_5280, int a1, int a2, int a3, unsigned int a4, int a5, unsigned int a6,
    unsigned int a7, int a8, int a9);
DECL_DETOUR_API(int, __stdcall, xlive_5281, int a1, unsigned int a2, int a3, unsigned int a4, int a5, unsigned int* a6,
    int a7, int a8);
DECL_DETOUR_API(int, __stdcall, xlive_5284, int, int, int, int, void* Src, int, int);
DECL_DETOUR_API(int, __stdcall, xlive_5286, int, int, int, int, int, void* Src, int, int);
DECL_DETOUR_API(int, __stdcall, xlive_5292, int a1, int a2, int a3, int a4);
DECL_DETOUR_API(int, __stdcall, xlive_5293, int a1, int a2, int a3, int a4, int a5);
DECL_DETOUR_API(int, __stdcall, xlive_5294, int a1, unsigned int a2, int a3, int a4);
DECL_DETOUR_API(int, __stdcall, xlive_5303, int a1, const char* a2, int a3, int a4, unsigned int a5, int a6, int a7);
DECL_DETOUR_API(int, __stdcall, xlive_5305, char a1, const unsigned __int16* a2, char a3, int a4, int a5);
DECL_DETOUR_API(
    int, __stdcall, xlive_5306, int a1, const unsigned __int16* a2, int a3, int a4, unsigned int a5, int a6, int a7);
DECL_DETOUR_S(int, __stdcall, xlive_5310);
DECL_DETOUR_S(int, __stdcall, xlive_5311);
DECL_DETOUR_API(signed int, __stdcall, xlive_5312, int a1, int a2, int a3, int a4, int a5);
DECL_DETOUR_API(signed int, __stdcall, xlive_5313, int a1);
DECL_DETOUR_API(signed int, __stdcall, xlive_5314, int a1, int a2, int a3, int a4);
DECL_DETOUR_API(signed int, __stdcall, xlive_5315, int a1, int a2);
DECL_DETOUR_API(int, __stdcall, xlive_5316, int a1, int a2, int a3, wchar_t* a4, int a5);
DECL_DETOUR_API(int, __stdcall, xlive_5320, int a1, int a2, int a3, int* a4, int a5, int a6);
DECL_DETOUR_API(int, __stdcall, xlive_5321, int a1, int a2, int a3, unsigned __int16 a4, __int16 a5, int a6, int a7,
    unsigned int* a8, int a9, int a10);
DECL_DETOUR_API(signed int, __stdcall, xlive_5322, int a1, int a2, int a3, int a4, int a5);
DECL_DETOUR_T(signed int, xlive_5324, void* a1);
DECL_DETOUR_API(unsigned int, __stdcall, xlive_5326, int a1, int a2, int a3, int a4, int a5);
DECL_DETOUR_API(signed int, __stdcall, xlive_5327, int a1, int a2, int* a3, int a4, int a5);
DECL_DETOUR_API(signed int, __stdcall, xlive_5328, int a1, DWORD* a2, int a3, int a4);
DECL_DETOUR_API(signed int, __stdcall, xlive_5329, int a1, int a2);
DECL_DETOUR_API(uintptr_t, __stdcall, xlive_5330, int a1, int a2);
DECL_DETOUR_API(signed int, __stdcall, xlive_5331, int a1, int a2, int a3, int a4, DWORD* a5, int a6, int a7);
DECL_DETOUR_API(signed int, __stdcall, xlive_5333, int a1, int a2, int a3, int a4, int* a5, int a6, int a7);
DECL_DETOUR_API(unsigned int, __stdcall, xlive_5335, void* Src, int a2, int a3, int a4);
DECL_DETOUR_API(signed int, __stdcall, xlive_5336, int a1, int a2, int a3, int a4);
DECL_DETOUR_API(signed int, __stdcall, xlive_5337, int a1, unsigned int a2, int a3, int a4);
DECL_DETOUR_API(signed int, __stdcall, xlive_5338, int a1, int a2, int a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_5340, int a1, int a2, int a3, int a4, int a5, int a6, int a7);
DECL_DETOUR_API(signed int, __stdcall, xlive_5342, int a1, unsigned int a2, int a3, int a4);
DECL_DETOUR_API(signed int, __stdcall, xlive_5343, unsigned int a1, int a2, double* a3, double* a4, double* a5);
DECL_DETOUR_API(signed int, __stdcall, xlive_5344, int a1, int a2, int a3, unsigned int a4, int a5, int a6, int a7);
DECL_DETOUR_API(
    int, __stdcall, xlive_5345, int a1, const unsigned __int16* a2, int a3, int a4, unsigned int a5, int a6, int a7);
DECL_DETOUR_API(int, __stdcall, xlive_5350, int a1, void* a2, int a3, int a4, int a5, int a6, int a7);
DECL_DETOUR_API(int, __stdcall, xlive_5355, int a1, int a2, char* a3, unsigned int* a4);
DECL_DETOUR_API(int, __stdcall, xlive_5356, int a1, int a2, char* a3, unsigned int* a4);
DECL_DETOUR_API(signed int, __stdcall, xlive_5360, unsigned int a1, int a2, DWORD* a3, int* a4);
DECL_DETOUR_API(signed int, __stdcall, xlive_5365, int a1, int a2, int a3, int a4, int a5);
DECL_DETOUR_API(int, __stdcall, xlive_5367, int a1, int a2, unsigned int a3, int a4, int a5);
DECL_DETOUR_API(int, __stdcall, xlive_5372, HANDLE hObject, int, int, int, int, int);

DECL_DETOUR_API(__int16, __stdcall, xlive_38, __int16 a1);
DECL_DETOUR_API(int, __stdcall, xlive_5208, int a1, int a2, int a3, int a4);
DECL_DETOUR_API(int, __stdcall, xlive_5209, int a1, int a2, int a3, int a4);
DECL_DETOUR_API(int, __stdcall, xlive_5210, int a1, int a2);
DECL_DETOUR_API(int, __stdcall, xlive_5214, int a1, int a2);
DECL_DETOUR_API(int, __stdcall, xlive_5250, int a1);
DECL_DETOUR_API(int, __stdcall, xlive_5259, int a1, int a2, int a3, int a4);
DECL_DETOUR_API(int, __stdcall, xlive_5260, int a1, int a2);
DECL_DETOUR_API(int, __stdcall, xlive_5300, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
DECL_DETOUR_API(int, __stdcall, xlive_5318, int a1, int a2, int a3);
DECL_DETOUR_API(int, __stdcall, xlive_5034, int a1, int a2, int a3, int a4, int a5);
DECL_DETOUR_API(int, __stdcall, xlive_5035, BYTE* protected_data, DWORD size_of_protected_data, BYTE* unprotected_data,
    DWORD* size_of_data, HANDLE* protected_data_handle);
DECL_DETOUR_API(int, __stdcall, xlive_5036, int a1, int a2);
//DECL_DETOUR_MID(xlive_5034);
//DECL_DETOUR_MID(xlive_5035);
//DECL_DETOUR_MID(xlive_5036);
DECL_DETOUR_API(int, __stdcall, xlive_5038, int a1);
//DECL_DETOUR_MID(xlive_5038);
DECL_DETOUR_API(int, __stdcall, xlive_5206, int a1);
DECL_DETOUR_API(int, __stdcall, xlive_5264, int a1, int a2, int a3, int a4, int a5);
DECL_DETOUR_API(int, __stdcall, xlive_5278, int a1, int a2, int a3);
DECL_DETOUR_API(int, __stdcall, xlive_5297, int a1, int a2);
DECL_DETOUR_API(int, __stdcall, xlive_5317, int a1, int a2, int a3, int a4, int a5);
DECL_DETOUR_API(int, __stdcall, xlive_5332, int a1, int a2);

auto change_gfwl_main_thread(bool suspend) -> bool;

auto patch_gfwl() -> void
{
    auto xlive = Memory::ModuleInfo();

    if (!Memory::TryGetModule("xlive.dll", &xlive)) {
        return println("[gfwl] Unable to find GFWL module :(");
    }

    // Modify memory modification check
    //      Original: 0F 84 (jz)
    //      Patch:    90 E9 (nop jmp)

    unsigned char nop_jmp[2] = { 0x90, 0xE9 };
    if (gfwlMemCheckPatch.Execute(xlive.base + Offsets::xlive_memory_check, nop_jmp, sizeof(nop_jmp))) {
        println("[gfwl] Patched xlive.dll memory check at 0x{:04x}", gfwlMemCheckPatch.GetLocation());
    } else {
        return println("[gfwl] Unable to patch memory check :(");
    }

    hook_gfwl(xlive);
    //change_gfwl_main_thread(true);
}

auto hook_gfwl(Memory::ModuleInfo& xlive) -> void
{
    auto handle = GetModuleHandleA(xlive.path);
    println("[gfwl] xlive.dll handle 0x{:04x}", uintptr_t(handle));

#define HOOK_ORDINAL(ordinal, name)                                                                                    \
    auto ordinal_##ordinal##_address = uintptr_t(GetProcAddress(handle, LPCSTR(ordinal)));                             \
    if (ordinal_##ordinal##_address) {                                                                                 \
        addressesToUnhook.insert_or_assign(ordinal, std::make_tuple(ordinal_##ordinal##_address, name, 0, 0));         \
        Hooks::queue(name, &xlive_##ordinal, xlive_##ordinal##_Hook, ordinal_##ordinal##_address);                     \
    } else {                                                                                                           \
        println("[gfwl] Unable to get address of xlive_" #ordinal);                                                    \
    }

#define HOOK_IAT_WRAPPER(wrapper_address, ordinal, name)                                                               \
    addressesToUnhook.insert_or_assign(ordinal, std::make_tuple(wrapper_address, name, 0, 0));                         \
    auto xlive_##ordinal##_original = **(uintptr_t**)(wrapper_address + 2);                                            \
    Hooks::queue(name, &xlive_##ordinal, xlive_##ordinal##_Hook, wrapper_address);                                     \
    xlive_##ordinal = _xlive_##ordinal(xlive_##ordinal##_original);

#define HOOK_IAT_WRAPPER_MID(wrapper_address, ordinal, name)                                                           \
    addressesToUnhook.insert_or_assign(ordinal, std::make_tuple(wrapper_address, name, 0, 0));                         \
    auto xlive_##ordinal##_original = **(uintptr_t**)(wrapper_address + 2);                                            \
    Hooks::queue(name, &xlive_##ordinal, xlive_##ordinal##_Hook, wrapper_address);                                     \
    xlive_##ordinal = xlive_##ordinal##_original;

    HOOK_ORDINAL(3, "XSocketCreate");
    HOOK_ORDINAL(4, "XSocketClose");
    HOOK_ORDINAL(6, "XSocketIOCTLSocket");
    HOOK_ORDINAL(7, "XSocketSetSockOpt");
    HOOK_ORDINAL(8, "XSocketGetSockOpt");
    HOOK_ORDINAL(9, "XSocketGetSockName");
    HOOK_ORDINAL(11, "XSocketBind");
    HOOK_ORDINAL(12, "XSocketConnect");
    HOOK_ORDINAL(13, "XSocketListen");
    HOOK_ORDINAL(14, "XSocketAccept");
    HOOK_ORDINAL(15, "XSocketSelect");
    HOOK_ORDINAL(18, "XSocketRecv"); // called frequently
    HOOK_ORDINAL(20, "XSocketRecvFrom"); // called frequently when in multiplayer lobby
    HOOK_ORDINAL(22, "XSocketSend"); // called frequently
    HOOK_ORDINAL(24, "XSocketSendTo");
    HOOK_ORDINAL(27, "XSocketWSAGetLastError"); // called frequently when online
    HOOK_ORDINAL(51, "XNetStartup");
    HOOK_ORDINAL(52, "XNetCleanup");
    HOOK_ORDINAL(53, "XNetRandom");
    HOOK_ORDINAL(55, "XNetRegisterKey");
    HOOK_ORDINAL(56, "XNetUnregisterKey");
    HOOK_ORDINAL(57, "XNetXnAddrToInAddr");
    HOOK_ORDINAL(58, "XNetServerToInAddr");
    HOOK_ORDINAL(63, "XNetUnregisterInAddr");
    HOOK_ORDINAL(67, "XNetDnsLookup");
    HOOK_ORDINAL(68, "XNetDnsRelease");
    HOOK_ORDINAL(69, "XNetQosListen");
    HOOK_ORDINAL(70, "XNetQosLookup");
    HOOK_ORDINAL(71, "XNetQosServiceLookup");
    HOOK_ORDINAL(72, "XNetQosRelease");
    HOOK_ORDINAL(73, "XNetGetTitleXnAddr");
    HOOK_ORDINAL(75, "XNetGetEthernetLinkStatus");
    HOOK_ORDINAL(77, "XNetQosGetListenStats");
    HOOK_ORDINAL(84, "XNetSetSystemLinkPort");
    HOOK_ORDINAL(651, "XNotifyGetNext"); // called frequently
    HOOK_ORDINAL(652, "XNotifyPositionUI");
    HOOK_ORDINAL(1082, "XGetOverlappedExtendedError"); // called frequently when online
    HOOK_ORDINAL(1083, "XGetOverlappedResult"); // called frequently when online
    HOOK_ORDINAL(5001, "XLiveInput"); // called frequently
    HOOK_ORDINAL(5002, "XLiveRender"); // called frequently
    HOOK_ORDINAL(5003, "XLiveUninitialize"); // manually reconstructed
    HOOK_ORDINAL(5006, "XLiveOnDestroyDevice");
    HOOK_ORDINAL(5007, "XLiveOnResetDevice");
    HOOK_ORDINAL(5008, "XHVCreateEngine");
    HOOK_ORDINAL(5016, "XLivePBufferAllocate"); // manually reconstructed
    HOOK_ORDINAL(5017, "XLivePBufferFree"); // manually reconstructed
    HOOK_ORDINAL(5030, "XLivePreTranslateMessage"); // called frequently
    HOOK_ORDINAL(5031, "XLiveSetDebugLevel");
    HOOK_ORDINAL(5212, "XShowCustomPlayerListUI"); // called frequently
    HOOK_ORDINAL(5215, "XShowGuideUI");
    HOOK_ORDINAL(5216, "XShowKeyboardUI");
    HOOK_ORDINAL(5251, "XCloseHandle"); // called frequently acquiring a handle
    HOOK_ORDINAL(5252, "XShowGamerCardUI");
    HOOK_ORDINAL(5256, "XEnumerate");
    HOOK_ORDINAL(5258, "XLiveSignout");
    HOOK_ORDINAL(5262, "XUserGetSigninState"); // called frequently
    HOOK_ORDINAL(5263, "XUserGetName");
    HOOK_ORDINAL(5265, "XUserCheckPrivilege");
    HOOK_ORDINAL(5267, "XUserGetSigninInfo");
    HOOK_ORDINAL(5270, "XNotifyCreateListener");
    HOOK_ORDINAL(5271, "XShowPlayersUI");
    HOOK_ORDINAL(5274, "XUserAwardGamerPicture");
    //HOOK_ORDINAL(5275, "XShowFriendsUI"); // manually reconstructed, TODO: this crashes
    HOOK_ORDINAL(5277, "XUserSetContext");
    HOOK_ORDINAL(5279, "XUserReadAchievementPicture");
    HOOK_ORDINAL(5280, "XUserCreateAchievementEnumerator");
    HOOK_ORDINAL(5281, "XUserReadStats");
    HOOK_ORDINAL(5284, "XUserCreateStatsEnumeratorByRank");
    HOOK_ORDINAL(5286, "XUserCreateStatsEnumeratorByXuid");
    HOOK_ORDINAL(5292, "XUserSetContextEx");
    HOOK_ORDINAL(5293, "XUserSetPropertyEx");
    HOOK_ORDINAL(5294, "XLivePBufferGetByteArray"); // manually reconstructed
    HOOK_ORDINAL(5303, "XStringVerify");
    HOOK_ORDINAL(5305, "XStorageUploadFromMemory");
    HOOK_ORDINAL(5306, "XStorageEnumerate");
    HOOK_ORDINAL(5310, "XOnlineStartup");
    HOOK_ORDINAL(5311, "XOnlineCleanup");
    HOOK_ORDINAL(5312, "XFriendsCreateEnumerator");
    HOOK_ORDINAL(5313, "XPresenceInitialize");
    HOOK_ORDINAL(5314, "XUserMuteListQuery");
    HOOK_ORDINAL(5315, "XInviteGetAcceptedInfo");
    HOOK_ORDINAL(5316, "XInviteSend");
    HOOK_ORDINAL(5320, "XSessionSearchByID");
    HOOK_ORDINAL(5321, "XSessionSearch");
    HOOK_ORDINAL(5322, "XSessionModify");
    HOOK_ORDINAL(5324, "XOnlineGetNatType");
    HOOK_ORDINAL(5326, "XSessionJoinRemote");
    HOOK_ORDINAL(5327, "XSessionJoinLocal");
    HOOK_ORDINAL(5328, "XSessionGetDetails");
    HOOK_ORDINAL(5329, "XSessionFlushStats");
    HOOK_ORDINAL(5330, "XSessionDelete"); // manually reconstructed
    HOOK_ORDINAL(5331, "XUserReadProfileSettings");
    HOOK_ORDINAL(5333, "XSessionArbitrationRegister");
    HOOK_ORDINAL(5335, "XTitleServerCreateEnumerator");
    HOOK_ORDINAL(5336, "XSessionLeaveRemote");
    HOOK_ORDINAL(5337, "XUserWriteProfileSettings");
    HOOK_ORDINAL(5338, "XPresenceSubscribe");
    HOOK_ORDINAL(5340, "XPresenceCreateEnumerator");
    HOOK_ORDINAL(5342, "XSessionModifySkill");
    HOOK_ORDINAL(5343, "XSessionCalculateSkill");
    HOOK_ORDINAL(5344, "XStorageBuildServerPath");
    HOOK_ORDINAL(5345, "XStorageDownloadToMemory");
    HOOK_ORDINAL(5350, "XLiveContentCreateAccessHandle");
    HOOK_ORDINAL(5355, "XLiveContentGetPath");
    HOOK_ORDINAL(5356, "XLiveContentGetDisplayName");
    HOOK_ORDINAL(5360, "XLiveContentCreateEnumerator");
    HOOK_ORDINAL(5365, "XShowMarketplaceUI"); // manually reconstructed
    HOOK_ORDINAL(5367, "XContentGetMarketplaceCounts");
    HOOK_ORDINAL(5372, "XMarketplaceCreateOfferEnumerator");

    // These failed static analysis caused by obfuscation :>

    HOOK_ORDINAL(38, "XSocketNTOHS");
    HOOK_ORDINAL(5208, "XShowGameInviteUI");
    HOOK_ORDINAL(5209, "XShowMessageComposeUI");
    HOOK_ORDINAL(5250, "XShowAchievementsUI");
    HOOK_ORDINAL(5259, "XLiveSignin");
    HOOK_ORDINAL(5260, "XShowSigninUI");
    HOOK_ORDINAL(5210, "XShowFriendRequestUI"); // weird, might work
    HOOK_ORDINAL(5214, "XShowPlayerReviewUI"); // weird, might work
    HOOK_ORDINAL(5300, "XSessionCreate"); // weird, might work
    HOOK_ORDINAL(5318, "XSessionStart"); // weird, might work

    // These are heavily obfuscated and protected

    //HOOK_IAT_WRAPPER_MID(0x1A33C6C, 5034, "XLiveProtectData"); // called twice when creating a save
    //HOOK_IAT_WRAPPER_MID(0x1A33C66, 5035, "XLiveUnprotectData"); // called when loading a save
    //HOOK_IAT_WRAPPER_MID(0x1A33C72, 5036, "XLiveCreateProtectedDataContext"); // called when creating a save
    //HOOK_IAT_WRAPPER_MID(0x1A33C60, 5038, "XLiveCloseProtectedDataContext"); // called when saving/loading a save

    HOOK_IAT_WRAPPER(0x1A33C6C, 5034, "XLiveProtectData"); // called twice when creating a save
    HOOK_IAT_WRAPPER(0x1A33C66, 5035, "XLiveUnprotectData"); // called when loading a save
    HOOK_IAT_WRAPPER(0x1A33C72, 5036, "XLiveCreateProtectedDataContext"); // called when creating a save
    HOOK_IAT_WRAPPER(0x1A33C60, 5038, "XLiveCloseProtectedDataContext"); // called when saving/loading a save

    HOOK_IAT_WRAPPER(0x1A33A50, 5206, "XShowMessagesUI");
    HOOK_IAT_WRAPPER(0x1A33A3E, 5264, "XUserAreUsersFriends");
    HOOK_IAT_WRAPPER(0x1A33C30, 5278, "XUserWriteAchievements");
    HOOK_IAT_WRAPPER(0x1A33B94, 5297, "XLiveInitializeEx");
    HOOK_IAT_WRAPPER(0x1A33B04, 5317, "XSessionWriteStats");
    HOOK_IAT_WRAPPER(0x1A33BE8, 5332, "XSessionEnd");
}

auto change_gfwl_main_thread(bool suspend) -> bool
{
    auto xlive = Memory::ModuleInfo();
    if (!Memory::TryGetModule("xlive.dll", &xlive)) {
        println("[gfwl] Unable to find GFWL module :(");
        return false;
    }

    auto ntdll = Memory::ModuleInfo();
    if (!Memory::TryGetModule("ntdll.dll", &ntdll)) {
        println("[gfwl] Unable to find ntdll.dll module :(");
        return false;
    }

    auto ntdll_NtQueryInformationThread
        = Memory::GetSymbolAddress<decltype(NtQueryInformationThread)*>(ntdll.base, "NtQueryInformationThread");

    if (!ntdll_NtQueryInformationThread) {
        println("[gfwl] Unable to find NtQueryInformationThread :(");
        return false;
    }

    auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

    auto index = 0;
    auto main_thread_start_address = xlive.base + Offsets::xlive_main_thread_start_address;

    auto process = GetCurrentProcess();
    auto process_id = GetCurrentProcessId();

    auto entry = THREADENTRY32();
    entry.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(snapshot, &entry)) {
        do {
            if (entry.th32OwnerProcessID == process_id) {
                auto handle = OpenThread(THREAD_ALL_ACCESS, false, entry.th32ThreadID);
                if (handle) {
                    auto start_address = 0ul;

                    ntdll_NtQueryInformationThread(handle, (THREADINFOCLASS)ThreadQuerySetWin32StartAddress,
                        &start_address, sizeof(start_address), NULL);

                    if (start_address == main_thread_start_address) {
                        if (index++ == GFWL_MAIN_THREAD_INDEX) {
                            auto result = suspend ? SuspendThread(handle) : ResumeThread(handle);
                            if (result != -1) {
                                println("[gfwl] {} main thread 0x{:04x} of start address 0x{:04x}",
                                    suspend ? "Suspended" : "Resumed", entry.th32ThreadID, start_address);
                                return true;
                            }
                            break;
                        }
                    }

                    CloseHandle(handle);
                }
            }
        } while (Thread32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return false;
}

auto unpatch_gfwl() -> void
{
    if (suspended_gfwl_main_thread) {
        change_gfwl_main_thread(false);
    }

    addressesToUnhook.clear();

    if (gfwlMemCheckPatch.Restore()) {
        println("[gfwl] Restored mem check patch");
    }
}

auto __forceinline print_stack(int parameters) -> void
{
    auto retAddress = uintptr_t(_AddressOfReturnAddress());
    for (int i = 12; i <= 12 + parameters + 1; ++i) {
        auto stack = retAddress + (i << 2);
        println("[gfwl] stack(0x{:04x}) -> 0x{:08x} | {}", stack, *(uintptr_t*)stack, *(uintptr_t*)stack);
    }
}

auto log_xlive_call(uint32_t ordinal) -> void
{
    auto& address = addressesToUnhook.at(ordinal);

    //auto& calls = std::get<2>(address);
    //if (calls++ > 10) {
    //    return;
    //}

    auto function = std::get<0>(address);
    auto name = std::get<1>(address);
    println("[gfwl] {} (xlive_{} at 0x{:04x})", name, ordinal, function);
}

auto xlive_debug_break(bool resume_main_thread = true) -> void
{
    if (IsDebuggerPresent()) {
        return;
    }

    change_gfwl_main_thread(true);

    println("[gfwl] waiting for debugger to attach...");

    while (!IsDebuggerPresent()) {
        Sleep(420);
    }

    DebugBreak();

    if (resume_main_thread) {
        change_gfwl_main_thread(false);
    }
}

#define SAVE_REGISTERS() __asm {\
   __asm pushad \
   __asm pushfd }
#define LOAD_REGISTERS() __asm {\
   __asm popfd \
   __asm popad }
#define JUMP_TO(address) __asm jmp address

auto __skip_call = false;
auto __skip_rv = 0;

#define NEVER_SKIP_AND_RETURN(rv) {};
#define SKIP_AND_RETURN(rv) \
    auto __skip_call = true; \
    auto __skip_rv = rv;
//#define SKIP_AND_RETURN(rv) {};

#define CALL_ORIGINAL(_ordinal, _name, ...)                                                                            \
    auto ordinal = _ordinal;                                                                                           \
    auto name = _name;                                                                                                 \
    auto original = xlive_##_ordinal;                                                                                  \
    if (__skip_call) {                                                                                           \
        return __skip_rv;                                                                                              \
    }                                                                                                                  \
    auto result = original(##__VA_ARGS__)

#define CALL_ORIGINAL_AND_RETURN(ordinal, name, ...)                                                                   \
    CALL_ORIGINAL(ordinal, name, ##__VA_ARGS__);                                                                       \
    return result

#define LOG_AND_RETURN(fmt, ...)                                                                                       \
    println("[gfwl] [{:04}] [0x{:x}] [0x{:x}] {}(" fmt ") -> {:x} | {}", ordinal, uintptr_t(original),                 \
        uintptr_t(_ReturnAddress()), name, ##__VA_ARGS__, result, result);                                             \
    return result

// clang-format off

typedef uintptr_t hex;

#define dptr(ptr) \
    ptr ? *(uintptr_t*)ptr : 0

DETOUR_API(signed int, __stdcall, xlive_3, int a1, signed int a2, signed int a3)
{
    CALL_ORIGINAL(3, "XSocketCreate", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_4, int a1)
{
    CALL_ORIGINAL(4, "XSocketClose", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(signed int, __stdcall, xlive_6, int a1, int a2, signed int* a3)
{
    CALL_ORIGINAL(6, "XSocketIOCTLSocket", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_7, int a1, int a2, signed int a3, int* a4, signed int a5)
{
    CALL_ORIGINAL(7, "XSocketSetSockOpt", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(int, __stdcall, xlive_8, int a1, int a2, int a3, void* a4, int a5)
{
    CALL_ORIGINAL(8, "XSocketGetSockOpt", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(int, __stdcall, xlive_9, int a1, int a2, int a3)
{
    CALL_ORIGINAL(9, "XSocketGetSockName", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(int, __stdcall, xlive_11, int a1, int a2, int a3)
{
    CALL_ORIGINAL(11, "XSocketBind", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_12, int a1, int a2, int a3)
{
    CALL_ORIGINAL(12, "XSocketConnect", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_13, int a1, signed int a2)
{
    CALL_ORIGINAL(13, "XSocketListen", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(uintptr_t, __stdcall, xlive_14, void* a1, int a2, int a3)
{
    CALL_ORIGINAL(14, "XSocketAccept", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(int, __stdcall, xlive_15, int a1, int a2, int a3, int a4, DWORD a5)
{
    CALL_ORIGINAL(15, "XSocketSelect", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(int, __stdcall, xlive_18, int a1, int a2, int a3, int a4)
{
    // called frequently
    CALL_ORIGINAL(18, "XSocketRecv", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(signed int, __stdcall, xlive_20, int a1, int a2, int a3, char a4, int a5, DWORD* a6)
{
    SKIP_AND_RETURN(-1);
    // called frequently when in multiplayer lobby
    CALL_ORIGINAL_AND_RETURN(20, "XSocketRecvFrom", a1, a2, a3, a4, a5, a6);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6));
}
DETOUR_API(int, __stdcall, xlive_22, int a1, int a2, int a3, int a4)
{
    CALL_ORIGINAL(22, "XSocketSend", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(signed int, __stdcall, xlive_24, int a1, int a2, int a3, int a4, int a5, int a6)
{
    CALL_ORIGINAL(24, "XSocketSendTo", a1, a2, a3, a4, a5, a6);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6));
}
DETOUR_API(DWORD, __stdcall, xlive_27)
{
    SKIP_AND_RETURN(0);
    // called frequently when online
    CALL_ORIGINAL_AND_RETURN(27, "XSocketWSAGetLastError");
}
DETOUR_API(int, __stdcall, xlive_51, int a1)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(51, "XNetStartup", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(int, __stdcall, xlive_52)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(52, "XNetCleanup");
    LOG_AND_RETURN("");
}
DETOUR_API(int, __stdcall, xlive_53, BYTE* a1, HCRYPTPROV a2)
{
    CALL_ORIGINAL(53, "XNetRandom", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(signed int, __stdcall, xlive_55, int a1, void* a2)
{
    CALL_ORIGINAL(55, "XNetRegisterKey", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(signed int, __stdcall, xlive_56, const void* a1)
{
    CALL_ORIGINAL(56, "XNetUnregisterKey", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(signed int, __stdcall, xlive_57, int a1, const void* a2, DWORD* a3)
{
    CALL_ORIGINAL(57, "XNetXnAddrToInAddr", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_58, int a1, int a2, DWORD* a3)
{
    CALL_ORIGINAL(58, "XNetServerToInAddr", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_63, unsigned int a1)
{
    CALL_ORIGINAL(63, "XNetUnregisterInAddr", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(signed int, __stdcall, xlive_67, int a1, int a2, DWORD* a3)
{
    CALL_ORIGINAL(67, "XNetDnsLookup", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_68, int a1)
{
    CALL_ORIGINAL(68, "XNetDnsRelease", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(int, __stdcall, xlive_69, int a1, void* a2, int a3, int a4, int a5)
{
    CALL_ORIGINAL(69, "XNetQosListen", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(int, __fastcall, xlive_70, int ecx0, int edx0, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12)
{
    CALL_ORIGINAL(70, "XNetQosLookup", ecx0, edx0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(ecx0), hex(edx0), hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7), hex(a8), hex(a9), hex(a10), hex(a11), hex(a12));
}
DETOUR_API(int, __fastcall, xlive_71, int ecx0, int edx0, int a1, int a2, int a3)
{
    CALL_ORIGINAL(71, "XNetQosServiceLookup", ecx0, edx0, a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(ecx0), hex(edx0), hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_72, int a1)
{
    CALL_ORIGINAL(72, "XNetQosRelease", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(int, __stdcall, xlive_73, int a1)
{
    CALL_ORIGINAL(73, "XNetGetTitleXnAddr", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(unsigned int, __stdcall, xlive_75)
{
    CALL_ORIGINAL(75, "XNetGetEthernetLinkStatus");
    LOG_AND_RETURN("");
}
DETOUR_API(signed int, __stdcall, xlive_77, const void* a1, int a2)
{
    CALL_ORIGINAL(77, "XNetQosGetListenStats", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(int, __stdcall, xlive_84, int a1)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(84, "XNetSetSystemLinkPort", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(int, __stdcall, xlive_651, int a1, int a2, DWORD* a3, DWORD* a4)
{
    SKIP_AND_RETURN(0);
    // Called frequently
    CALL_ORIGINAL_AND_RETURN(651, "XNotifyGetNext", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(int, __stdcall, xlive_652, LONG Value)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(652, "XNotifyPositionUI", Value);
    LOG_AND_RETURN("{:x}", hex(Value));
}
DETOUR_API(DWORD, __stdcall, xlive_1082, int a1)
{
    SKIP_AND_RETURN(0);
    // called on login, frequently when online
    CALL_ORIGINAL(1082, "XGetOverlappedExtendedError", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(DWORD, __stdcall, xlive_1083, int a1, DWORD* a2, int a3)
{
    SKIP_AND_RETURN(0);
    // called on login or when gfwl ui opens, frequently when online
    CALL_ORIGINAL_AND_RETURN(1083, "XGetOverlappedResult", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_5001, int a1)
{
    NEVER_SKIP_AND_RETURN(0);
    // frequent call
    CALL_ORIGINAL_AND_RETURN(5001, "XLiveInput", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(unsigned int, __stdcall, xlive_5002)
{
    NEVER_SKIP_AND_RETURN(0);
    // frequent call
    CALL_ORIGINAL_AND_RETURN(5002, "XLiveRender");
}
DETOUR_API(int, __stdcall, xlive_5003)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5003, "XLiveUninitialize");
    LOG_AND_RETURN("");
}
DETOUR_API(signed int, __stdcall, xlive_5006)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5006, "XLiveOnDestroyDevice");
    LOG_AND_RETURN("");
}
DETOUR_API(signed int, __stdcall, xlive_5007, int a1)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5007, "XLiveOnResetDevice", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(signed int, __stdcall, xlive_5008, int a1, int a2, DWORD* a3)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5008, "XHVCreateEngine", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_5016, int a1, unsigned int a2)
{
    CALL_ORIGINAL(5016, "XLivePBufferAllocate", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(signed int, __stdcall, xlive_5017, int a1)
{
    CALL_ORIGINAL(5017, "XLivePBufferFree", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(int, __stdcall, xlive_5030, HIMC a1)
{
    SKIP_AND_RETURN(0);
    // frequently called when window focused
    CALL_ORIGINAL_AND_RETURN(5030, "XLivePreTranslateMessage", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(int, __stdcall, xlive_5031, unsigned int a1, DWORD* a2)
{
    // Wait, this sounds interesting...
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5031, "XLiveSetDebugLevel", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(int, __stdcall, xlive_5212, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, void* Src, void* a10, int a11, int a12)
{
    // called frequently on login
    CALL_ORIGINAL(5212, "XShowCustomPlayerListUI", a1, a2, a3, a4, a5, a6, a7, a8, Src, a10, a11, a12);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7), hex(a8), hex(Src), hex(a10), hex(a11), hex(a12));
}
DETOUR_API(signed int, __stdcall, xlive_5215, int a1)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5215, "XShowGuideUI", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(signed int, __stdcall, xlive_5216, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
    CALL_ORIGINAL(5216, "XShowKeyboardUI", a1, a2, a3, a4, a5, a6, a7, a8);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7), hex(a8));
}
DETOUR_API(int, __stdcall, xlive_5251, HANDLE hObject)
{
    SKIP_AND_RETURN(1);
    // called frequently acquiring a handle
    CALL_ORIGINAL(5251, "XCloseHandle", hObject);
    LOG_AND_RETURN("{:x}", hex(hObject));
}
DETOUR_API(signed int, __stdcall, xlive_5252, int a1, __int64 a2)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5252, "XShowGamerCardUI", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), __int64(a2));
}
DETOUR_API(int, __stdcall, xlive_5256, int a1, int a2, int a3, DWORD* a4, int a5)
{
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

    SKIP_AND_RETURN(0);
    // called after login
    CALL_ORIGINAL(5256, "XEnumerate", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(signed int, __stdcall, xlive_5258, int a1)
{
    CALL_ORIGINAL(5258, "XLiveSignout", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(XUSER_SIGNIN_STATE, __stdcall, xlive_5262, int a1)
{
    // The game makes exactly seven calls to this function before it can log into an account.

    static auto calls_to_user_get_signin_state = 0;

    auto can_login = calls_to_user_get_signin_state >= 7;
    if (!can_login) {
        calls_to_user_get_signin_state += 1;
    }

    auto loginState =  can_login ? XUSER_SIGNIN_STATE::SignedInLocally : XUSER_SIGNIN_STATE::NotSignedIn;
    println("[gfwl] Forcing XUserGetSigninState -> {}", loginState);

    SKIP_AND_RETURN(loginState);

    // called on login and then frequently after
    CALL_ORIGINAL(5262, "XUserGetSigninState", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(int, __stdcall, xlive_5263, uint32_t user_index, CHAR* user_name, uint32_t user_name_length)
{
    SKIP_AND_RETURN(0);

    // called after login
    CALL_ORIGINAL(5263, "XUserGetName", user_index, user_name, user_name_length);

    println("[gfwl] user_index = {:X}", user_index);
    println("[gfwl] user_name = {}", user_name);
    println("[gfwl] user_name_length = {:X}", user_name_length);

    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(user_index), hex(user_name), hex(user_name_length));
}
DETOUR_API(int, __stdcall, xlive_5265, unsigned int a1, int a2, DWORD* a3)
{
    CALL_ORIGINAL(5265, "XUserCheckPrivilege", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_5267, uint32_t user_index, uint32_t flags, XUSER_SIGNIN_INFO* signin_info)
{
    if (signin_info) {
        // TODO: Figure out new self-made login system:
        //          - Fallback to GFWL?
        //          - Should we auto-login? Which GFWL account should it be?

        signin_info->xuid = 0xE00000FDAD8D3F73; // TODO: This should be variable
        signin_info->flags = 0;
        signin_info->user_signin_state = XUSER_SIGNIN_STATE::SignedInLocally;
        signin_info->guest_number = 0;
        signin_info->sponsor_user_index = 254;
        memcpy(signin_info->user_name, "NeKz", 5); // TODO: This should be variable
        return 0;
    }

    SKIP_AND_RETURN(0);

    // called after login
    CALL_ORIGINAL(5267, "XUserGetSigninInfo", user_index, flags, signin_info);
    
    if (signin_info) {
        println("[gfwl] signin_info->xuid = {:X}", signin_info->xuid);
        println("[gfwl] signin_info->flags = {}", signin_info->flags);
        println("[gfwl] signin_info->user_signin_state = {}", uint32_t(signin_info->user_signin_state));
        println("[gfwl] signin_info->guest_number = {}", signin_info->guest_number);
        println("[gfwl] signin_info->sponsor_user_index = {}", signin_info->sponsor_user_index);
        println("[gfwl] signin_info->user_name = {}", signin_info->user_name);
    }

    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(user_index), hex(flags), hex(signin_info));
}
DETOUR_API(int, __stdcall, xlive_5270, __int64 a1)
{
    // NOTE: The game needs a value or it will fail to initialize.
    SKIP_AND_RETURN(1);
    CALL_ORIGINAL(5270, "XNotifyCreateListener", a1);
    LOG_AND_RETURN("{:x}", a1);
}
DETOUR_API(signed int, __stdcall, xlive_5271, int a1)
{
    CALL_ORIGINAL(5271, "XShowPlayersUI", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(signed int, __stdcall, xlive_5274, int a1, int a2, int a3, int a4)
{
    CALL_ORIGINAL(5274, "XUserAwardGamerPicture", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(signed int, __stdcall, xlive_5275, int a1)
{
    CALL_ORIGINAL(5275, "XShowFriendsUI", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(signed int, __stdcall, xlive_5277, int a1, int a2, int a3)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5277, "XUserSetContext", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_5279, int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    CALL_ORIGINAL(5279, "XUserReadAchievementPicture", a1, a2, a3, a4, a5, a6, a7);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7));
}
DETOUR_API(signed int, __stdcall, xlive_5280, int a1, int a2, int a3, unsigned int a4, int a5, unsigned int a6, unsigned int a7, int a8, int a9)
{
    SKIP_AND_RETURN(0);
    // called after login
    CALL_ORIGINAL(5280, "XUserCreateAchievementEnumerator", a1, a2, a3, a4, a5, a6, a7, a8, a9);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7), hex(a8), hex(a9));
}
DETOUR_API(int, __stdcall, xlive_5281, int a1, unsigned int a2, int a3, unsigned int a4, int a5, unsigned int* a6, int a7, int a8)
{
    CALL_ORIGINAL(5281, "XUserReadStats", a1, a2, a3, a4, a5, a6, a7, a8);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7), hex(a8));
}
DETOUR_API(int, __stdcall, xlive_5284, int a1, int a2, int a3, int a4, void* Src, int a6, int a7)
{
    struct XUSER_STATS_SPEC {
	    DWORD view_id;
	    DWORD num_column_ids;
	    WORD rgw_column_ids[64];
    };

    auto a5 = (XUSER_STATS_SPEC*)Src;
    println("[gfwl] view_id = {}", a5->view_id);
    println("[gfwl] num_column_ids = {}", a5->num_column_ids);
    println("[gfwl] rgw_column_ids[0] = {}", a5->rgw_column_ids[0]);
    println("[gfwl] rgw_column_ids[1] = {}", a5->rgw_column_ids[1]);
    println("[gfwl] rgw_column_ids[2] = {}", a5->rgw_column_ids[2]);
    println("[gfwl] rgw_column_ids[3] = {}", a5->rgw_column_ids[3]);
    println("[gfwl] rgw_column_ids[4] = {}", a5->rgw_column_ids[4]);

    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5284, "XUserCreateStatsEnumeratorByRank", a1, a2, a3, a4, Src, a6, a7);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(Src), hex(a6), hex(a7));
}
DETOUR_API(int, __stdcall, xlive_5286, int a1, int a2, int a3, int a4, int a5, void* Src, int a7, int a8)
{
    CALL_ORIGINAL(5286, "XUserCreateStatsEnumeratorByXuid", a1, a2, a3, a4, a5, Src, a7, a8);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(Src), hex(a7), hex(a8));
}
DETOUR_API(int, __stdcall, xlive_5292, int a1, int a2, int a3, int a4)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5292, "XUserSetContextEx", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(int, __stdcall, xlive_5293, int a1, int a2, int a3, int a4, int a5)
{
    SKIP_AND_RETURN(0);
    // called on level load?
    CALL_ORIGINAL(5293, "XUserSetPropertyEx", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(int, __stdcall, xlive_5294, int a1, unsigned int a2, int a3, int a4)
{
    CALL_ORIGINAL(5294, "XLivePBufferGetByteArray", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(int, __stdcall, xlive_5303, int a1, const char* a2, int a3, int a4, unsigned int a5, int a6, int a7)
{
    CALL_ORIGINAL(5303, "XStringVerify", a1, a2, a3, a4, a5, a6, a7);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7));
}
DETOUR_API(int, __stdcall, xlive_5305, char a1, const unsigned __int16* a2, char a3, int a4, int a5)
{
    CALL_ORIGINAL(5305, "XStorageUploadFromMemory", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(int, __stdcall, xlive_5306, int a1, const unsigned __int16* a2, int a3, int a4, unsigned int a5, int a6, int a7)
{
    CALL_ORIGINAL(5306, "XStorageEnumerate", a1, a2, a3, a4, a5, a6, a7);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7));
}
DETOUR_API(int, __stdcall, xlive_5310)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5310, "XOnlineStartup");
    LOG_AND_RETURN("");
}
DETOUR_API(int, __stdcall, xlive_5311)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5311, "XOnlineCleanup");
    LOG_AND_RETURN("");
}
DETOUR_API(signed int, __stdcall, xlive_5312, int a1, int a2, int a3, int a4, int a5)
{
    // NOTE: The game needs a value or it will loop forever when exiting.
    SKIP_AND_RETURN(1);

    // called after login, or when loading/exiting main menu
    CALL_ORIGINAL(5312, "XFriendsCreateEnumerator", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(signed int, __stdcall, xlive_5313, int a1)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5313, "XPresenceInitialize", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(signed int, __stdcall, xlive_5314, int a1, int a2, int a3, int a4)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5314, "XUserMuteListQuery", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(signed int, __stdcall, xlive_5315, int a1, int a2)
{
    CALL_ORIGINAL(5315, "XInviteGetAcceptedInfo", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(int, __stdcall, xlive_5316, int a1, int a2, int a3, wchar_t* a4, int a5)
{
    CALL_ORIGINAL(5316, "XInviteSend", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(int, __stdcall, xlive_5320, int a1, int a2, int a3, int* a4, int a5, int a6)
{
    CALL_ORIGINAL(5320, "XSessionSearchByID", a1, a2, a3, a4, a5, a6);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6));
}
DETOUR_API(int, __stdcall, xlive_5321, int a1, int a2, int a3, unsigned __int16 a4, __int16 a5, int a6, int a7, unsigned int* a8, int a9, int a10)
{
    CALL_ORIGINAL(5321, "XSessionSearch", a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7), hex(a8), hex(a9), hex(a10));
}
DETOUR_API(signed int, __stdcall, xlive_5322, int a1, int a2, int a3, int a4, int a5)
{
    CALL_ORIGINAL(5322, "XSessionModify", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_T(signed int, xlive_5324, void* a1)
{
    SKIP_AND_RETURN(1);
    CALL_ORIGINAL(5324, "XOnlineGetNatType", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(unsigned int, __stdcall, xlive_5326, int a1, int a2, int a3, int a4, int a5)
{
    CALL_ORIGINAL(5326, "XSessionJoinRemote", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(signed int, __stdcall, xlive_5327, int a1, int a2, int* a3, int a4, int a5)
{
    CALL_ORIGINAL(5327, "XSessionJoinLocal", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(signed int, __stdcall, xlive_5328, int a1, DWORD* a2, int a3, int a4)
{
    CALL_ORIGINAL(5328, "XSessionGetDetails", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(signed int, __stdcall, xlive_5329, int a1, int a2)
{
    CALL_ORIGINAL(5329, "XSessionFlushStats", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(uintptr_t, __stdcall, xlive_5330, int a1, int a2)
{
    CALL_ORIGINAL(5330, "XSessionDelete", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(signed int, __stdcall, xlive_5331, int a1, int a2, int a3, int a4, DWORD* a5, int a6, int a7)
{
    SKIP_AND_RETURN(0);
    // called after login
    CALL_ORIGINAL(5331, "XUserReadProfileSettings", a1, a2, a3, a4, a5, a6, a7);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7));
}
DETOUR_API(signed int, __stdcall, xlive_5333, int a1, int a2, int a3, int a4, int* a5, int a6, int a7)
{
    CALL_ORIGINAL(5333, "XSessionArbitrationRegister", a1, a2, a3, a4, a5, a6, a7);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7));
}
DETOUR_API(unsigned int, __stdcall, xlive_5335, void* Src, int a2, int a3, int a4)
{
    CALL_ORIGINAL(5335, "XTitleServerCreateEnumerator", Src, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(Src), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(signed int, __stdcall, xlive_5336, int a1, int a2, int a3, int a4)
{
    CALL_ORIGINAL(5336, "XSessionLeaveRemote", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(signed int, __stdcall, xlive_5337, int a1, unsigned int a2, int a3, int a4)
{
    CALL_ORIGINAL(5337, "XUserWriteProfileSettings", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(signed int, __stdcall, xlive_5338, int a1, int a2, int a3)
{
    CALL_ORIGINAL(5338, "XPresenceSubscribe", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(signed int, __stdcall, xlive_5340, int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    CALL_ORIGINAL(5340, "XPresenceCreateEnumerator", a1, a2, a3, a4, a5, a6, a7);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7));
}
DETOUR_API(signed int, __stdcall, xlive_5342, int a1, unsigned int a2, int a3, int a4)
{
    CALL_ORIGINAL(5342, "XSessionModifySkill", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(signed int, __stdcall, xlive_5343, unsigned int a1, int a2, double* a3, double* a4, double* a5)
{
    CALL_ORIGINAL(5343, "XSessionCalculateSkill", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(signed int, __stdcall, xlive_5344, int a1, int a2, int a3, unsigned int a4, int a5, int a6, int a7)
{
    CALL_ORIGINAL(5344, "XStorageBuildServerPath", a1, a2, a3, a4, a5, a6, a7);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7));
}
DETOUR_API(int, __stdcall, xlive_5345, int a1, const unsigned __int16* a2, int a3, int a4, unsigned int a5, int a6, int a7)
{
    CALL_ORIGINAL(5345, "XStorageDownloadToMemory", a1, a2, a3, a4, a5, a6, a7);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7));
}
DETOUR_API(int, __stdcall, xlive_5350, int a1, void* a2, int a3, int a4, int a5, int a6, int a7)
{
    CALL_ORIGINAL(5350, "XLiveContentCreateAccessHandle", a1, a2, a3, a4, a5, a6, a7);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7));
}
DETOUR_API(int, __stdcall, xlive_5355, int a1, int a2, char* a3, unsigned int* a4)
{
    CALL_ORIGINAL(5355, "XLiveContentGetPath", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(int, __stdcall, xlive_5356, int a1, int a2, char* a3, unsigned int* a4)
{
    CALL_ORIGINAL(5356, "XLiveContentGetDisplayName", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(signed int, __stdcall, xlive_5360, unsigned int a1, int a2, DWORD* a3, int* a4)
{
    // called after login
    CALL_ORIGINAL(5360, "XLiveContentCreateEnumerator", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(signed int, __stdcall, xlive_5365, int a1, int a2, int a3, int a4, int a5)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5365, "XShowMarketplaceUI", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(int, __stdcall, xlive_5367, int a1, int a2, unsigned int a3, int a4, int a5)
{
    CALL_ORIGINAL(5367, "XContentGetMarketplaceCounts", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(int, __stdcall, xlive_5372, HANDLE hObject, int a2, int a3, int a4, int a5, int a6)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5372, "XMarketplaceCreateOfferEnumerator", hObject, a2, a3, a4, a5, a6);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(hObject), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6));
}
DETOUR_API(__int16, __stdcall, xlive_38, __int16 a1)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(38, "XSocketNTOHS", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(int, __stdcall, xlive_5208, int a1, int a2, int a3, int a4)
{
    CALL_ORIGINAL(5208, "XShowGameInviteUI", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(int, __stdcall, xlive_5209, int a1, int a2, int a3, int a4)
{
    CALL_ORIGINAL(5209, "XShowMessageComposeUI", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(int, __stdcall, xlive_5210, int a1, int a2)
{
    CALL_ORIGINAL(5210, "XShowFriendRequestUI", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(int, __stdcall, xlive_5214, int a1, int a2)
{
    CALL_ORIGINAL(5214, "XShowPlayerReviewUI", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(int, __stdcall, xlive_5250, int a1)
{
    CALL_ORIGINAL(5250, "XShowAchievementsUI", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(int, __stdcall, xlive_5259, int a1, int a2, int a3, int a4)
{
    CALL_ORIGINAL(5259, "XLiveSignin", a1, a2, a3, a4);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4));
}
DETOUR_API(int, __stdcall, xlive_5260, int a1, int a2)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5260, "XShowSigninUI", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(int, __stdcall, xlive_5300, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
    CALL_ORIGINAL(5300, "XSessionCreate", a1, a2, a3, a4, a5, a6, a7, a8);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5), hex(a6), hex(a7), hex(a8));
}
DETOUR_API(int, __stdcall, xlive_5318, int a1, int a2, int a3)
{
    CALL_ORIGINAL(5318, "XSessionStart", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(int, __stdcall, xlive_5034, int a1, int a2, int a3, int a4, int a5)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5034, "XLiveProtectData", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x} -> {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), dptr(a4), hex(a5));
}
DETOUR_API(int, __stdcall, xlive_5035, BYTE* protected_data, DWORD size_of_protected_data, BYTE* unprotected_data, DWORD* size_of_data, HANDLE* protected_data_handle)
{
    // XLive decrypts the protected data buffer, which checks against your logged in GFWL account. We simply do not call
    // the original function and instead write the same CRC values from the save file back into the buffer, making the
    // save manager succeed verifying it. This means that we can now load any save file :^)

    //xlive_debug_break(false);

    struct unprotected_buffer_t {
        uint32_t header_crc;
        uint32_t body_crc;
    };
    static_assert(sizeof(unprotected_buffer_t) == 8);

    if (!unprotected_data) {
        if (size_of_data) {
            *size_of_data = sizeof(unprotected_buffer_t);
            return 0;
        }
    } else {
        auto pg_save_load = *reinterpret_cast<PgSaveLoad**>(Offsets::pg_save_load);
        if (pg_save_load
            && pg_save_load->file_manager
            && pg_save_load->file_manager->save_file
            && pg_save_load->file_manager->save_file->buffer) {
            auto unprotected_buffer = reinterpret_cast<unprotected_buffer_t*>(unprotected_data);
            auto save_buffer = pg_save_load->file_manager->save_file->buffer;

            unprotected_buffer->header_crc = save_buffer->header.header_crc;
            unprotected_buffer->body_crc = save_buffer->header.body_crc;
            return 0;
        }
    }

    //CALL_ORIGINAL(5035, "XLiveUnprotectData", protected_data, size_of_protected_data, unprotected_data, size_of_data, protected_data_handle);
    //LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x} -> {:x}, {:x} -> {:x}", hex(protected_data), hex(size_of_protected_data), hex(unprotected_data), hex(size_of_data), dptr(size_of_data), hex(protected_data_handle), dptr(protected_data_handle));

    return -1;
}
DETOUR_API(int, __stdcall, xlive_5036, int a1, int a2)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5036, "XLiveCreateProtectedDataContext", a1, a2);
    LOG_AND_RETURN("{:x}, {:x} -> {:x}", hex(a1), hex(a2), dptr(a2));
}
//DETOUR_MID(xlive_5034)
//{
//    SAVE_REGISTERS();
//
//    print_stack(5);
//    log_xlive_call(5034);
//
//    LOAD_REGISTERS();
//
//    JUMP_TO(xlive_5034);
//}
//DETOUR_MID(xlive_5035)
//{
//    SAVE_REGISTERS();
//
//    print_stack(5);
//    log_xlive_call(5035);
//
//    LOAD_REGISTERS();
//
//    JUMP_TO(xlive_5035);
//}
//DETOUR_MID(xlive_5036)
//{
//    SAVE_REGISTERS();
//
//    print_stack(2);
//    log_xlive_call(5036);
//
//    LOAD_REGISTERS();
//
//    JUMP_TO(xlive_5036);
//}
DETOUR_API(int, __stdcall, xlive_5038, int a1)
{
    // Isn't it interesting how this function crashes if we do not hook it?
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5038, "XLiveCloseProtectedDataContext", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(int, __stdcall, xlive_5206, int a1)
{
    CALL_ORIGINAL(5206, "XShowMessagesUI", a1);
    LOG_AND_RETURN("{:x}", hex(a1));
}
DETOUR_API(int, __stdcall, xlive_5264, int a1, int a2, int a3, int a4, int a5)
{
    CALL_ORIGINAL(5264, "XUserAreUsersFriends", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(int, __stdcall, xlive_5278, int a1, int a2, int a3)
{
    CALL_ORIGINAL(5278, "XUserWriteAchievements", a1, a2, a3);
    LOG_AND_RETURN("{:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3));
}
DETOUR_API(int, __stdcall, xlive_5297, int a1, int a2)
{
    SKIP_AND_RETURN(0);
    CALL_ORIGINAL(5297, "XLiveInitializeEx", a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
DETOUR_API(int, __stdcall, xlive_5317, int a1, int a2, int a3, int a4, int a5)
{
    CALL_ORIGINAL(5317, "XSessionWriteStats", a1, a2, a3, a4, a5);
    LOG_AND_RETURN("{:x}, {:x}, {:x}, {:x}, {:x}", hex(a1), hex(a2), hex(a3), hex(a4), hex(a5));
}
DETOUR_API(int, __stdcall, xlive_5332, int a1, int a2)
{
    CALL_ORIGINAL(5332, "XSessionEnd",  a1, a2);
    LOG_AND_RETURN("{:x}, {:x}", hex(a1), hex(a2));
}
// clang-format on
