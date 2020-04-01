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

    std::unordered_map<std::string, Player> allies;
    std::unordered_map<std::string, Player> enemies;
    std::unordered_map<std::string, Weapon> _weapons;
    std::unordered_map<std::string, Projectile> _projectiles;
    std::unordered_map<std::string, Shared> _otherEntities;

    ColorToggleThickness reloadProgress{ 5.0f };
    ColorToggleThickness recoilCrosshair;
    bool normalizePlayerNames = true;
    bool bombZoneHint = false;
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
