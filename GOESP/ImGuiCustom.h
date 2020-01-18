#pragma once

#include "Config.h"

#include <functional>

namespace ImGuiCustom
{
    void colorPopup(const char* name, std::array<float, 4>& color, bool* enable = nullptr, std::function<void()> popupFn = nullptr) noexcept;
    void colorPicker(const char* name, Config::ColorToggle& colorConfig, std::function<void()> pickerFn = nullptr) noexcept;
    void colorPicker(const char* name, Config::ColorToggleRounding& colorConfig, std::function<void()> pickerFn = nullptr) noexcept;
    void colorPicker(const char* name, Config::ColorToggleThickness& colorConfig, std::function<void()> pickerFn = nullptr) noexcept;
    void colorPicker(const char* name, Config::ColorToggleThicknessRounding& colorConfig, std::function<void()> pickerFn = nullptr) noexcept;
}
