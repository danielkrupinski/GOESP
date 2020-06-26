#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#endif

class Entity;
class ItemSystem;
class WeaponSystem;

struct ActiveChannels;
struct Channel;
struct GlobalVars;
struct Vector;

class Memory {
public:
    Memory() noexcept;

    std::uintptr_t reset;
    std::uintptr_t present;
    std::uintptr_t setCursorPos;

    const GlobalVars* globalVars;
    WeaponSystem* weaponSystem;
    ActiveChannels* activeChannels;
    Channel* channels;

#ifdef _WIN32
    bool(__thiscall* isOtherEnemy)(Entity*, Entity*);
    std::add_pointer_t<void __cdecl(const char* msg, ...)> debugMsg;
    std::add_pointer_t<ItemSystem* __cdecl()> itemSystem;
    std::add_pointer_t<bool __cdecl(Vector, Vector, short)> lineGoesThroughSmoke;
#else
    bool(*isOtherEnemy)(Entity*, Entity*);
    std::add_pointer_t<void(const char* msg, ...)> debugMsg;
    std::add_pointer_t<ItemSystem*()> itemSystem;
    std::add_pointer_t<bool(Vector, Vector, short)> lineGoesThroughSmoke;
#endif
private:
    static std::pair<void*, std::size_t> getModuleInformation(const char* name) noexcept
    {
#ifdef _WIN32
        if (HMODULE handle = GetModuleHandleA(name)) {
            if (MODULEINFO moduleInfo; GetModuleInformation(GetCurrentProcess(), handle, &moduleInfo, sizeof(moduleInfo)))
                return std::make_pair(moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);
        }
#endif
        return {};
    }

    static std::uintptr_t findPattern(const char* module, const char* pattern) noexcept
    {
        static auto id = 0;
        ++id;

        const auto [moduleBase, moduleSize] = getModuleInformation(module);

        if (moduleBase && moduleSize) {
            auto start = static_cast<const char*>(moduleBase);
            const auto end = start + moduleSize;

            auto first = start;
            auto second = pattern;

            while (first < end && *second) {
                if (*first == *second || *second == '?') {
                    ++first;
                    ++second;
                } else {
                    first = ++start;
                    second = pattern;
                }
            }

            if (!*second)
                return reinterpret_cast<std::uintptr_t>(start);
        }
#ifdef _WIN32
        MessageBoxA(NULL, ("Failed to find pattern #" + std::to_string(id) + '!').c_str(), "GOESP", MB_OK | MB_ICONWARNING);
#endif
        return 0;
    }
};

inline std::unique_ptr<const Memory> memory;
