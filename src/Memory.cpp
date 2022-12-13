/*
 * Copyright (c) 2022 NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#include "Memory.hpp"

#include <cstring>
#include <memory>
#include <vector>

#ifdef _WIN32
#include <tchar.h>
#include <windows.h>
// Last
#include <psapi.h>
#else
#include <cstdint>
#include <dlfcn.h>
#include <link.h>
#include <sys/uio.h>
#include <unistd.h>
#endif

#define INRANGE(x, a, b) (x >= a && x <= b)
#define getBits(x)                                                                                                     \
    (INRANGE((x & (~0x20)), 'A', 'F') ? ((x & (~0x20)) - 'A' + 0xA) : (INRANGE(x, '0', '9') ? x - '0' : 0))
#define getByte(x) (getBits(x[0]) << 4 | getBits(x[1]))

std::vector<Memory::ModuleInfo> Memory::moduleList;

auto Memory::TryGetModule(const char* moduleName, Memory::ModuleInfo* info) -> bool
{
    if (Memory::moduleList.empty()) {
#ifdef _WIN32
        HMODULE hMods[1024];
        HANDLE pHandle = GetCurrentProcess();
        DWORD cbNeeded;
        if (EnumProcessModules(pHandle, hMods, sizeof(hMods), &cbNeeded)) {
            for (unsigned i = 0; i < (cbNeeded / sizeof(HMODULE)); ++i) {
                char buffer[MAX_PATH];
                if (!GetModuleFileNameA(hMods[i], buffer, sizeof(buffer))) {
                    continue;
                }

                auto modinfo = MODULEINFO();
                if (!GetModuleInformation(pHandle, hMods[i], &modinfo, sizeof(modinfo))) {
                    continue;
                }

                auto module = ModuleInfo();

                auto temp = std::string(buffer);
                auto index = temp.find_last_of("\\/");
                temp = temp.substr(index + 1, temp.length() - index);

                std::snprintf(module.name, sizeof(module.name), "%s", temp.c_str());
                module.base = uintptr_t(modinfo.lpBaseOfDll);
                module.size = uintptr_t(modinfo.SizeOfImage);
                std::snprintf(module.path, sizeof(module.path), "%s", buffer);

                Memory::moduleList.push_back(module);
            }
        }

#else
        dl_iterate_phdr(
            [](struct dl_phdr_info* info, size_t, uintptr_t) {
                auto module = Memory::ModuleInfo();

                auto temp = std::string(info->dlpi_name);
                auto index = temp.find_last_of("\\/");
                temp = temp.substr(index + 1, temp.length() - index);
                std::snprintf(module.name, sizeof(module.name), "%s", temp.c_str());

                module.base = info->dlpi_addr + info->dlpi_phdr[0].p_paddr;
                module.size = info->dlpi_phdr[0].p_memsz;
                std::strncpy(module.path, info->dlpi_name, sizeof(module.path));

                Memory::moduleList.push_back(module);
                return 0;
            },
            nullptr);
#endif
    }

    for (Memory::ModuleInfo& item : Memory::moduleList) {
        if (!std::strcmp(item.name, moduleName)) {
            if (info) {
                *info = item;
            }
            return true;
        }
    }

    return false;
}
auto Memory::GetModulePath(const char* moduleName) -> const char*
{
    auto info = Memory::ModuleInfo();
    return (Memory::TryGetModule(moduleName, &info)) ? std::string(info.path).c_str() : nullptr;
}
auto Memory::GetModuleHandleByName(const char* moduleName) -> uintptr_t
{
    auto info = Memory::ModuleInfo();
#ifdef _WIN32
    return uintptr_t(Memory::TryGetModule(moduleName, &info) ? GetModuleHandleA(info.path) : nullptr);
#else
    return (TryGetModule(moduleName, &info)) ? dlopen(info.path, RTLD_NOLOAD | RTLD_NOW) : nullptr;
#endif
}
auto Memory::CloseModuleHandle(uintptr_t moduleHandle) -> void
{
#ifndef _WIN32
    dlclose(moduleHandle);
#endif
}
std::string Memory::GetProcessName()
{
#ifdef _WIN32
    char temp[MAX_PATH];
    GetModuleFileNameA(NULL, temp, sizeof(temp));
#else
    char link[32];
    char temp[MAX_PATH] = { 0 };
    std::sprintf(link, "/proc/%d/exe", getpid());
    readlink(link, temp, sizeof(temp));
#endif

    auto proc = std::string(temp);
    auto index = proc.find_last_of("\\/");
    proc = proc.substr(index + 1, proc.length() - index);

    return proc;
}

auto Memory::FindAddress(const uintptr_t start, const uintptr_t end, const char* target) -> uintptr_t
{
    const char* pattern = target;
    uintptr_t result = 0;

    for (auto position = start; position < end; ++position) {
        if (!*pattern)
            return result;

        auto match = *reinterpret_cast<const uint8_t*>(pattern);
        auto byte = *reinterpret_cast<const uint8_t*>(position);

        if (match == '\?' || byte == getByte(pattern)) {
            if (!result)
                result = position;

            if (!pattern[2])
                return result;

            pattern += (match != '\?') ? 3 : 2;
        } else {
            pattern = target;
            result = 0;
        }
    }
    return 0;
}
auto Memory::Scan(const char* moduleName, const char* pattern, int offset) -> uintptr_t
{
    uintptr_t result = 0;

    auto info = Memory::ModuleInfo();
    if (Memory::TryGetModule(moduleName, &info)) {
        auto start = uintptr_t(info.base);
        auto end = start + info.size;
        result = Memory::FindAddress(start, end, pattern);
        if (result) {
            result += offset;
        }
    }
    return result;
}
auto Memory::MultiScan(const char* moduleName, const char* pattern, int offset) -> std::vector<uintptr_t>
{
    std::vector<uintptr_t> result;
    auto length = std::strlen(pattern);

    auto info = Memory::ModuleInfo();
    if (Memory::TryGetModule(moduleName, &info)) {
        auto start = uintptr_t(info.base);
        auto end = start + info.size;
        auto addr = uintptr_t();
        while (true) {
            auto addr = Memory::FindAddress(start, end, pattern);
            if (addr) {
                result.push_back(addr + offset);
                start = addr + length;
            } else {
                break;
            }
        }
    }
    return result;
}

auto Memory::Scan(const char* moduleName, const Memory::Pattern* pattern) -> std::vector<uintptr_t>
{
    std::vector<uintptr_t> result;

    auto info = Memory::ModuleInfo();
    if (Memory::TryGetModule(moduleName, &info)) {
        auto start = uintptr_t(info.base);
        auto end = start + info.size;
        auto addr = Memory::FindAddress(start, end, pattern->signature);
        if (addr) {
            for (auto const& offset : pattern->offsets) {
                result.push_back(addr + offset);
            }
        }
    }
    return result;
}
auto Memory::MultiScan(const char* moduleName, const Memory::Patterns* patterns) -> std::vector<std::vector<uintptr_t>>
{
    auto results = std::vector<std::vector<uintptr_t>>();

    auto info = Memory::ModuleInfo();
    if (Memory::TryGetModule(moduleName, &info)) {
        auto moduleStart = uintptr_t(info.base);
        auto moduleEnd = moduleStart + info.size;

        for (const auto& pattern : *patterns) {
            auto length = std::strlen(pattern->signature);
            auto start = moduleStart;

            while (true) {
                auto addr = Memory::FindAddress(start, moduleEnd, pattern->signature);
                if (addr) {
                    auto result = std::vector<uintptr_t>();
                    for (const auto& offset : pattern->offsets) {
                        result.push_back(addr + offset);
                    }
                    results.push_back(result);
                    start = addr + length;
                } else {
                    break;
                }
            }
        }
    }
    return results;
}

#ifdef _WIN32
auto Memory::Patch::Execute(uintptr_t location, unsigned char* bytes, unsigned int size) -> bool
{
    this->location = location;
    this->size = size;
    this->original = new unsigned char[this->size];

    auto proc = GetCurrentProcess();

    if (!ReadProcessMemory(proc, LPVOID(this->location), &this->original, this->size, 0)) {
        return false;
    }

    return WriteProcessMemory(proc, LPVOID(this->location), reinterpret_cast<LPCVOID>(bytes), this->size, 0);
}
auto Memory::Patch::AddressAt(uintptr_t location, uintptr_t address) -> bool
{
    auto proc = GetCurrentProcess();

    union {
        char bytes[4];
        uintptr_t address;
    } temp = { .address = address };

    DWORD oldProtect = 0;
    VirtualProtectEx(GetCurrentProcess(), LPVOID(location), sizeof(temp), PAGE_EXECUTE_READWRITE, &oldProtect);

    auto result = WriteProcessMemory(proc, LPVOID(location), reinterpret_cast<LPCVOID>(temp.bytes), sizeof(temp), 0);

    VirtualProtectEx(GetCurrentProcess(), LPVOID(location), sizeof(temp), oldProtect, 0);
    return result;
}
auto Memory::Patch::ExecuteForce(uintptr_t location, unsigned char* bytes, unsigned int size) -> bool
{
    this->location = location;
    this->size = size;
    this->original = new unsigned char[this->size];

    DWORD oldProtect = 0;
    VirtualProtectEx(GetCurrentProcess(), LPVOID(this->location), this->size, PAGE_EXECUTE_READWRITE, &oldProtect);

    auto result = ReadProcessMemory(GetCurrentProcess(), LPVOID(this->location), &this->original, this->size, 0)
        && WriteProcessMemory(
            GetCurrentProcess(), LPVOID(this->location), reinterpret_cast<LPCVOID>(bytes), this->size, 0);

    VirtualProtectEx(GetCurrentProcess(), LPVOID(this->location), this->size, oldProtect, 0);

    return result;
}
auto Memory::Patch::Restore() -> bool
{
    return this->location && this->original
        && WriteProcessMemory(GetCurrentProcess(), LPVOID(this->location), &this->original, this->size, 0);
}
auto Memory::Patch::RestoreForce() -> bool
{
    if (!this->location || !this->original) {
        return false;
    }

    auto proc = GetCurrentProcess();

    DWORD oldProtect = 0;
    VirtualProtectEx(proc, LPVOID(this->location), this->size, PAGE_EXECUTE_READWRITE, &oldProtect);

    auto result = WriteProcessMemory(proc, LPVOID(this->location), &this->original, this->size, 0);

    VirtualProtectEx(proc, LPVOID(this->location), this->size, oldProtect, 0);

    return result;
}
#endif

Memory::Interface::Interface()
    : baseclass(nullptr)
    , vtable(nullptr)
    , vtableSize(0)
    , isHooked(false)
    , copy(nullptr)
{
}
Memory::Interface::Interface(uintptr_t baseclass, bool copyVtable, bool autoHook)
    : Interface()
{
    this->baseclass = reinterpret_cast<uintptr_t**>(baseclass);
    this->vtable = *this->baseclass;

    while (this->vtable[this->vtableSize]) {
        ++this->vtableSize;
    }

    if (copyVtable) {
        this->CopyVtable();
        if (autoHook) {
            this->EnableHooks();
        }
    }
}
Memory::Interface::~Interface()
{
    this->DisableHooks();
    if (this->copy) {
        this->copy.reset();
    }
}
auto Memory::Interface::CopyVtable() -> void
{
    if (!this->copy) {
        this->copy = std::make_unique<uintptr_t[]>(this->vtableSize + 1);
        std::memcpy(this->copy.get(), this->vtable - 1, sizeof(uintptr_t) + this->vtableSize * sizeof(uintptr_t));
    }
}
auto Memory::Interface::EnableHooks() -> void
{
    if (!this->isHooked) {
        *this->baseclass = this->copy.get() + 1;
        this->isHooked = true;
    }
}
auto Memory::Interface::DisableHooks() -> void
{
    if (this->isHooked) {
        *this->baseclass = this->vtable;
        this->isHooked = false;
    }
}
auto Memory::Interface::Unhook(int index) -> bool
{
    if (index >= 0 && index < this->vtableSize) {
        this->copy[index + 1] = this->vtable[index];
        return true;
    }
    return false;
}
auto Memory::Interface::Create(uintptr_t ptr, bool copyVtable, bool autoHook) -> Memory::Interface*
{
    return (ptr) ? new Interface(ptr, copyVtable, autoHook) : nullptr;
}
auto Memory::Interface::Delete(Interface* ptr) -> void
{
    if (ptr) {
        delete ptr;
        ptr = nullptr;
    }
}
