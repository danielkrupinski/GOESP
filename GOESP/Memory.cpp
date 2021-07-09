#include <cassert>
#include <cstring>
#include <string_view>
#include <utility>

#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#elif __linux__
#include <dlfcn.h>
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#endif

#include "Interfaces.h"
#include "Memory.h"
#include "SDK/LocalPlayer.h"

template <typename T>
static constexpr auto relativeToAbsolute(std::uintptr_t address) noexcept
{
    return (T)(address + 4 + *reinterpret_cast<std::int32_t*>(address));
}

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
        if (!std::string_view{ info->dlpi_name }.ends_with(moduleInfo->name))
            return 0;

        if (const auto fd = open(info->dlpi_name, O_RDONLY); fd >= 0) {
            if (struct stat st; fstat(fd, &st) == 0) {
                if (const auto map = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0); map != MAP_FAILED) {
                    const auto ehdr = (ElfW(Ehdr)*)map;
                    const auto shdrs = (ElfW(Shdr)*)(std::uintptr_t(ehdr) + ehdr->e_shoff);
                    const auto strTab = (const char*)(std::uintptr_t(ehdr) + shdrs[ehdr->e_shstrndx].sh_offset);

                    for (auto i = 0; i < ehdr->e_shnum; ++i) {
                        const auto shdr = (ElfW(Shdr)*)(std::uintptr_t(shdrs) + i * ehdr->e_shentsize);

                        if (std::strcmp(strTab + shdr->sh_name, ".text") != 0)
                            continue;

                        moduleInfo->base = (void*)(info->dlpi_addr + shdr->sh_offset);
                        moduleInfo->size = shdr->sh_size;
                        munmap(map, st.st_size);
                        close(fd);
                        return 1;
                    }
                    munmap(map, st.st_size);
                }
            }
            close(fd);
        }

        moduleInfo->base = (void*)(info->dlpi_addr + info->dlpi_phdr[0].p_vaddr);
        moduleInfo->size = info->dlpi_phdr[0].p_memsz;
        return 1;
    }, &moduleInfo);

    return std::make_pair(moduleInfo.base, moduleInfo.size);
#elif __APPLE__
    return {};
#endif
}

static std::uintptr_t findPattern(const char* moduleName, const char* pattern) noexcept
{
    static auto id = 0;
    ++id;

    const auto [moduleBase, moduleSize] = getModuleInformation(moduleName);

    if (moduleBase && moduleSize) {
        auto start = static_cast<const char*>(moduleBase);
        const auto end = start + moduleSize;

        auto first = start;
        auto second = pattern;

        while (first < end && *second) {
            if (*first == *second || *second == '?') {
                ++first;
                ++second;
            }
            else {
                first = ++start;
                second = pattern;
            }
        }

        if (!*second)
            return reinterpret_cast<std::uintptr_t>(start);
    }

    assert(false);
#ifdef _WIN32
    MessageBoxA(nullptr, ("Failed to find pattern #" + std::to_string(id) + '!').c_str(), "GOESP", MB_OK | MB_ICONWARNING);
#endif
    return 0;
}

Memory::Memory() noexcept
{
    assert(interfaces);

#ifdef _WIN32
    debugMsg = decltype(debugMsg)(GetProcAddress(GetModuleHandleA(TIER0_DLL), "Msg"));

    enum class Overlay {
        Steam,
        Discord
    };

    constexpr auto overlay = Overlay::Steam;
    static_assert(overlay == Overlay::Steam || overlay == Overlay::Discord, "Invalid overlay selected!");

    if (overlay == Overlay::Discord) {
        if (!GetModuleHandleA("discordhook"))
            goto steamOverlay;
        present = *reinterpret_cast<std::uintptr_t*>(findPattern("discordhook", "\x83\xC4\x0C\xFF\x75\x18") + 14);
        reset = present + 60;
        setCursorPos = *reinterpret_cast<std::uintptr_t*>(findPattern("discordhook", "\xC2\x08?\x5D") + 6);
    } else {
    steamOverlay:
        reset = *reinterpret_cast<std::uintptr_t*>(findPattern("gameoverlayrenderer", "\x57\x53\xC7\x45") + 11);
        present = reset + 4;
        setCursorPos = *reinterpret_cast<std::uintptr_t*>(findPattern("gameoverlayrenderer", "\xC2\x08?\x5D") + 6);
    }

    globalVars = **reinterpret_cast<GlobalVars***>((*reinterpret_cast<std::uintptr_t**>(interfaces->client))[11] + 10);
    weaponSystem = *reinterpret_cast<WeaponSystem**>(findPattern(CLIENT_DLL, "\x8B\x35????\xFF\x10\x0F\xB7\xC0") + 2);
    activeChannels = *reinterpret_cast<ActiveChannels**>(findPattern(ENGINE_DLL, "\x8B\x1D????\x89\x5C\x24\x48") + 2);
    channels = *reinterpret_cast<Channel**>(findPattern(ENGINE_DLL, "\x81\xC2????\x8B\x72\x54") + 2);
    plantedC4s = *reinterpret_cast<decltype(plantedC4s)*>(findPattern(CLIENT_DLL, "\x7E\x2C\x8B\x15") + 4);
    playerResource = *reinterpret_cast<PlayerResource***>(findPattern(CLIENT_DLL, "\x74\x30\x8B\x35????\x85\xF6") + 4);
    gameRules = *reinterpret_cast<Entity***>(findPattern(CLIENT_DLL, "\x8B\xEC\x8B\x0D????\x85\xC9\x74\x07") + 4);

    isOtherEnemy = relativeToAbsolute<decltype(isOtherEnemy)>(findPattern(CLIENT_DLL, "\x8B\xCE\xE8????\x02\xC0") + 3);
    itemSystem = relativeToAbsolute<decltype(itemSystem)>(findPattern(CLIENT_DLL, "\xE8????\x0F\xB7\x0F") + 1);
    lineGoesThroughSmoke = relativeToAbsolute<decltype(lineGoesThroughSmoke)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x4C\x24\x30\x33\xD2") + 1);
    smokeHandles = *reinterpret_cast<decltype(smokeHandles)*>(std::uintptr_t(lineGoesThroughSmoke) + 46);
    getDecoratedPlayerName = relativeToAbsolute<decltype(getDecoratedPlayerName)>(findPattern(CLIENT_DLL, "\xE8????\x66\x83\x3E") + 1);
    getGameModeNameFn = findPattern(CLIENT_DLL, "\x55\x8B\xEC\x8B\x0D????\x53\x57\x8B\x01");

    localPlayer.init(*reinterpret_cast<CSPlayer***>(findPattern(CLIENT_DLL, "\xA1????\x89\x45\xBC\x85\xC0") + 1));
#elif __linux__
    debugMsg = decltype(debugMsg)(dlsym(dlopen(TIER0_DLL, RTLD_NOLOAD | RTLD_NOW), "Msg"));

    globalVars = *relativeToAbsolute<GlobalVars**>((*reinterpret_cast<std::uintptr_t**>(interfaces->client))[11] + 16);
    itemSystem = relativeToAbsolute<decltype(itemSystem)>(findPattern(CLIENT_DLL, "\xE8????\x4D\x63\xEC") + 1);
    weaponSystem = *relativeToAbsolute<WeaponSystem**>(findPattern(CLIENT_DLL, "\x48\x8B\x58\x10\x48\x8B\x07\xFF\x10") + 12);
    localPlayer.init(relativeToAbsolute<CSPlayer**>(findPattern(CLIENT_DLL, "\x83\xFF\xFF\x48\x8B\x05") + 6));

    isOtherEnemy = relativeToAbsolute<decltype(isOtherEnemy)>(findPattern(CLIENT_DLL, "\xE8????\x84\xC0\x44\x89\xE2") + 1);
    lineGoesThroughSmoke = reinterpret_cast<decltype(lineGoesThroughSmoke)>(findPattern(CLIENT_DLL, "\x40\x0F\xB6\xFF\x55"));
    smokeHandles = relativeToAbsolute<decltype(smokeHandles)>(std::uintptr_t(lineGoesThroughSmoke) + 143);
    getDecoratedPlayerName = relativeToAbsolute<decltype(getDecoratedPlayerName)>(findPattern(CLIENT_DLL, "\xE8????\x8B\x33\x4C\x89\xF7") + 1);

    activeChannels = relativeToAbsolute<ActiveChannels*>(findPattern(ENGINE_DLL, "\x48\x8D\x3D????\x4C\x89\xE6\xE8????\x8B\xBD") + 3);
    channels = relativeToAbsolute<Channel*>(findPattern(ENGINE_DLL, "\x4C\x8D\x35????\x49\x83\xC4\x04") + 3);
    plantedC4s = relativeToAbsolute<decltype(plantedC4s)>(findPattern(CLIENT_DLL, "\x48\x8D\x3D????\x42\xC6\x44\x28") + 3);
    playerResource = relativeToAbsolute<PlayerResource**>(findPattern(CLIENT_DLL, "\x74\x38\x48\x8B\x3D????\x89\xDE") + 5);
    gameRules = *relativeToAbsolute<Entity***>(findPattern(CLIENT_DLL, "\x48\x8B\x1D????\x48\x8B\x3B\x48\x85\xFF\x74\x06") + 3);

    getGameModeNameFn = findPattern(CLIENT_DLL, "\x48\x8B\x05????\x55\x48\x89\xE5\x41\x54\x41\x89\xF4\x53\x48\x8B\x38\x48\x8B\x07\xFF\x50\x60");

    const auto libSDL = dlopen("libSDL2-2.0.so.0", RTLD_LAZY | RTLD_NOLOAD);
    pollEvent = relativeToAbsolute<uintptr_t>(uintptr_t(dlsym(libSDL, "SDL_PollEvent")) + 2);
    swapWindow = relativeToAbsolute<uintptr_t>(uintptr_t(dlsym(libSDL, "SDL_GL_SwapWindow")) + 2);
    dlclose(libSDL);
#elif __APPLE__
    debugMsg = decltype(debugMsg)(dlsym(dlopen(TIER0_DLL, RTLD_NOLOAD | RTLD_NOW), "Msg"));

    globalVars = *relativeToAbsolute<GlobalVars**>((*reinterpret_cast<std::uintptr_t**>(interfaces->client))[11] + 18);
    itemSystem = relativeToAbsolute<decltype(itemSystem)>(findPattern(CLIENT_DLL, "\x74\x06\x48\x83\xC7\x08") - 7);
    weaponSystem = *relativeToAbsolute<WeaponSystem**>(findPattern(CLIENT_DLL, "\x74\x1F\x48\x8B\x1D") + 5);
    localPlayer.init(relativeToAbsolute<CSPlayer**>(findPattern(CLIENT_DLL, "\x74\x10\x48\x63\xC7") + 8));

    isOtherEnemy = relativeToAbsolute<decltype(isOtherEnemy)>(findPattern(CLIENT_DLL, "\xE8????\x34\x01\xEB\x06") + 1);
    lineGoesThroughSmoke = relativeToAbsolute<decltype(lineGoesThroughSmoke)>(findPattern(CLIENT_DLL, "\xE8????\x84\xC0\x75\x20\x4C\x89\xFF") + 1);
    getDecoratedPlayerName = relativeToAbsolute<decltype(getDecoratedPlayerName)>(findPattern(CLIENT_DLL, "\xE8????\x41\x83\xFE\x07") + 1);

    const auto channelsTemp = findPattern(ENGINE_DLL, "\x45\x31\xE4\x48\x8D\x1D????\x66\x0F\x1F\x44");
    activeChannels = relativeToAbsolute<ActiveChannels*>(channelsTemp - 61);
    channels = relativeToAbsolute<Channel*>(channelsTemp + 6);
    plantedC4s = reinterpret_cast<decltype(plantedC4s)>(relativeToAbsolute<uintptr_t>(findPattern(CLIENT_DLL, "\x8B\x1D????\x45\x84\xFF\x74\x44") + 2) - 4);
    playerResource = relativeToAbsolute<PlayerResource**>(findPattern(CLIENT_DLL, "\x48\x8D\x05????\x48\x8B\x18\x48\x85\xDB\x74\x26") + 3);
    
    const auto libSDL = dlopen("libsdl2-2.0.0.dylib", RTLD_LAZY | RTLD_NOLOAD);
    pollEvent = relativeToAbsolute<uintptr_t>(uintptr_t(dlsym(libSDL, "SDL_PollEvent")) + 2);
    swapWindow = relativeToAbsolute<uintptr_t>(uintptr_t(dlsym(libSDL, "SDL_GL_SwapWindow")) + 2);
    dlclose(libSDL);
#endif
}
