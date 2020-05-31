#include <cassert>

#include "Interfaces.h"
#include "Memory.h"
#include "SDK/LocalPlayer.h"

template <typename T>
static constexpr auto relativeToAbsolute(std::uintptr_t address) noexcept
{
    return reinterpret_cast<T>(address + 4 + *reinterpret_cast<std::uintptr_t*>(address));
}

enum class Overlay {
    Steam,
    Discord
};

constexpr auto overlay = Overlay::Steam;

static_assert(overlay == Overlay::Steam || overlay == Overlay::Discord, "Invalid overlay selected!");

Memory::Memory() noexcept
{
    assert(interfaces);

    debugMsg = decltype(debugMsg)(GetProcAddress(GetModuleHandleW(L"tier0"), "Msg"));

    if (GetModuleHandleW(L"discordhook") && overlay == Overlay::Discord) {
        reset = *reinterpret_cast<std::uintptr_t*>(findPattern(L"discordhook", "\x8B\x1F\x68") + 3);
        present = *reinterpret_cast<std::uintptr_t*>(findPattern(L"discordhook", "\x8B\x46\x10\x68") + 4);
        setCursorPos = *reinterpret_cast<std::uintptr_t*>(findPattern(L"discordhook", "\x74\x1B\x8B\x4D\x08") + 32);
    } else {
        reset = *reinterpret_cast<std::uintptr_t*>(findPattern(L"gameoverlayrenderer", "\x53\x57\xC7\x45") + 11);
        present = reset + 4;
        setCursorPos = *reinterpret_cast<std::uintptr_t*>(findPattern(L"gameoverlayrenderer", "\xC2\x08?\x5D") + 6);
    }

    isOtherEnemy = relativeToAbsolute<decltype(isOtherEnemy)>(findPattern(L"client_panorama", "\xE8????\x02\xC0") + 1);
    globalVars = **reinterpret_cast<GlobalVars***>((*reinterpret_cast<std::uintptr_t**>(interfaces->client))[11] + 10);
    itemSystem = relativeToAbsolute<decltype(itemSystem)>(findPattern(L"client_panorama", "\xE8????\x0F\xB7\x0F") + 1);
    weaponSystem = *reinterpret_cast<WeaponSystem**>(findPattern(L"client_panorama", "\x8B\x35????\xFF\x10\x0F\xB7\xC0") + 2);
    activeChannels = *reinterpret_cast<ActiveChannels**>(findPattern(L"engine", "\x8B\x1D????\x89\x5C\x24\x48") + 2);
    channels = *reinterpret_cast<Channel**>(findPattern(L"engine", "\x81\xC2????\x8B\x72\x54") + 2);
    lineGoesThroughSmoke = relativeToAbsolute<decltype(lineGoesThroughSmoke)>(findPattern(L"client_panorama", "\xE8????\x8B\x4C\x24\x30\x33\xD2") + 1);

    localPlayer.init(*reinterpret_cast<Entity***>(findPattern(L"client_panorama", "\xA1????\x89\x45\xBC\x85\xC0") + 1));
}
