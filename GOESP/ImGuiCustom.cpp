#include "imguiCustom.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

void ImGuiCustom::colorPopup(const char* name, std::array<float, 4>& color, bool* enable, std::function<void()> popupFn) noexcept
{
    ImGui::PushID(name);
    if (enable) {
        ImGui::Checkbox("##check", enable);
        ImGui::SameLine(0.0f, 5.0f);
    }
    bool openPopup = ImGui::ColorButton("##btn", ImColor{ color[0], color[1], color[2] }, ImGuiColorEditFlags_NoTooltip);
    ImGui::SameLine(0.0f, 5.0f);
    ImGui::TextUnformatted(name);

    if (openPopup)
        ImGui::OpenPopup("##popup");

    if (ImGui::BeginPopup("##popup")) {
        ImGui::ColorPicker4("##picker", color.data(), ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaPreview);
        if (popupFn)
            popupFn();
        ImGui::EndPopup();
    }
    ImGui::PopID();
}

void ImGuiCustom::colorPicker(const char* name, ColorToggle& colorConfig, std::function<void()> pickerFn) noexcept
{
    colorPopup(name, colorConfig.color, &colorConfig.enabled, [&] {
        ImGui::SameLine();
        if (ImGui::BeginChild("##child", { 150.0f, 0.0f })) {
            ImGui::Checkbox("Rainbow", &colorConfig.rainbow);
            ImGui::PushItemWidth(85.0f);
            ImGui::InputFloat("Speed", &colorConfig.rainbowSpeed, 0.01f, 0.15f, "%.2f");
            if (pickerFn)
                pickerFn();
            ImGui::PopItemWidth();
            ImGui::EndChild();
        }
    });
}

void ImGuiCustom::colorPicker(const char* name, ColorToggleRounding& colorConfig, std::function<void()> pickerFn) noexcept
{
    colorPicker(name, static_cast<ColorToggle&>(colorConfig), [&] {
        ImGui::Separator();
        ImGui::InputFloat("Rounding", &colorConfig.rounding, 0.1f, 0.0f, "%.1f");
        colorConfig.rounding = std::max(colorConfig.rounding, 0.0f);
        if (pickerFn)
            pickerFn();
    });
}

void ImGuiCustom::colorPicker(const char* name, ColorToggleThickness& colorConfig, std::function<void()> pickerFn) noexcept
{
    colorPicker(name, static_cast<ColorToggle&>(colorConfig), [&] {
        ImGui::Separator();
        ImGui::InputFloat("Thickness", &colorConfig.thickness, 0.1f, 0.0f, "%.1f");
        colorConfig.thickness = std::max(colorConfig.thickness, 0.0f);
        if (pickerFn)
            pickerFn();
    });
}

void ImGuiCustom::colorPicker(const char* name, ColorToggleThicknessRounding& colorConfig, std::function<void()> pickerFn) noexcept
{
    colorPicker(name, static_cast<ColorToggleRounding&>(colorConfig), [&] {
        ImGui::InputFloat("Thickness", &colorConfig.thickness, 0.1f, 0.0f, "%.1f");
        colorConfig.thickness = std::max(colorConfig.thickness, 0.0f);
        if (pickerFn)
            pickerFn();
    });
}
