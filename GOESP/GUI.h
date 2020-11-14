#pragma once

#include <filesystem>
#include <memory>

class GUI {
public:
    GUI() noexcept;
    void render() noexcept;

    bool open = true;

private:
    void loadConfig() const noexcept;

    std::filesystem::path path;
};

inline std::unique_ptr<GUI> gui;
