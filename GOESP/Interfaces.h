#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <Windows.h>

class Interfaces {
public:
#define GAME_INTERFACE(type, name, module, version) \
class type* name = reinterpret_cast<type*>(find(L##module, version));

    GAME_INTERFACE(Client, client, "client", "VClient018")
    GAME_INTERFACE(ClientTools, clientTools, "client", "VCLIENTTOOLS001")
    GAME_INTERFACE(Cvar, cvar, "vstdlib", "VEngineCvar007")
    GAME_INTERFACE(Engine, engine, "engine", "VEngineClient014")
    GAME_INTERFACE(EngineTrace, engineTrace, "engine", "EngineTraceClient004")
    GAME_INTERFACE(EntityList, entityList, "client", "VClientEntityList003")
    GAME_INTERFACE(GameEventManager, gameEventManager, "engine", "GAMEEVENTSMANAGER002")
    GAME_INTERFACE(InputSystem, inputSystem, "inputsystem", "InputSystemVersion001")
    GAME_INTERFACE(Localize, localize, "localize", "Localize_001")
    GAME_INTERFACE(ModelInfo, modelInfo, "engine", "VModelInfoClient004")

#undef GAME_INTERFACE
private:
    static void* find(const wchar_t* module, const char* name) noexcept
    {
        if (const auto createInterface = reinterpret_cast<std::add_pointer_t<void* __cdecl (const char* name, int* returnCode)>>(GetProcAddress(GetModuleHandleW(module), "CreateInterface")))
            if (void* foundInterface = createInterface(name, nullptr))
                return foundInterface;

        MessageBoxA(nullptr, ("Failed to find " + std::string{ name } + " interface!").c_str(), "GOESP", MB_OK | MB_ICONERROR);
        std::exit(EXIT_FAILURE);
    }
};

inline std::unique_ptr<const Interfaces> interfaces;
