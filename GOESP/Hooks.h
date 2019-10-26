#pragma once

#include <d3d9.h>
#include <type_traits>

class Hooks {
public:
    Hooks() noexcept;

    std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)> originalPresent;
    std::add_pointer_t<HRESULT __stdcall(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)> originalReset;
};

extern Hooks hooks;
