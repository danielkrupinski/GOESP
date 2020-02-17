#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <Windows.h>

class Client;
class Cvar;
class Engine;
class EngineTrace;
class EntityList;
class GameEventManager;
class InputSystem;
class Localize;

class Interfaces {
public:
    Client* client = find<Client>(L"client_panorama", "VClient018");
    Cvar* cvar = find<Cvar>(L"vstdlib", "VEngineCvar007");
    Engine* engine = find<Engine>(L"engine", "VEngineClient014");
    EngineTrace* engineTrace = find<EngineTrace>(L"engine", "EngineTraceClient004");
    EntityList* entityList = find<EntityList>(L"client_panorama", "VClientEntityList003");
    GameEventManager* gameEventManager = find<GameEventManager>(L"engine", "GAMEEVENTSMANAGER002");
    InputSystem* inputSystem = find<InputSystem>(L"inputsystem", "InputSystemVersion001");
    Localize* localize = find<Localize>(L"localize", "Localize_001");
private:
    template <typename T>
    static auto find(const wchar_t* module, const char* name) noexcept
    {
        if (const auto createInterface = reinterpret_cast<std::add_pointer_t<T* __cdecl (const char* name, int* returnCode)>>(GetProcAddress(GetModuleHandleW(module), "CreateInterface")))
            if (T* foundInterface = createInterface(name, nullptr))
                return foundInterface;

        MessageBoxA(nullptr, ("Failed to find " + std::string{ name } + " interface!").c_str(), "GOESP", MB_OK | MB_ICONERROR);
        std::exit(EXIT_FAILURE);
    }
};

inline std::unique_ptr<const Interfaces> interfaces;
