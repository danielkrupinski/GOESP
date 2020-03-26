#pragma once

#include <memory>
#include <Windows.h>

class GUI {
public:
    GUI(HWND window) noexcept;
    void render() noexcept;

    bool open = true;
};

inline std::unique_ptr<GUI> gui;
