/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#define MAX_PATH 4096
#endif

#include <memory>
#include <string>
#include <vector>

namespace Memory {

struct ModuleInfo {
    char name[MAX_PATH];
    uintptr_t base;
    uintptr_t size;
    char path[MAX_PATH];
};

extern std::vector<ModuleInfo> moduleList;

auto TryGetModule(const char* moduleName, ModuleInfo* info) -> bool;
auto GetModulePath(const char* moduleName) -> const char*;
auto GetModuleHandleByName(const char* moduleName) -> uintptr_t;
auto CloseModuleHandle(uintptr_t moduleHandle) -> void;
auto GetProcessName() -> std::string;

auto FindAddress(const uintptr_t start, const uintptr_t end, const char* target) -> uintptr_t;
auto Scan(const char* moduleName, const char* pattern, int offset = 0) -> uintptr_t;
auto MultiScan(const char* moduleName, const char* pattern, int offset = 0) -> std::vector<uintptr_t>;

#ifdef _WIN32
class Patch {
private:
    uintptr_t location;
    unsigned char* original;
    size_t size;

public:
    auto GetLocation() -> uintptr_t { return this->location; }
    auto Execute(uintptr_t location, unsigned char* bytes, unsigned int size) -> bool;
    auto ExecuteForce(uintptr_t location, unsigned char* bytes, unsigned int size) -> bool;
    auto Restore() -> bool;
    auto RestoreForce() -> bool;
    static auto AddressAt(uintptr_t location, uintptr_t address) -> bool;
};
#endif

struct Pattern {
    const char* signature;
    std::vector<int> offsets;
};

typedef std::vector<int> Offset;
typedef std::vector<const Pattern*> Patterns;

#define PATTERN(name, sig, ...)                                                                                        \
    Memory::Pattern name                                                                                               \
    {                                                                                                                  \
        sig, Memory::Offset({ __VA_ARGS__ })                                                                           \
    }
#define PATTERNS(name, ...) Memory::Patterns name({ __VA_ARGS__ })

auto Scan(const char* moduleName, const Pattern* pattern) -> std::vector<uintptr_t>;
auto MultiScan(const char* moduleName, const Patterns* patterns) -> std::vector<std::vector<uintptr_t>>;

template <typename T = uintptr_t> inline auto Absolute(const char* moduleName, int relative) -> T
{
    auto info = Memory::ModuleInfo();
    return Memory::TryGetModule(moduleName, &info) ? T(info.base + relative) : T(0);
}
template <typename T = uintptr_t> inline auto GetSymbolAddress(uintptr_t moduleHandle, const char* symbolName) -> T
{
#ifdef _WIN32
    return T(GetProcAddress(HMODULE(moduleHandle), symbolName));
#else
    return T(dlsym(moduleHandle, symbolName));
#endif
}
template <typename T = uintptr_t> inline auto VMT(void* ptr, int index) -> T
{
    return reinterpret_cast<T>((*((void***)ptr))[index]);
}
template <typename T = uintptr_t> inline auto Read(uintptr_t source) -> T
{
    auto rel = *reinterpret_cast<int*>(source);
    return (T)(source + rel + sizeof(rel));
}
template <typename T = uintptr_t> inline auto Read(uintptr_t source, T* destination) -> void
{
    auto rel = *reinterpret_cast<int*>(source);
    *destination = (T)(source + rel + sizeof(rel));
}
template <typename T = uintptr_t> inline auto Deref(uintptr_t source) -> T { return *reinterpret_cast<T*>(source); }
template <typename T = uintptr_t> inline auto Deref(uintptr_t source, T* destination) -> void
{
    *destination = *reinterpret_cast<T*>(source);
}
template <typename T = uintptr_t> inline auto DerefDeref(uintptr_t source) -> T
{
    return **reinterpret_cast<T**>(source);
}
template <typename T = uintptr_t> inline auto DerefDeref(uintptr_t source, T* destination) -> void
{
    *destination = **reinterpret_cast<T**>(source);
}
template <typename T = uintptr_t> inline auto Scan(const char* moduleName, const char* pattern, int offset = 0) -> T
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
    return reinterpret_cast<T>(result);
}

class Interface {
public:
    uintptr_t** baseclass;
    uintptr_t* vtable;
    int vtableSize;

private:
    bool isHooked;
    std::unique_ptr<uintptr_t[]> copy;

public:
    Interface();
    Interface(uintptr_t baseclass, bool copyVtable = true, bool autoHook = true);
    ~Interface();

    auto CopyVtable() -> void;
    auto EnableHooks() -> void;
    auto DisableHooks() -> void;

    template <typename T = uintptr_t> auto Original(int index, bool readJmp = false) -> T
    {
        if (readJmp) {
            auto source = this->vtable[index] + 1;
            auto rel = *reinterpret_cast<uintptr_t*>(source);
            return (T)(source + rel + sizeof(rel));
        }
        return (T)this->vtable[index];
    }
    template <typename T = uintptr_t> auto Hooked(int index) -> T { return (T)this->copy[index + 1]; }
    template <typename T = uintptr_t> auto Current(int index) -> T { return (T)(*this->baseclass)[index]; }
    template <typename T = uintptr_t, typename U = uintptr_t> auto Hook(T detour, U& original, int index) -> bool
    {
        if (index >= 0 && index < this->vtableSize) {
            this->copy[index + 1] = reinterpret_cast<uintptr_t>(detour);
            original = this->Original<U>(index);
            return true;
        }
        return false;
    }

    auto Unhook(int index) -> bool;

    inline auto ThisPtr() -> uintptr_t { return uintptr_t(this->baseclass); }

    static auto Create(uintptr_t ptr, bool copyVtable = true, bool autoHook = true) -> Interface*;
    static auto Delete(Interface* ptr) -> void;
};
}
