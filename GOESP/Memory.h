#pragma once

#include <cstdint>
#include <optional>
#include <type_traits>

#include "SDK/Platform.h"

class CSPlayer;
class Entity;
class ItemSystem;
class PlantedC4;
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
    UtlVector<PlantedC4*>* plantedC4s;
    UtlVector<int>* smokeHandles;
    PlayerResource** playerResource;
    Entity** gameRules;

    bool(__THISCALL* isOtherEnemy)(CSPlayer*, CSPlayer*);
    std::add_pointer_t<void __CDECL(const char* msg, ...)> debugMsg;
#ifdef __APPLE__
    ItemSystem** itemSystem;
#else
    std::add_pointer_t<ItemSystem* __CDECL()> itemSystem;
#endif
    std::add_pointer_t<bool __CDECL(Vector, Vector, short)> lineGoesThroughSmoke;
    const wchar_t*(__THISCALL* getDecoratedPlayerName)(PlayerResource* pr, int index, wchar_t* buffer, int buffsize, int flags);

    const char* getGameModeName(bool skirmish) const noexcept
    {
#ifdef _WIN32
        return reinterpret_cast<const char*(__stdcall*)(bool)>(getGameModeNameFn)(skirmish);
#else
        return reinterpret_cast<const char*(*)(void*, bool)>(getGameModeNameFn)(nullptr, skirmish);
#endif
    }

#ifdef _WIN32
    std::uintptr_t reset;
    std::uintptr_t present;
    std::uintptr_t setCursorPos;
#else
    std::uintptr_t pollEvent;
    std::uintptr_t swapWindow;
#endif

private:
    std::uintptr_t getGameModeNameFn;
};

inline std::optional<const Memory> memory;
