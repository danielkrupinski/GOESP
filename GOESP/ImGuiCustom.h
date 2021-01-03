#pragma once

#include <array>

struct Color;
struct ColorToggle;
struct ColorToggleRounding;
struct ColorToggleThickness;
struct ColorToggleThicknessRounding;

namespace ImGuiCustom
{
    void colorPopup(const char* name, std::array<float, 4>& color, bool* rainbow = nullptr, float* rainbowSpeed = nullptr, bool* enable = nullptr, float* thickness = nullptr, float* rounding = nullptr) noexcept;
    void colorPicker(const char* name, Color& colorConfig, bool* enable = nullptr, float* thickness = nullptr) noexcept;
    void colorPicker(const char* name, ColorToggle& colorConfig) noexcept;
    void colorPicker(const char* name, ColorToggleRounding& colorConfig) noexcept;
    void colorPicker(const char* name, ColorToggleThickness& colorConfig) noexcept;
    void colorPicker(const char* name, ColorToggleThicknessRounding& colorConfig) noexcept;
}

namespace ImGui
{
    bool smallButtonFullWidth(const char* label, bool disabled = false) noexcept;

    // table that inherits ImGuiWindowFlags_NoInputs from parent window for its scrolling region
    bool beginTable(const char* str_id, int columns_count, ImGuiTableFlags flags = 0, const ImVec2& outer_size = ImVec2(0, 0), float inner_width = 0.0f) noexcept;
    bool beginTableEx(const char* name, ImGuiID id, int columns_count, ImGuiTableFlags flags = 0, const ImVec2& outer_size = ImVec2(0, 0), float inner_width = 0.0f) noexcept;
    void textEllipsisInTableCell(const char* text) noexcept;
    void TableSetColumnIsEnabled(int column_n, bool hidden);
}
