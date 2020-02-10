#pragma once

#include "ConfigStructs.h"

#include <functional>

namespace ImGuiCustom
{
    void colorPopup(const char* name, std::array<float, 4>& color, bool* enable = nullptr, std::function<void()> popupFn = nullptr) noexcept;
    void colorPicker(const char* name, ColorToggle& colorConfig, std::function<void()> pickerFn = nullptr) noexcept;
    void colorPicker(const char* name, ColorToggleRounding& colorConfig, std::function<void()> pickerFn = nullptr) noexcept;
    void colorPicker(const char* name, ColorToggleThickness& colorConfig, std::function<void()> pickerFn = nullptr) noexcept;
    void colorPicker(const char* name, ColorToggleThicknessRounding& colorConfig, std::function<void()> pickerFn = nullptr) noexcept;
}
