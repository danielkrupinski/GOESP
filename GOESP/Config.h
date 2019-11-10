#pragma once

#include <array>
#include <filesystem>

class Config {
public:
    explicit Config(const char* folderName) noexcept;

    struct Color {
        float color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
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

    struct Weapon : public Shared {

    };

    std::array<Player, 6> players;
    Weapon weapons;
    std::array<Weapon, 11> pistols;
    std::array<Weapon, 8> smgs;
    std::array<Weapon, 8> rifles;
    std::array<Weapon, 5> sniperRifles;
    std::array<Weapon, 7> shotguns;
    std::array<Weapon, 3> heavy;

    std::array<Weapon, 3> misc;
private:
    std::filesystem::path path;
};

extern Config config;
