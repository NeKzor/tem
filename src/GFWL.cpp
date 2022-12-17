/*
 * Copyright (c) 2022 NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#include "GFWL.hpp"
#include "Console.hpp"
#include "Memory.hpp"
#include "Offsets.hpp"
#include "Platform.hpp"
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
DECL_DETOUR_API(void*, __stdcall, xlive_14, void* a1, int a2, int a3);
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
DECL_DETOUR_API(void, __stdcall, xlive_5003);
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
DECL_DETOUR_API(signed int, __stdcall, xlive_5262, int a1);
DECL_DETOUR_API(int, __stdcall, xlive_5263, int a1, CHAR* a2, int a3);
DECL_DETOUR_API(int, __stdcall, xlive_5265, unsigned int a1, int a2, DWORD* a3);
DECL_DETOUR_API(signed int, __stdcall, xlive_5267, int a1, int a2, int a3);
DECL_DETOUR_API(int, __stdcall, xlive_5270, int a1, int a2);
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
DECL_DETOUR_API(void*, __stdcall, xlive_5330, int a1, int a2);
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

//DECL_DETOUR_API(int, __stdcall, xlive_5034, int a1, int a2, int a3, int a4, int a5);
//DECL_DETOUR_API(int, __stdcall, xlive_5035, int a1, int a2, int a3, int a4, int a5);
//DECL_DETOUR_API(int, __stdcall, xlive_5036, int a1, int a2);
//DECL_DETOUR_API(int, __stdcall, xlive_5038, int a1);
DECL_DETOUR_MID(xlive_5034);
DECL_DETOUR_MID(xlive_5035);
DECL_DETOUR_MID(xlive_5036);
DECL_DETOUR_MID(xlive_5038);

//DECL_DETOUR_API(int, __stdcall, xlive_5206, int a1);
//DECL_DETOUR_API(int, __stdcall, xlive_5264, int a1, int a2, int a3, int a4, int a5);
//DECL_DETOUR_API(int, __stdcall, xlive_5278, int a1, int a2, int a3);
//DECL_DETOUR_API(int, __stdcall, xlive_5297, int a1, int a2);
//DECL_DETOUR_API(int, __stdcall, xlive_5317, int a1, int a2, int a3, int a4, int a5);
//DECL_DETOUR_API(int, __stdcall, xlive_5332, int a1, int a2);
DECL_DETOUR_MID(xlive_5206);
DECL_DETOUR_MID(xlive_5264);
DECL_DETOUR_MID(xlive_5278);
DECL_DETOUR_MID(xlive_5297);
DECL_DETOUR_MID(xlive_5317);
DECL_DETOUR_MID(xlive_5332);

auto patch_gfwl() -> void
{
    auto xlive = Memory::ModuleInfo();

    if (!Memory::TryGetModule("xlive.dll", &xlive)) {
        return console->Println("[gfwl] Unable to find GFWL module :(");
    }

    // Modify memory modification check
    //      Original: 0F 84 (jz)
    //      Patch:    90 E9 (nop jmp)

    unsigned char nop_jmp[2] = { 0x90, 0xE9 };
    if (gfwlMemCheckPatch.Execute(xlive.base + Offsets::xlive_memory_check, nop_jmp, sizeof(nop_jmp))) {
        console->Println("[gfwl] Patched xlive.dll memory check at 0x{:04x}", gfwlMemCheckPatch.GetLocation());
    } else {
        return console->Println("[gfwl] Unable to patch memory check :(");
    }

    //hook_gfwl(xlive);
}

auto hook_gfwl(Memory::ModuleInfo& xlive) -> void
{
    auto handle = GetModuleHandleA(xlive.path);
    console->Println("[gfwl] xlive.dll handle 0x{:04x}", uintptr_t(handle));

#define HOOK_ORDINAL(ordinal, name)                                                                                    \
    auto ordinal_##ordinal##_address = uintptr_t(GetProcAddress(handle, LPCSTR(ordinal)));                             \
    if (ordinal_##ordinal##_address) {                                                                                 \
        addressesToUnhook.insert_or_assign(ordinal, std::make_tuple(ordinal_##ordinal##_address, name, 0, 0));         \
        MH_HOOK(xlive_##ordinal, ordinal_##ordinal##_address);                                                         \
        console->Println("[gfwl] Hooked xlive_{} at 0x{:04x}", ordinal, ordinal_##ordinal##_address);                  \
    } else {                                                                                                           \
        console->Println("[gfwl] Unable to hook xlive_" #ordinal);                                                     \
    }

#define HOOK_IAT_WRAPPER(wrapper_address, ordinal, name)                                                               \
    addressesToUnhook.insert_or_assign(ordinal, std::make_tuple(wrapper_address, name, 0, 0));                         \
    auto xlive_##ordinal##_original = **(uintptr_t**)(wrapper_address + 2);                                            \
    MH_HOOK_MID(xlive_##ordinal, wrapper_address);                                                                     \
    xlive_##ordinal = xlive_##ordinal##_original;                                                                      \
    console->Println("[gfwl] Hooked {} 0x{:04x} at 0x{:04x}", name, xlive_##ordinal, wrapper_address);

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
        console->Println("[gfwl] Unable to find GFWL module :(");
        return false;
    }

    auto ntdll = Memory::ModuleInfo();
    if (!Memory::TryGetModule("ntdll.dll", &ntdll)) {
        console->Println("[gfwl] Unable to find ntdll.dll module :(");
        return false;
    }

    auto ntdll_NtQueryInformationThread
        = Memory::GetSymbolAddress<decltype(NtQueryInformationThread)*>(ntdll.base, "NtQueryInformationThread");

    if (!ntdll_NtQueryInformationThread) {
        console->Println("[gfwl] Unable to find NtQueryInformationThread :(");
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
                                console->Println("[gfwl] {} main thread 0x{:04x} of start address 0x{:04x}",
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

    for (auto& address : addressesToUnhook) {
        auto function = std::get<0>(address.second);
        auto name = std::get<1>(address.second);

        MH_UNHOOK_TARGET(function);
        console->Println("[gfwl] Unhooked {} at 0x{:04x}", name, function);
    }

    addressesToUnhook.clear();

    if (gfwlMemCheckPatch.Restore()) {
        console->Println("[gfwl] Restored mem check patch");
    }
}

auto __forceinline print_stack(int parameters) -> void
{
    //auto retAddress = uintptr_t(_AddressOfReturnAddress());
    //for (int i = 12; i <= 12 + parameters; ++i) {
    //    auto stack = retAddress + (i << 2);
    //    console->Println("[gfwl] stack(0x{:04x}) -> 0x{:08x} | {}", stack, *(uintptr_t*)stack, *(uintptr_t*)stack);
    //}
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
    console->Println("[gfwl] {} (xlive_{} at 0x{:04x})", name, ordinal, function);
}

#define SAVE_REGISTERS() __asm {\
   __asm pushad \
   __asm pushfd }
#define LOAD_REGISTERS() __asm {\
   __asm popfd \
   __asm popad }
#define JUMP_TO(address) __asm jmp address

DETOUR_API(signed int, __stdcall, xlive_3, int a1, signed int a2, signed int a3)
{
    log_xlive_call(3);
    return xlive_3(a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_4, int a1)
{
    log_xlive_call(4);
    return xlive_4(a1);
}
DETOUR_API(signed int, __stdcall, xlive_6, int a1, int a2, signed int* a3)
{
    log_xlive_call(6);
    return xlive_6(a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_7, int a1, int a2, signed int a3, int* a4, signed int a5)
{
    log_xlive_call(7);
    return xlive_7(a1, a2, a3, a4, a5);
}
DETOUR_API(int, __stdcall, xlive_8, int a1, int a2, int a3, void* a4, int a5)
{
    log_xlive_call(8);
    return xlive_8(a1, a2, a3, a4, a5);
}
DETOUR_API(int, __stdcall, xlive_9, int a1, int a2, int a3)
{
    log_xlive_call(9);
    return xlive_9(a1, a2, a3);
}
DETOUR_API(int, __stdcall, xlive_11, int a1, int a2, int a3)
{
    log_xlive_call(11);
    return xlive_11(a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_12, int a1, int a2, int a3)
{
    log_xlive_call(12);
    return xlive_12(a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_13, int a1, signed int a2)
{
    log_xlive_call(13);
    return xlive_13(a1, a2);
}
DETOUR_API(void*, __stdcall, xlive_14, void* a1, int a2, int a3)
{
    log_xlive_call(14);
    return xlive_14(a1, a2, a3);
}
DETOUR_API(int, __stdcall, xlive_15, int a1, int a2, int a3, int a4, DWORD a5)
{
    log_xlive_call(15);
    return xlive_15(a1, a2, a3, a4, a5);
}
DETOUR_API(int, __stdcall, xlive_18, int a1, int a2, int a3, int a4)
{
    // called frequently
    //log_xlive_call(18);
    return xlive_18(a1, a2, a3, a4);
}
DETOUR_API(signed int, __stdcall, xlive_20, int a1, int a2, int a3, char a4, int a5, DWORD* a6)
{
    // called frequently when in multiplayer lobby
    //log_xlive_call(20);
    return xlive_20(a1, a2, a3, a4, a5, a6);
}
DETOUR_API(int, __stdcall, xlive_22, int a1, int a2, int a3, int a4)
{
    //log_xlive_call(22);
    return xlive_22(a1, a2, a3, a4);
}
DETOUR_API(signed int, __stdcall, xlive_24, int a1, int a2, int a3, int a4, int a5, int a6)
{
    log_xlive_call(24);
    return xlive_24(a1, a2, a3, a4, a5, a6);
}
DETOUR_API(DWORD, __stdcall, xlive_27)
{
    // called frequently when online
    // log_xlive_call(27);
    return xlive_27();
}
DETOUR_API(int, __stdcall, xlive_51, int a1)
{
    log_xlive_call(51);
    return xlive_51(a1);
}
DETOUR_API(int, __stdcall, xlive_52)
{
    log_xlive_call(52);
    return xlive_52();
}
DETOUR_API(int, __stdcall, xlive_53, BYTE* a1, HCRYPTPROV a2)
{
    log_xlive_call(53);
    return xlive_53(a1, a2);
}
DETOUR_API(signed int, __stdcall, xlive_55, int a1, void* a2)
{
    log_xlive_call(55);
    return xlive_55(a1, a2);
}
DETOUR_API(signed int, __stdcall, xlive_56, const void* a1)
{
    log_xlive_call(56);
    return xlive_56(a1);
}
DETOUR_API(signed int, __stdcall, xlive_57, int a1, const void* a2, DWORD* a3)
{
    log_xlive_call(57);
    return xlive_57(a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_58, int a1, int a2, DWORD* a3)
{
    log_xlive_call(58);
    return xlive_58(a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_63, unsigned int a1)
{
    log_xlive_call(63);
    return xlive_63(a1);
}
DETOUR_API(signed int, __stdcall, xlive_67, int a1, int a2, DWORD* a3)
{
    log_xlive_call(67);
    return xlive_67(a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_68, int a1)
{
    log_xlive_call(68);
    return xlive_68(a1);
}
DETOUR_API(int, __stdcall, xlive_69, int a1, void* a2, int a3, int a4, int a5)
{
    log_xlive_call(69);
    return xlive_69(a1, a2, a3, a4, a5);
}
DETOUR_API(int, __fastcall, xlive_70, int ecx0, int edx0, int a1, int a2, int a3, int a4, int a5, int a6, int a7,
    int a8, int a9, int a10, int a11, int a12)
{
    log_xlive_call(70);
    return xlive_70(ecx0, edx0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
}
DETOUR_API(int, __fastcall, xlive_71, int ecx0, int edx0, int a1, int a2, int a3)
{
    log_xlive_call(71);
    return xlive_71(ecx0, edx0, a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_72, int a1)
{
    log_xlive_call(72);
    return xlive_72(a1);
}
DETOUR_API(int, __stdcall, xlive_73, int a1)
{
    log_xlive_call(73);
    return xlive_73(a1);
}
DETOUR_API(unsigned int, __stdcall, xlive_75)
{
    log_xlive_call(75);
    return xlive_75();
}
DETOUR_API(signed int, __stdcall, xlive_77, const void* a1, int a2)
{
    log_xlive_call(77);
    return xlive_77(a1, a2);
}
DETOUR_API(int, __stdcall, xlive_84, int a1)
{
    log_xlive_call(84);
    return xlive_84(a1);
}
DETOUR_API(int, __stdcall, xlive_651, int a1, int a2, DWORD* a3, DWORD* a4)
{
    // Called frequently
    //log_xlive_call(651);
    return xlive_651(a1, a2, a3, a4);
}
DETOUR_API(int, __stdcall, xlive_652, LONG Value)
{
    log_xlive_call(652);
    return xlive_652(Value);
}
DETOUR_API(DWORD, __stdcall, xlive_1082, int a1)
{
    // called on login, frequently when online
    //log_xlive_call(1082);
    return xlive_1082(a1);
}
DETOUR_API(DWORD, __stdcall, xlive_1083, int a1, DWORD* a2, int a3)
{
    // called on login or when gfwl ui opens, frequently when online
    //log_xlive_call(1083);
    return xlive_1083(a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_5001, int a1)
{
    // frequent call
    //log_xlive_call(5001);
    return xlive_5001(a1);
}
DETOUR_API(unsigned int, __stdcall, xlive_5002)
{
    // frequent call
    //log_xlive_call(5002);
    return xlive_5002();
}
DETOUR_API(void, __stdcall, xlive_5003)
{
    log_xlive_call(5003);
    return xlive_5003();
}
DETOUR_API(signed int, __stdcall, xlive_5006)
{
    log_xlive_call(5006);
    return xlive_5006();
}
DETOUR_API(signed int, __stdcall, xlive_5007, int a1)
{
    log_xlive_call(5007);
    return xlive_5007(a1);
}
DETOUR_API(signed int, __stdcall, xlive_5008, int a1, int a2, DWORD* a3)
{
    log_xlive_call(5008);
    return xlive_5008(a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_5016, int a1, unsigned int a2)
{
    log_xlive_call(5016);
    return xlive_5016(a1, a2);
}
DETOUR_API(signed int, __stdcall, xlive_5017, int a1)
{
    log_xlive_call(5017);
    return xlive_5017(a1);
}
DETOUR_API(int, __stdcall, xlive_5030, HIMC a1)
{
    // frequent called when window focused
    //log_xlive_call(5030);
    return xlive_5030(a1);
}
DETOUR_API(int, __stdcall, xlive_5031, unsigned int a1, DWORD* a2)
{
    log_xlive_call(5031);
    return xlive_5031(a1, a2);
}
DETOUR_API(int, __stdcall, xlive_5212, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, void* Src,
    void* a10, int a11, int a12)
{
    // called frequently on login
    //log_xlive_call(5212);
    return xlive_5212(a1, a2, a3, a4, a5, a6, a7, a8, Src, a10, a11, a12);
}
DETOUR_API(signed int, __stdcall, xlive_5215, int a1)
{
    log_xlive_call(5215);
    return xlive_5215(a1);
}
DETOUR_API(signed int, __stdcall, xlive_5216, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
    log_xlive_call(5216);
    return xlive_5216(a1, a2, a3, a4, a5, a6, a7, a8);
}
DETOUR_API(int, __stdcall, xlive_5251, HANDLE hObject)
{
    // called frequently acquiring a handle
    //log_xlive_call(5251);
    return xlive_5251(hObject);
}
DETOUR_API(signed int, __stdcall, xlive_5252, int a1, __int64 a2)
{
    log_xlive_call(5252);
    return xlive_5252(a1, a2);
}
DETOUR_API(int, __stdcall, xlive_5256, int a1, int a2, int a3, DWORD* a4, int a5)
{
    // called after login
    log_xlive_call(5256);
    return xlive_5256(a1, a2, a3, a4, a5);
}
DETOUR_API(signed int, __stdcall, xlive_5258, int a1)
{
    log_xlive_call(5258);
    return xlive_5258(a1);
}
DETOUR_API(signed int, __stdcall, xlive_5262, int a1)
{
    // called on login and then frequently after
    //log_xlive_call(5262);
    return xlive_5262(a1);
}
DETOUR_API(int, __stdcall, xlive_5263, int a1, CHAR* a2, int a3)
{
    // called after login
    log_xlive_call(5263);
    return xlive_5263(a1, a2, a3);
}
DETOUR_API(int, __stdcall, xlive_5265, unsigned int a1, int a2, DWORD* a3)
{
    log_xlive_call(5265);
    return xlive_5265(a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_5267, int a1, int a2, int a3)
{
    // called after login
    log_xlive_call(5267);
    return xlive_5267(a1, a2, a3);
}
DETOUR_API(int, __stdcall, xlive_5270, int a1, int a2)
{
    log_xlive_call(5270);
    return xlive_5270(a1, a2);
}
DETOUR_API(signed int, __stdcall, xlive_5271, int a1)
{
    log_xlive_call(5271);
    return xlive_5271(a1);
}
DETOUR_API(signed int, __stdcall, xlive_5274, int a1, int a2, int a3, int a4)
{
    log_xlive_call(5274);
    return xlive_5274(a1, a2, a3, a4);
}
DETOUR_API(signed int, __stdcall, xlive_5275, int a1)
{
    log_xlive_call(5275);
    return xlive_5275(a1);
}
DETOUR_API(signed int, __stdcall, xlive_5277, int a1, int a2, int a3)
{
    log_xlive_call(5277);
    return xlive_5277(a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_5279, int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    log_xlive_call(5279);
    return xlive_5279(a1, a2, a3, a4, a5, a6, a7);
}
DETOUR_API(signed int, __stdcall, xlive_5280, int a1, int a2, int a3, unsigned int a4, int a5, unsigned int a6,
    unsigned int a7, int a8, int a9)
{
    // called after login
    log_xlive_call(5280);
    return xlive_5280(a1, a2, a3, a4, a5, a6, a7, a8, a9);
}
DETOUR_API(int, __stdcall, xlive_5281, int a1, unsigned int a2, int a3, unsigned int a4, int a5, unsigned int* a6,
    int a7, int a8)
{
    log_xlive_call(5281);
    return xlive_5281(a1, a2, a3, a4, a5, a6, a7, a8);
}
DETOUR_API(int, __stdcall, xlive_5284, int a1, int a2, int a3, int a4, void* Src, int a6, int a7)
{
    log_xlive_call(5284);
    return xlive_5284(a1, a2, a3, a4, Src, a6, a7);
}
DETOUR_API(int, __stdcall, xlive_5286, int a1, int a2, int a3, int a4, int a5, void* Src, int a7, int a8)
{
    log_xlive_call(5286);
    return xlive_5286(a1, a2, a3, a4, a5, Src, a7, a8);
}
DETOUR_API(int, __stdcall, xlive_5292, int a1, int a2, int a3, int a4)
{
    log_xlive_call(5292);
    return xlive_5292(a1, a2, a3, a4);
}
DETOUR_API(int, __stdcall, xlive_5293, int a1, int a2, int a3, int a4, int a5)
{
    // called on level load?
    log_xlive_call(5293);
    return xlive_5293(a1, a2, a3, a4, a5);
}
DETOUR_API(int, __stdcall, xlive_5294, int a1, unsigned int a2, int a3, int a4)
{
    log_xlive_call(5294);
    return xlive_5294(a1, a2, a3, a4);
}
DETOUR_API(int, __stdcall, xlive_5303, int a1, const char* a2, int a3, int a4, unsigned int a5, int a6, int a7)
{
    log_xlive_call(5303);
    return xlive_5303(a1, a2, a3, a4, a5, a6, a7);
}
DETOUR_API(int, __stdcall, xlive_5305, char a1, const unsigned __int16* a2, char a3, int a4, int a5)
{
    log_xlive_call(5305);
    return xlive_5305(a1, a2, a3, a4, a5);
}
DETOUR_API(
    int, __stdcall, xlive_5306, int a1, const unsigned __int16* a2, int a3, int a4, unsigned int a5, int a6, int a7)
{
    log_xlive_call(5306);
    return xlive_5306(a1, a2, a3, a4, a5, a6, a7);
}
DETOUR_API(int, __stdcall, xlive_5310)
{
    log_xlive_call(5310);
    return xlive_5310();
}
DETOUR_API(int, __stdcall, xlive_5311)
{
    log_xlive_call(5311);
    return xlive_5311();
}
DETOUR_API(signed int, __stdcall, xlive_5312, int a1, int a2, int a3, int a4, int a5)
{
    // called after login, or when loading/exiting main menu
    log_xlive_call(5312);
    return xlive_5312(a1, a2, a3, a4, a5);
}
DETOUR_API(signed int, __stdcall, xlive_5313, int a1)
{
    log_xlive_call(5313);
    return xlive_5313(a1);
}
DETOUR_API(signed int, __stdcall, xlive_5314, int a1, int a2, int a3, int a4)
{
    log_xlive_call(5314);
    return xlive_5314(a1, a2, a3, a4);
}
DETOUR_API(signed int, __stdcall, xlive_5315, int a1, int a2)
{
    log_xlive_call(5315);
    return xlive_5315(a1, a2);
}
DETOUR_API(int, __stdcall, xlive_5316, int a1, int a2, int a3, wchar_t* a4, int a5)
{
    log_xlive_call(5316);
    return xlive_5316(a1, a2, a3, a4, a5);
}
DETOUR_API(int, __stdcall, xlive_5320, int a1, int a2, int a3, int* a4, int a5, int a6)
{
    log_xlive_call(5320);
    return xlive_5320(a1, a2, a3, a4, a5, a6);
}
DETOUR_API(int, __stdcall, xlive_5321, int a1, int a2, int a3, unsigned __int16 a4, __int16 a5, int a6, int a7,
    unsigned int* a8, int a9, int a10)
{
    log_xlive_call(5321);
    return xlive_5321(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
}
DETOUR_API(signed int, __stdcall, xlive_5322, int a1, int a2, int a3, int a4, int a5)
{
    log_xlive_call(5322);
    return xlive_5322(a1, a2, a3, a4, a5);
}
DETOUR_T(signed int, xlive_5324, void* a1)
{
    log_xlive_call(5324);
    return xlive_5324(a1);
}
DETOUR_API(unsigned int, __stdcall, xlive_5326, int a1, int a2, int a3, int a4, int a5)
{
    log_xlive_call(5326);
    return xlive_5326(a1, a2, a3, a4, a5);
}
DETOUR_API(signed int, __stdcall, xlive_5327, int a1, int a2, int* a3, int a4, int a5)
{
    log_xlive_call(5327);
    return xlive_5327(a1, a2, a3, a4, a5);
}
DETOUR_API(signed int, __stdcall, xlive_5328, int a1, DWORD* a2, int a3, int a4)
{
    log_xlive_call(5328);
    return xlive_5328(a1, a2, a3, a4);
}
DETOUR_API(signed int, __stdcall, xlive_5329, int a1, int a2)
{
    log_xlive_call(5329);
    return xlive_5329(a1, a2);
}
DETOUR_API(void*, __stdcall, xlive_5330, int a1, int a2)
{
    log_xlive_call(5330);
    return xlive_5330(a1, a2);
}
DETOUR_API(signed int, __stdcall, xlive_5331, int a1, int a2, int a3, int a4, DWORD* a5, int a6, int a7)
{
    // called after login
    log_xlive_call(5331);
    return xlive_5331(a1, a2, a3, a4, a5, a6, a7);
}
DETOUR_API(signed int, __stdcall, xlive_5333, int a1, int a2, int a3, int a4, int* a5, int a6, int a7)
{
    log_xlive_call(5333);
    return xlive_5333(a1, a2, a3, a4, a5, a6, a7);
}
DETOUR_API(unsigned int, __stdcall, xlive_5335, void* Src, int a2, int a3, int a4)
{
    log_xlive_call(5335);
    return xlive_5335(Src, a2, a3, a4);
}
DETOUR_API(signed int, __stdcall, xlive_5336, int a1, int a2, int a3, int a4)
{
    log_xlive_call(5336);
    return xlive_5336(a1, a2, a3, a4);
}
DETOUR_API(signed int, __stdcall, xlive_5337, int a1, unsigned int a2, int a3, int a4)
{
    log_xlive_call(5337);
    return xlive_5337(a1, a2, a3, a4);
}
DETOUR_API(signed int, __stdcall, xlive_5338, int a1, int a2, int a3)
{
    log_xlive_call(5338);
    return xlive_5338(a1, a2, a3);
}
DETOUR_API(signed int, __stdcall, xlive_5340, int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    log_xlive_call(5340);
    return xlive_5340(a1, a2, a3, a4, a5, a6, a7);
}
DETOUR_API(signed int, __stdcall, xlive_5342, int a1, unsigned int a2, int a3, int a4)
{
    log_xlive_call(5342);
    return xlive_5342(a1, a2, a3, a4);
}
DETOUR_API(signed int, __stdcall, xlive_5343, unsigned int a1, int a2, double* a3, double* a4, double* a5)
{
    log_xlive_call(5343);
    return xlive_5343(a1, a2, a3, a4, a5);
}
DETOUR_API(signed int, __stdcall, xlive_5344, int a1, int a2, int a3, unsigned int a4, int a5, int a6, int a7)
{
    log_xlive_call(5344);
    return xlive_5344(a1, a2, a3, a4, a5, a6, a7);
}
DETOUR_API(
    int, __stdcall, xlive_5345, int a1, const unsigned __int16* a2, int a3, int a4, unsigned int a5, int a6, int a7)
{
    log_xlive_call(5345);
    return xlive_5345(a1, a2, a3, a4, a5, a6, a7);
}
DETOUR_API(int, __stdcall, xlive_5350, int a1, void* a2, int a3, int a4, int a5, int a6, int a7)
{
    log_xlive_call(5350);
    return xlive_5350(a1, a2, a3, a4, a5, a6, a7);
}
DETOUR_API(int, __stdcall, xlive_5355, int a1, int a2, char* a3, unsigned int* a4)
{
    log_xlive_call(5355);
    return xlive_5355(a1, a2, a3, a4);
}
DETOUR_API(int, __stdcall, xlive_5356, int a1, int a2, char* a3, unsigned int* a4)
{
    log_xlive_call(5356);
    return xlive_5356(a1, a2, a3, a4);
}
DETOUR_API(signed int, __stdcall, xlive_5360, unsigned int a1, int a2, DWORD* a3, int* a4)
{
    // called after login
    log_xlive_call(5360);
    return xlive_5360(a1, a2, a3, a4);
}
DETOUR_API(signed int, __stdcall, xlive_5365, int a1, int a2, int a3, int a4, int a5)
{
    log_xlive_call(5365);
    return xlive_5365(a1, a2, a3, a4, a5);
}
DETOUR_API(int, __stdcall, xlive_5367, int a1, int a2, unsigned int a3, int a4, int a5)
{
    log_xlive_call(5367);
    return xlive_5367(a1, a2, a3, a4, a5);
}
DETOUR_API(int, __stdcall, xlive_5372, HANDLE hObject, int a2, int a3, int a4, int a5, int a6)
{
    log_xlive_call(5372);
    return xlive_5372(hObject, a2, a3, a4, a5, a6);
}

DETOUR_API(__int16, __stdcall, xlive_38, __int16 a1)
{
    log_xlive_call(38);
    return xlive_38(a1);
}
DETOUR_MID(xlive_5034)
{
    SAVE_REGISTERS();

    print_stack(5);
    log_xlive_call(5034);

    LOAD_REGISTERS();

    JUMP_TO(xlive_5034);
}
DETOUR_MID(xlive_5035)
{
    SAVE_REGISTERS();

    print_stack(5);
    log_xlive_call(5035);

    LOAD_REGISTERS();

    JUMP_TO(xlive_5035);
}
DETOUR_MID(xlive_5036)
{
    SAVE_REGISTERS();

    print_stack(2);
    log_xlive_call(5036);

    LOAD_REGISTERS();

    JUMP_TO(xlive_5036);
}
DETOUR_MID(xlive_5038)
{
    SAVE_REGISTERS();

    print_stack(1);
    log_xlive_call(5038);

    LOAD_REGISTERS();

    JUMP_TO(xlive_5038);
}
DETOUR_MID(xlive_5206)
{
    SAVE_REGISTERS();

    print_stack(1);
    log_xlive_call(5206);

    LOAD_REGISTERS();

    JUMP_TO(xlive_5206);
}
DETOUR_API(int, __stdcall, xlive_5208, int a1, int a2, int a3, int a4)
{
    log_xlive_call(5208);
    return xlive_5208(a1, a2, a3, a4);
}
DETOUR_API(int, __stdcall, xlive_5209, int a1, int a2, int a3, int a4)
{
    log_xlive_call(5209);
    return xlive_5209(a1, a2, a3, a4);
}
DETOUR_API(int, __stdcall, xlive_5210, int a1, int a2)
{
    log_xlive_call(5210);
    return xlive_5210(a1, a2);
}
DETOUR_API(int, __stdcall, xlive_5214, int a1, int a2)
{
    log_xlive_call(5214);
    return xlive_5214(a1, a2);
}
DETOUR_API(int, __stdcall, xlive_5250, int a1)
{
    log_xlive_call(5250);
    return xlive_5250(a1);
}
DETOUR_API(int, __stdcall, xlive_5259, int a1, int a2, int a3, int a4)
{
    log_xlive_call(5259);
    return xlive_5259(a1, a2, a3, a4);
}
DETOUR_API(int, __stdcall, xlive_5260, int a1, int a2)
{
    log_xlive_call(5260);
    return xlive_5260(a1, a2);
}
DETOUR_MID(xlive_5264)
{
    SAVE_REGISTERS();

    print_stack(5);
    log_xlive_call(5264);

    LOAD_REGISTERS();

    JUMP_TO(xlive_5264);
}
DETOUR_MID(xlive_5278)
{
    SAVE_REGISTERS();

    print_stack(3);
    log_xlive_call(5278);

    LOAD_REGISTERS();

    JUMP_TO(xlive_5278);
}
DETOUR_MID(xlive_5297)
{
    SAVE_REGISTERS();

    print_stack(2);
    log_xlive_call(5297);

    LOAD_REGISTERS();

    JUMP_TO(xlive_5297);
}

DETOUR_API(int, __stdcall, xlive_5300, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
    log_xlive_call(5300);
    return xlive_5300(a1, a2, a3, a4, a5, a6, a7, a8);
}
DETOUR_MID(xlive_5317)
{
    SAVE_REGISTERS();

    print_stack(5);
    log_xlive_call(5317);

    LOAD_REGISTERS();

    JUMP_TO(xlive_5317);
}
DETOUR_API(int, __stdcall, xlive_5318, int a1, int a2, int a3)
{
    log_xlive_call(5318);
    return xlive_5318(a1, a2, a3);
}
DETOUR_MID(xlive_5332)
{
    SAVE_REGISTERS();

    print_stack(2);
    log_xlive_call(5332);

    LOAD_REGISTERS();

    JUMP_TO(xlive_5332);
}
