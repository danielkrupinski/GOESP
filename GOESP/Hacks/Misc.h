#pragma once

class GameEvent;
struct ImDrawList;

namespace Misc
{
    void collectData() noexcept;
    void drawReloadProgress(ImDrawList* drawList) noexcept;
    void drawRecoilCrosshair(ImDrawList* drawList) noexcept;
    void purchaseList(GameEvent* event = nullptr) noexcept;
    void drawBombZoneHint() noexcept;
}
