#pragma once

struct ImDrawList;

namespace Misc
{
    void collectData() noexcept;
    void drawReloadProgress(ImDrawList* drawList) noexcept;
    void drawRecoilCrosshair(ImDrawList* drawList) noexcept;
}
