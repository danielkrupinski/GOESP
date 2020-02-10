#pragma once

#include <array>

struct Color {
    std::array<float, 4> color{ 1.0f, 1.0f, 1.0f, 1.0f };
    bool rainbow = false;
    float rainbowSpeed = 0.6f;
};

struct ColorToggle : Color {
    bool enabled = false;
};

struct ColorToggleThickness : ColorToggle {
    ColorToggleThickness() = default;
    ColorToggleThickness(float thickness) : thickness{ thickness } { }
    float thickness = 1.0f;
};

struct ColorToggleRounding : ColorToggle {
    float rounding = 5.0f;
};

struct ColorToggleThicknessRounding : ColorToggleRounding {
    float thickness = 1.0f;
};

struct Shared {
    bool enabled = false;
    std::string font;
    int fontIndex = 0; // runtime only, don't save
    ColorToggleThickness snaplines;
    int snaplineType = 0;
    ColorToggleThicknessRounding box;
    int boxType = 0;
    ColorToggle name;
    ColorToggleRounding textBackground{ 0.0f, 0.0f, 0.0f, 1.0f };
    float textCullDistance = 0.0f;
};

struct Player : Shared {
    ColorToggle weapon;
    ColorToggle flashDuration;
};

struct Weapon : Shared {
    ColorToggle ammo;
};
