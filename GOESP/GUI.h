#pragma once

#include <memory>

class GUI {
public:
    GUI() noexcept;
    void render() noexcept;

    bool blockInput = true;
};

inline std::unique_ptr<GUI> gui;
