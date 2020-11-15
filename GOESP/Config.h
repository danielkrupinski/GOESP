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
    Config() noexcept;

    struct Font {
        ImFont* tiny;
        ImFont* medium;
        ImFont* big;
    };

    void scheduleFontLoad(std::size_t index) noexcept;
    bool loadScheduledFonts() noexcept;
    const auto& getSystemFonts() noexcept { return systemFonts; }
    const auto& getFonts() noexcept { return fonts; }
private:
    std::vector<std::size_t> scheduledFonts{ 0 };
    std::vector<std::string> systemFonts{ "Default" };
    std::unordered_map<std::string, Font> fonts;
#ifndef _WIN32
    std::vector<std::string> systemFontPaths{ "" };
#endif
};

inline std::unique_ptr<Config> config;
