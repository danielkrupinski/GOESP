#pragma once

#include <filesystem>
#include <memory>

struct ImFont;

class GUI {
public:
    GUI() noexcept;
    void render() noexcept;
    ImFont* getUnicodeFont() const noexcept;
    void handleToggle() noexcept;
    bool isOpen() const noexcept { return open; }
    bool isFullyClosed() const noexcept { return !open && toggleAnimationEnd > 1.0f; }
    float getTransparency() const noexcept { return std::clamp(open ? toggleAnimationEnd : 1.0f - toggleAnimationEnd, 0.0f, 1.0f); }
private:
    void loadConfig() const noexcept;
    void saveConfig() const noexcept;
    void createConfigDir() const noexcept;

    inline constexpr float animationLength() { return 0.35f; }
    float toggleAnimationEnd = 0.0f;
    bool open = true;
    ImFont* unicodeFont;
    std::filesystem::path path;
};

inline std::unique_ptr<GUI> gui;
