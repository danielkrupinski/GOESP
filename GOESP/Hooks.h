#pragma once

#include <d3d9.h>
#include <memory>
#include <type_traits>

template <typename T>
struct Hook {
    T* original;
    bool hookCalled = false;
};

class Hooks {
public:
    Hooks() noexcept;

    constexpr auto readyForUnload() noexcept
    {
        return unload && !(present.hookCalled || reset.hookCalled || wndProc.hookCalled);
    }

    Hook<HRESULT __stdcall(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)> present;
    Hook<HRESULT __stdcall(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)> reset;
    Hook<std::remove_pointer_t<WNDPROC>> wndProc;

    void restore() noexcept;

private:
    bool unload = false;
};

inline std::unique_ptr<Hooks> hooks;
