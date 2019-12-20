#pragma once

#include <memory>

class GUI {
public:
    GUI() noexcept;
    void render() noexcept;

    bool blockInput = false;
};

extern std::unique_ptr<GUI> gui;
