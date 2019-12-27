#pragma once

#include <d3d9.h>
#include <memory>
#include <type_traits>
#include <Windows.h>

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
        return unload && !(present.hookCalled || reset.hookCalled || wndProc.hookCalled || setCursorPos.hookCalled);
    }

    Hook<HRESULT __stdcall(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*)> present;
    Hook<HRESULT __stdcall(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*)> reset;
    Hook<std::remove_pointer_t<WNDPROC>> wndProc;
    Hook<BOOL WINAPI(int, int)> setCursorPos;

    void restore() noexcept;

private:
    bool unload = false;
};

inline std::unique_ptr<Hooks> hooks;
