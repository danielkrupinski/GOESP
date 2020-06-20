#pragma once

#include <d3d9.h>
#include <memory>
#include <type_traits>
#include <Windows.h>

class Hooks {
public:
    Hooks(HMODULE module) noexcept;
    
    std::add_pointer_t<HRESULT D3DAPI(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)> reset;
    std::add_pointer_t<HRESULT D3DAPI(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)> present;
    std::add_pointer_t<BOOL WINAPI(int, int)> setCursorPos;

    WNDPROC wndProc;

    void install() noexcept;
    void uninstall() noexcept;

    enum class State {
        NotInstalled,
        Installing,
        Installed
    };

    constexpr auto getState() noexcept { return state; }
private:
    HMODULE module;
    HWND window;
    State state = State::NotInstalled;
};

inline std::unique_ptr<Hooks> hooks;
