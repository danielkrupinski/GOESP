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

    bool open = true;

private:
    void loadConfig() const noexcept;
    void saveConfig() const noexcept;

    inline constexpr float animationLength() { return 0.5f; }
    float toggleAnimationEnd = 0.0f;
    std::filesystem::path path;
    ImFont* unicodeFont;
};

inline std::unique_ptr<GUI> gui;
