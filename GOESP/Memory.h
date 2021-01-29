#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "SDK/Platform.h"

class CSPlayer;
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
    std::uintptr_t warpMouseInWindow;
#endif

private:
    std::uintptr_t getGameModeNameFn;
};

inline std::unique_ptr<const Memory> memory;
