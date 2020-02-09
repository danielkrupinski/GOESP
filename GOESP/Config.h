#pragma once

#include <array>
#include <filesystem>
#include <memory>
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

    std::array<Player, 6> players;
    Weapon weapons;
    std::array<Weapon, 11> pistols;
    std::array<Weapon, 8> smgs;
    std::array<Weapon, 8> rifles;
    std::array<Weapon, 5> sniperRifles;
    std::array<Weapon, 7> shotguns;
    std::array<Weapon, 3> machineguns;
    std::array<Weapon, 12> grenades;
    std::array<Shared, 10> projectiles;
    std::array<Shared, 4> otherEntities;

    ColorToggleThickness reloadProgress{ 5.0f };
    ColorToggleThickness recoilCrosshair;
    bool normalizePlayerNames = true;

    std::vector<std::pair<std::string, std::string>> systemFonts{ { "Default", "" } };
    std::unordered_map<std::string, ImFont*> fonts;

    void scheduleFontLoad(const std::string& name) noexcept;
    bool loadScheduledFonts() noexcept;

private:
    std::filesystem::path fontsPath;
    std::vector<std::string> scheduledFonts;
    std::filesystem::path path;
};

inline std::unique_ptr<Config> config;
