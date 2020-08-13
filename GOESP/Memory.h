#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#elif __linux__
#include <link.h>
#endif

#include "SDK/Platform.h"

class Entity;
class ItemSystem;
class PlayerResource;
class WeaponSystem;
template <typename T>
class UtlVector;

struct ActiveChannels;
struct Channel;
struct GlobalVars;
struct Vector;

class Memory {
public:
    Memory() noexcept;

    const GlobalVars* globalVars;
    WeaponSystem* weaponSystem;
    ActiveChannels* activeChannels;
    Channel* channels;
    UtlVector<Entity*>* plantedC4s;
    PlayerResource** playerResource;

    bool(__THISCALL* isOtherEnemy)(Entity*, Entity*);
    std::add_pointer_t<void __CDECL(const char* msg, ...)> debugMsg;
    std::add_pointer_t<ItemSystem* __CDECL()> itemSystem;
    std::add_pointer_t<bool __CDECL(Vector, Vector, short)> lineGoesThroughSmoke;
    const wchar_t*(__THISCALL* getDecoratedPlayerName)(PlayerResource* pr, int index, wchar_t* buffer, int buffsize, int flags);

#ifdef _WIN32
    std::uintptr_t reset;
    std::uintptr_t present;
    std::uintptr_t setCursorPos;
#elif __linux__
    std::uintptr_t pollEvent;
    std::uintptr_t swapWindow;
    std::uintptr_t warpMouseInWindow;
#endif
private:
    static std::pair<void*, std::size_t> getModuleInformation(const char* name) noexcept
    {
#ifdef _WIN32
        if (HMODULE handle = GetModuleHandleA(name)) {
            if (MODULEINFO moduleInfo; GetModuleInformation(GetCurrentProcess(), handle, &moduleInfo, sizeof(moduleInfo)))
                return std::make_pair(moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);
        }
        return {};
#elif __linux__
        struct ModuleInfo {
            const char* name;
            void* base = nullptr;
            std::size_t size = 0;
        } moduleInfo;

        moduleInfo.name = name;

        dl_iterate_phdr([](struct dl_phdr_info* info, std::size_t, void* data) {
            const auto moduleInfo = reinterpret_cast<ModuleInfo*>(data);
       	    if (std::string_view{ info->dlpi_name }.ends_with(moduleInfo->name)) {
                moduleInfo->base = (void*)(info->dlpi_addr + info->dlpi_phdr[0].p_vaddr);
                moduleInfo->size = info->dlpi_phdr[0].p_memsz;
                return 1;
       	    }
            return 0;
        }, &moduleInfo);
            
       return std::make_pair(moduleInfo.base, moduleInfo.size);
#endif
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
