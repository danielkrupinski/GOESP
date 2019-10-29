#pragma once

#include <array>
#include <filesystem>

class Config {
public:
    explicit Config(const char* folderName) noexcept;

    struct Color {
        float color[3]{ 1.0f, 1.0f, 1.0f };
        bool rainbow = false;
        float rainbowSpeed = 0.6f;
    };

    struct ColorToggle : public Color {
        bool enabled = false;
    };

    struct Shared {
        bool enabled = false;
        ColorToggle snaplines;
        ColorToggle box;
        int boxType = 0;
    };

    struct Player : public Shared {

    };

    std::array<Player, 6> players;

private:
    std::filesystem::path path;
};

extern Config config;
