#pragma once

#include <memory>
#include <type_traits>

#ifdef _WIN32
#include <d3d9.h>
#include <Windows.h>
#elif __linux__
struct SDL_Window;
union SDL_Event;
#endif

class Hooks {
public:
#ifdef _WIN32
    Hooks(HMODULE module) noexcept;

    std::add_pointer_t<HRESULT D3DAPI(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)> reset;
    std::add_pointer_t<HRESULT D3DAPI(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)> present;
    std::add_pointer_t<BOOL WINAPI(int, int)> setCursorPos;

    WNDPROC wndProc;
#elif __linux__
    Hooks() noexcept;

    std::add_pointer_t<int(SDL_Event*)> pollEvent;
    std::add_pointer_t<void(SDL_Window*)> swapWindow;
    std::add_pointer_t<void(SDL_Window*, int x, int y)> warpMouseInWindow;
#endif

    void setup() noexcept;
    void install() noexcept;
    void uninstall() noexcept;

    enum class State {
        NotInstalled,
        Installing,
        Installed
    };

    constexpr auto getState() noexcept { return state; }
private:
#ifdef _WIN32
    HMODULE module;
    HWND window;
#endif
    State state = State::NotInstalled;
};

inline std::unique_ptr<Hooks> hooks;
