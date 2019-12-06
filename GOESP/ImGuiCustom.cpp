#include "imguiCustom.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

void ImGuiCustom::colorPicker(const char* name, std::array<float, 4>& color, bool* enable, bool* rainbow, float* rainbowSpeed, float* rounding, float* thickness) noexcept
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

        if (rainbow && rainbowSpeed) {
            ImGui::SameLine();

            if (ImGui::BeginChild("##child", { 150.0f, 0.0f })) {
                ImGui::Checkbox("Rainbow", rainbow);
                ImGui::PushItemWidth(85.0f);
                ImGui::InputFloat("Speed", rainbowSpeed, 0.01f, 0.15f, "%.2f");

                if (rounding || thickness)
                    ImGui::Separator();

                if (rounding) {
                    ImGui::InputFloat("Rounding", rounding, 0.1f, 0.0f, "%.1f");
                    *rounding = std::max(*rounding, 0.0f);
                }
                if (thickness) {
                    ImGui::InputFloat("Thickness", thickness, 0.1f, 0.0f, "%.1f");
                    *thickness = std::max(*thickness, 0.0f);
                }
                ImGui::PopItemWidth();
                ImGui::EndChild();
            }
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
}

void ImGuiCustom::colorPicker(const char* name, Config::ColorToggle& colorConfig) noexcept
{
    colorPicker(name, colorConfig.color, &colorConfig.enabled, &colorConfig.rainbow, &colorConfig.rainbowSpeed);
}

void ImGuiCustom::colorPicker(const char* name, Config::ColorToggleRounding& colorConfig) noexcept
{
    colorPicker(name, colorConfig.color, &colorConfig.enabled, &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.rounding);
}

void ImGuiCustom::colorPicker(const char* name, Config::ColorToggleThickness& colorConfig) noexcept
{
    colorPicker(name, colorConfig.color, &colorConfig.enabled, &colorConfig.rainbow, &colorConfig.rainbowSpeed, nullptr, &colorConfig.thickness);
}

void ImGuiCustom::colorPicker(const char* name, Config::ColorToggleThicknessRounding& colorConfig) noexcept
{
    colorPicker(name, colorConfig.color, &colorConfig.enabled, &colorConfig.rainbow, &colorConfig.rainbowSpeed, &colorConfig.rounding, &colorConfig.thickness);
}
