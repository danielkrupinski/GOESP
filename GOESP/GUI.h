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
private:
    void loadConfig() const noexcept;
    void saveConfig() const noexcept;

    inline constexpr float animationLength() { return 0.5f; }
    float toggleAnimationEnd = 0.0f;
    bool open = true;
    ImFont* unicodeFont;
    std::filesystem::path path;
};

inline std::unique_ptr<GUI> gui;
