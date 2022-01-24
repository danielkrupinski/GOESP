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

#include "Linux/LinuxFile.h"
#include "Linux/LinuxFileMap.h"
#endif

#include "Helpers.h"
#include "Interfaces.h"
#include "Memory.h"
#include "SDK/LocalPlayer.h"

using Helpers::relativeToAbsolute;

class DLLModule {
public:
    DLLModule(void* base, std::size_t size) : base{ base }, size{ size } {}

    std::uintptr_t findPattern(const char* pattern) noexcept
    {
        if (base && size) {
            auto start = static_cast<const char*>(base);
            const auto end = start + size;

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
        return 0;
    }

private:
    void* base = nullptr;
    std::size_t size = 0;
};

static DLLModule getModuleInformation(const char* name) noexcept
{
#ifdef _WIN32
    if (HMODULE handle = GetModuleHandleA(name)) {
        if (MODULEINFO moduleInfo; GetModuleInformation(GetCurrentProcess(), handle, &moduleInfo, sizeof(moduleInfo)))
            return DLLModule{ moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage };
    }
    return { nullptr, 0 };
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

        if (const auto file = LinuxFile{ info->dlpi_name, O_RDONLY }; file.isValid()) {
            if (const auto map = LinuxFileMap{ nullptr, static_cast<std::size_t>(file.getStatus().st_size), PROT_READ, MAP_PRIVATE, file.getDescriptor(), 0 }; map.isValid()) {
                const auto ehdr = (ElfW(Ehdr)*)map.get();
                const auto shdrs = (ElfW(Shdr)*)(std::uintptr_t(ehdr) + ehdr->e_shoff);
                const auto strTab = (const char*)(std::uintptr_t(ehdr) + shdrs[ehdr->e_shstrndx].sh_offset);

                for (auto i = 0; i < ehdr->e_shnum; ++i) {
                    const auto shdr = (ElfW(Shdr)*)(std::uintptr_t(shdrs) + i * ehdr->e_shentsize);

                    if (std::strcmp(strTab + shdr->sh_name, ".text") != 0)
                        continue;

                    moduleInfo->base = (void*)(info->dlpi_addr + shdr->sh_offset);
                    moduleInfo->size = shdr->sh_size;
                    return 1;
                }
            }
        }

        moduleInfo->base = (void*)(info->dlpi_addr + info->dlpi_phdr[0].p_vaddr);
        moduleInfo->size = info->dlpi_phdr[0].p_memsz;
        return 1;
    }, &moduleInfo);

    return DLLModule{ moduleInfo.base, moduleInfo.size };
#endif
}

static std::uintptr_t findPattern(const char* moduleName, const char* pattern) noexcept
{
    static auto id = 0;
    ++id;

    const auto result = getModuleInformation(moduleName).findPattern(pattern);

    assert(result != 0);
#ifdef _WIN32
    if (result == 0)
        MessageBoxA(nullptr, ("Failed to find pattern #" + std::to_string(id) + '!').c_str(), "GOESP", MB_OK | MB_ICONWARNING);
#endif
    return result;
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
    lineGoesThroughSmoke = reinterpret_cast<decltype(lineGoesThroughSmoke)>(findPattern(CLIENT_DLL, "\x55\x48\x89\xE5\x41\x56\x41\x55\x41\x54\x53\x48\x83\xEC\x30\x66\x0F\xD6\x45\xD0"));
    smokeHandles = relativeToAbsolute<decltype(smokeHandles)>(std::uintptr_t(lineGoesThroughSmoke) + 119);
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
#endif
}
