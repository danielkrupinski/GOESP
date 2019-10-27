#pragma once

class GUI {
public:
    GUI() noexcept;
    void render() noexcept;

    bool blockInput = false;
};

extern GUI gui;
