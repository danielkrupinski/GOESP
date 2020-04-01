#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "ConfigStructs.h"

struct ImFont;

class Config {
public:
    explicit Config(const char* folderName) noexcept;
    void load() noexcept;
    void save() noexcept;

    // Legacy
    /*
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
    */

    //
    std::unordered_map<std::string, Player> allies;
    std::unordered_map<std::string, Player> enemies;
    std::unordered_map<std::string, Weapon> _weapons;
    std::unordered_map<std::string, Projectile> _projectiles;
    std::unordered_map<std::string, Shared> _otherEntities;
    //

    ColorToggleThickness reloadProgress{ 5.0f };
    ColorToggleThickness recoilCrosshair;
    bool normalizePlayerNames = true;

    PurchaseList purchaseList;

    std::vector<std::string> systemFonts{ "Default" };
    std::unordered_map<std::string, ImFont*> fonts;

    void scheduleFontLoad(const std::string& name, int size = 15) noexcept;
    bool loadScheduledFonts() noexcept;

private:
    std::vector<std::pair<std::string, int>> scheduledFonts;
    std::filesystem::path path;
};

inline std::unique_ptr<Config> config;
