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
    std::unordered_map<std::string, Weapon> weapons;
    std::unordered_map<std::string, Projectile> projectiles;
    std::unordered_map<std::string, Shared> lootCrates;
    std::unordered_map<std::string, Shared> otherEntities;

    ColorToggleThickness reloadProgress{ 5.0f };
    ColorToggleThickness recoilCrosshair;
    ColorToggleThickness noscopeCrosshair;
    PurchaseList purchaseList;
    ObserverList observerList;
    bool ignoreFlashbang = false;
    OverlayWindow fpsCounter{ "FPS Counter" };

    std::vector<std::string> systemFonts{ "Default" };
#ifdef __linux__
    std::vector<std::string> systemFontPaths{ "" };
#endif

    struct Font {
        ImFont* tiny;
        ImFont* medium;
        ImFont* big;
    };

    std::unordered_map<std::string, Font> fonts;

    void scheduleFontLoad(std::size_t index) noexcept;
    bool loadScheduledFonts() noexcept;

private:
    std::vector<std::size_t> scheduledFonts{ 0 };
    std::filesystem::path path;
};

inline std::unique_ptr<Config> config;
