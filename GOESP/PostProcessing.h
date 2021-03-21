#pragma once

struct ImDrawList;

#ifdef _WIN32
struct IDirect3DDevice9;
#endif

namespace PostProcessing
{
#ifdef _WIN32
    void performFullscreenBlur(ImDrawList* drawList, float alpha, IDirect3DDevice9* device) noexcept;
    void clearBlurTextures() noexcept;
#else
    void performFullscreenBlur(ImDrawList* drawList, float alpha) noexcept;
#endif
}
