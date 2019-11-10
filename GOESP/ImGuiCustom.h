#pragma once

#include "Config.h"

namespace ImGuiCustom {
    void colorPicker(const char* name, std::array<float, 4>& color, bool* enable = nullptr, bool* rainbow = nullptr, float* rainbowSpeed = nullptr) noexcept;
    void colorPicker(const char* name, Config::ColorToggle& colorConfig) noexcept;
}
