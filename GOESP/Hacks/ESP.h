#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../ConfigStructs.h"

namespace ESP
{
    void render() noexcept;
    void drawGUI() noexcept;
    json toJSON() noexcept;
    void fromJSON(const json& j) noexcept;
    void scheduleFontLoad(std::size_t index) noexcept;
    bool loadScheduledFonts() noexcept;
    const std::vector<std::string>& getSystemFonts() noexcept;
}
