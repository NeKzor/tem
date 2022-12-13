/*
* Copyright (c) 2022 NeKz
* 
* SPDX-License-Identifier: MIT
*/

#include "TEM.hpp"
#include <Windows.h>

auto APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) -> BOOL
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        CreateThread(0, 0, LPTHREAD_START_ROUTINE(tem_attach), module, 0, 0);
    } else if (reason == DLL_PROCESS_DETACH) {
        tem_detach();
    }
    return TRUE;
}
