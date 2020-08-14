#pragma once

#include <memory>

class GUI {
public:
    GUI() noexcept;
    void render() noexcept;

    bool open = true;
private:
    void drawESPTab() noexcept;
};

inline std::unique_ptr<GUI> gui;
