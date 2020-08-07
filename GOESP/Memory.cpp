#include <cassert>

#ifdef __linux__
#include <dlfcn.h>
#endif

#include "Interfaces.h"
#include "Memory.h"
#include "SDK/LocalPlayer.h"

template <typename T>
static constexpr auto relativeToAbsolute(std::uintptr_t address) noexcept
{
    return (T)(address + 4 + *reinterpret_cast<std::int32_t*>(address));
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

#ifdef _WIN32
    debugMsg = decltype(debugMsg)(GetProcAddress(GetModuleHandleA(TIER0_DLL), "Msg"));

    if (overlay == Overlay::Discord) {
        if (!GetModuleHandleA("discordhook"))
            goto steamOverlay;
        reset = *reinterpret_cast<std::uintptr_t*>(findPattern("discordhook", "\x8B\x1F\x68") + 3);
        present = *reinterpret_cast<std::uintptr_t*>(findPattern("discordhook", "\x4E\x10\x8B\x45\xFC\x68") + 6);
        setCursorPos = *reinterpret_cast<std::uintptr_t*>(findPattern("discordhook", "\x74\x1B\x8B\x4D\x08") + 32);
    } else {
    steamOverlay:
        reset = *reinterpret_cast<std::uintptr_t*>(findPattern("gameoverlayrenderer", "\x53\x57\xC7\x45") + 11);
        present = reset + 4;
        setCursorPos = *reinterpret_cast<std::uintptr_t*>(findPattern("gameoverlayrenderer", "\xC2\x08?\x5D") + 6);
    }

    globalVars = **reinterpret_cast<GlobalVars***>((*reinterpret_cast<std::uintptr_t**>(interfaces->client))[11] + 10);
    weaponSystem = *reinterpret_cast<WeaponSystem**>(findPattern(CLIENT_DLL, "\x8B\x35????\xFF\x10\x0F\xB7\xC0") + 2);
    activeChannels = *reinterpret_cast<ActiveChannels**>(findPattern(ENGINE_DLL, "\x8B\x1D????\x89\x5C\x24\x48") + 2);
    channels = *reinterpret_cast<Channel**>(findPattern(ENGINE_DLL, "\x81\xC2????\x8B\x72\x54") + 2);
    plantedC4s = *reinterpret_cast<decltype(plantedC4s)*>(findPattern(CLIENT_DLL, "\x7E\x2C\x8B\x15") + 4);
    playerResource = *reinterpret_cast<PlayerResource***>(findPattern(CLIENT_DLL, "\x74\x30\x8B\x35????\x85\xF6") + 4);

    isOtherEnemy = relativeToAbsolute<decltype(isOtherEnemy)>(findPattern(CLIENT_DLL, "\x8B\xCE\xE8????\x02\xC0") + 3);
    itemSystem = relativeToAbsolute<decltype(itemSystem)>(findPattern(CLIENT_DLL, "\xE8????\x0F\xB7\x0F") + 1);
    lineGoesThroughSmoke = relativeToAbsolute<decltype(lineGoesThroughSmoke)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x4C\x24\x30\x33\xD2") + 1);
    getDecoratedPlayerName = relativeToAbsolute<decltype(getDecoratedPlayerName)>(findPattern(CLIENT_DLL, "\xE8????\x66\x83\x3E") + 1);

    localPlayer.init(*reinterpret_cast<Entity***>(findPattern(CLIENT_DLL, "\xA1????\x89\x45\xBC\x85\xC0") + 1));
#elif __linux__
    debugMsg = decltype(debugMsg)(dlsym(dlopen(TIER0_DLL, RTLD_NOLOAD | RTLD_NOW), "Msg"));

    globalVars = *relativeToAbsolute<GlobalVars**>((*reinterpret_cast<std::uintptr_t**>(interfaces->client))[11] + 16);
    itemSystem = relativeToAbsolute<decltype(itemSystem)>(findPattern(CLIENT_DLL, "\xE8????\x4D\x63\xEC") + 1);
    weaponSystem = *relativeToAbsolute<WeaponSystem**>(findPattern(CLIENT_DLL, "\x48\x8B\x58\x10\x48\x8B\x07\xFF\x10") + 12);
    localPlayer.init(relativeToAbsolute<Entity**>(findPattern(CLIENT_DLL, "\x83\xFF\xFF\x48\x8B\x05") + 6));

    isOtherEnemy = relativeToAbsolute<decltype(isOtherEnemy)>(findPattern(CLIENT_DLL, "\xE8????\x84\xC0\x44\x89\xE2") + 1);
    lineGoesThroughSmoke = reinterpret_cast<decltype(lineGoesThroughSmoke)>(findPattern(CLIENT_DLL, "\x40\x0F\xB6\xFF\x55"));
    getDecoratedPlayerName = relativeToAbsolute<decltype(getDecoratedPlayerName)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x33\x4C\x89\xF7") + 1);

    activeChannels = relativeToAbsolute<ActiveChannels*>(findPattern(ENGINE_DLL, "\x48\x8D\x3D????\x4C\x89\xE6\xE8????\x8B\xBD") + 3);
    channels = relativeToAbsolute<Channel*>(findPattern(ENGINE_DLL, "\x4C\x8D\x35????\x49\x83\xC4\x04") + 3);
    plantedC4s = *reinterpret_cast<decltype(plantedC4s)*>(findPattern(CLIENT_DLL, "\x48\x8D\x3D????\x49\x8B\x0C\x24") + 3);
    playerResource = relativeToAbsolute<PlayerResource**>(findPattern(CLIENT_DLL, "\x74\x38\x48\x8B\x3D????\x89\xDE") + 5);

    pollEvent = relativeToAbsolute<uintptr_t>(uintptr_t(dlsym(RTLD_NEXT, "SDL_PollEvent")) + 3);
    swapWindow = relativeToAbsolute<uintptr_t>(uintptr_t(dlsym(RTLD_NEXT, "SDL_GL_SwapWindow")) + 3);
    warpMouseInWindow = relativeToAbsolute<uintptr_t>(uintptr_t(dlsym(RTLD_NEXT, "SDL_WarpMouseInWindow")) + 3);
#endif
}
