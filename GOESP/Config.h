#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

struct ImFont;

class Config {
public:
    explicit Config(const char* folderName) noexcept;
    void load() noexcept;
    void save() noexcept;

    struct Color {
        std::array<float, 4> color{ 1.0f, 1.0f, 1.0f, 1.0f };
        bool rainbow = false;
        float rainbowSpeed = 0.6f;
    };

    struct ColorToggle : public Color {
        bool enabled = false;
    };

    struct Shared {
        bool enabled = false;
        std::string font;
        int fontIndex = 0; // don't save
        ColorToggle snaplines;
        ColorToggle box;
        int boxType = 0;
        ColorToggle name;
    };

    struct Player : public Shared {
    };

    struct Weapon : public Shared {
        ColorToggle ammo;
    };

    std::array<Player, 6> players;
    Weapon weapons;
    std::array<Weapon, 11> pistols;
    std::array<Weapon, 8> smgs;
    std::array<Weapon, 8> rifles;
    std::array<Weapon, 5> sniperRifles;
    std::array<Weapon, 7> shotguns;
    std::array<Weapon, 3> heavy;

    std::array<Shared, 4> misc;

    std::vector<std::pair<std::string, std::string>> systemFonts{ { "Default", "" } };
    std::unordered_map<std::string, ImFont*> fonts;

    void scheduleFontLoad(const std::string& name) noexcept;
    bool loadScheduledFonts() noexcept;

private:
    std::filesystem::path fontsPath;
    std::vector<std::string> scheduledFonts;
    std::filesystem::path path;
};

extern Config config;
