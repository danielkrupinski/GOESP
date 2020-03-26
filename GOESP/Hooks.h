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
    void restore() noexcept;

private:
    HMODULE module;
    HWND window;
};

inline std::unique_ptr<Hooks> hooks;
