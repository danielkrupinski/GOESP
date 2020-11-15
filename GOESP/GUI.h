#pragma once

#include <filesystem>
#include <memory>

struct ImFont;

class GUI {
public:
    GUI() noexcept;
    void render() noexcept;
    ImFont* getUnicodeFont() const noexcept;

    bool open = true;

private:
    void loadConfig() const noexcept;
    void saveConfig() const noexcept;

    std::filesystem::path path;
    ImFont* unicodeFont;
};

inline std::unique_ptr<GUI> gui;
