#pragma once

#include <d3d9.h>
#include <type_traits>

template <typename T>
struct Hook {
    T* original;
    bool hookCalled{ false };
};

class Hooks {
public:
    Hooks() noexcept;

    Hook<HRESULT __stdcall(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)> present;
    Hook<HRESULT __stdcall(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)> reset;
};

extern Hooks hooks;
