#pragma once

#include <memory>
#include <string>
#include <type_traits>

#include "SDK/Platform.h"

#ifdef _WIN32
#include <Windows.h>
#elif __linux__
#include <dlfcn.h>
#endif

class Interfaces {
public:
#define GAME_INTERFACE(type, name, module, version) \
class type* name = reinterpret_cast<type*>(find(module, version));

    GAME_INTERFACE(Client, client, CLIENT_DLL, "VClient018")
    GAME_INTERFACE(ClientTools, clientTools, CLIENT_DLL, "VCLIENTTOOLS001")
    GAME_INTERFACE(Cvar, cvar, VSTDLIB_DLL, "VEngineCvar007")
    GAME_INTERFACE(Engine, engine, ENGINE_DLL, "VEngineClient014")
    GAME_INTERFACE(EngineTrace, engineTrace, ENGINE_DLL, "EngineTraceClient004")
    GAME_INTERFACE(EntityList, entityList, CLIENT_DLL, "VClientEntityList003")
    GAME_INTERFACE(GameEventManager, gameEventManager, ENGINE_DLL, "GAMEEVENTSMANAGER002")
    GAME_INTERFACE(InputSystem, inputSystem, INPUTSYSTEM_DLL, "InputSystemVersion001")
    GAME_INTERFACE(Localize, localize, LOCALIZE_DLL, "Localize_001")
    GAME_INTERFACE(ModelInfo, modelInfo, ENGINE_DLL, "VModelInfoClient004")

#undef GAME_INTERFACE

private:
    static void* find(const char* module, const char* name) noexcept
    {
        if (const auto createInterface = reinterpret_cast<std::add_pointer_t<void* __CDECL(const char* name, int* returnCode)>>(
#ifdef _WIN32
            GetProcAddress(GetModuleHandleA(module), "CreateInterface")
#elif __linux__
            dlsym(dlopen(module, RTLD_NOLOAD | RTLD_NOW), "CreateInterface")
#endif
            )) {
            if (void* foundInterface = createInterface(name, nullptr))
                return foundInterface;
        }

#ifdef _WIN32
        MessageBoxA(nullptr, ("Failed to find " + std::string{ name } + " interface!").c_str(), "GOESP", MB_OK | MB_ICONERROR);
#endif
        std::exit(EXIT_FAILURE);
    }
};

inline std::unique_ptr<const Interfaces> interfaces;
