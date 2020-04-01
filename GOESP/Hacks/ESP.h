#pragma once

struct ImDrawList;

namespace ESP
{
    void collectData() noexcept;
    void render(ImDrawList* drawList) noexcept;
    void clearProjectileList() noexcept;
}
