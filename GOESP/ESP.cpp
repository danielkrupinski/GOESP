#include "ESP.h"

#include "imgui/imgui.h"

void ESP::render(ImDrawList* drawList) noexcept
{
    drawList->AddCircle({ 200, 200 }, 15.0f, 0xFFFF0000);
}
