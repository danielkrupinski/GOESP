#pragma once

#include <array>

struct Color {
    std::array<float, 4> color{ 1.0f, 1.0f, 1.0f, 1.0f };
    bool rainbow = false;
    float rainbowSpeed = 0.6f;

    auto operator==(const Color& c) const
    {
        return color == c.color
            && rainbow == c.rainbow
            && rainbowSpeed == c.rainbowSpeed;
    }
};

struct ColorToggle : Color {
    bool enabled = false;

    auto operator==(const ColorToggle& ct) const
    {
        return static_cast<Color>(*this) == static_cast<Color>(ct)
            && enabled == ct.enabled;
    }
};

struct ColorToggleThickness : ColorToggle {
    ColorToggleThickness() = default;
    ColorToggleThickness(float thickness) : thickness{ thickness } { }
    float thickness = 1.0f;

    auto operator==(const ColorToggleThickness& ctt) const
    {
        return static_cast<ColorToggle>(*this) == static_cast<ColorToggle>(ctt)
            && thickness == ctt.thickness;
    }
};

struct ColorToggleRounding : ColorToggle {
    float rounding = 5.0f;

    auto operator==(const ColorToggleRounding& ctr) const
    {
        return static_cast<ColorToggle>(*this) == static_cast<ColorToggle>(ctr)
            && rounding == ctr.rounding;
    }
};

struct ColorToggleThicknessRounding : ColorToggleRounding {
    float thickness = 1.0f;

    auto operator==(const ColorToggleThicknessRounding& cttr) const
    {
        return static_cast<ColorToggleRounding>(*this) == static_cast<ColorToggleRounding>(cttr)
            && thickness == cttr.thickness;
    }
};

struct Font {
    int index = 0; // do not save
    std::string name;

    auto operator==(const Font& f) const
    {
        return index == f.index
            && name == f.name;
    }
};

struct Shared {
    bool enabled = false;
    Font font;
    ColorToggleThickness snaplines;
    int snaplineType = 0;
    ColorToggleThicknessRounding box;
    int boxType = 0;
    std::array<float, 3> boxScale{ 0.25f, 0.25f, 0.25f };
    ColorToggle name;
    ColorToggleRounding textBackground{ 0.0f, 0.0f, 0.0f, 1.0f };
    float textCullDistance = 0.0f;

    auto operator==(const Shared& s) const
    {
        return enabled == s.enabled
            && font == s.font
            && snaplines == s.snaplines
            && snaplineType == s.snaplineType
            && box == s.box
            && boxType == s.boxType
            && boxScale == s.boxScale
            && name == s.name
            && textBackground == s.textBackground
            && textCullDistance == s.textCullDistance;
    }
};

struct Player : Shared {
    ColorToggle weapon;
    ColorToggle flashDuration;
    bool audibleOnly = false;

    auto operator==(const Player& p) const
    {
        return static_cast<Shared>(*this) == static_cast<Shared>(p)
            && weapon == p.weapon
            && flashDuration == p.flashDuration
            && audibleOnly == p.audibleOnly;
    }

    auto& operator=(const Shared& s)
    {
        static_cast<Shared&>(*this) = s;
        return *this;
    }
};

struct Weapon : Shared {
    ColorToggle ammo;

    auto operator==(const Weapon& w) const
    {
        return static_cast<Shared>(*this) == static_cast<Shared>(w)
            && ammo == w.ammo;
    }

    auto& operator=(const Shared& s)
    {
        static_cast<Shared&>(*this) = s;
        return *this;
    }
};

struct Trail {
    bool enabled = false;
    float localPlayerTime = 2.0f;
    float alliesTime = 2.0f;
    float enemiesTime = 2.0f;
    ColorToggleThickness localPlayer;
    ColorToggleThickness allies;
    ColorToggleThickness enemies;

    auto operator==(const Trail& t) const
    {
        return enabled == t.enabled
            && localPlayerTime == t.localPlayerTime
            && alliesTime == t.alliesTime
            && enemiesTime == t.enemiesTime
            && localPlayer == t.localPlayer
            && allies == t.allies
            && enemies == t.enemies;
    }
};

struct Projectile : Shared {
    Trail trail;
  
    auto operator==(const Projectile& p) const
    {
        return static_cast<Shared>(*this) == static_cast<Shared>(p)
            && trail == p.trail;
    }

    auto& operator=(const Shared& s)
    {
        static_cast<Shared&>(*this) = s;
        return *this;
    }
};

struct PurchaseList {
    bool enabled = false;
    bool onlyDuringFreezeTime = false;
    bool showPrices = false;
    bool noTitleBar = false;

    enum Mode {
        Details = 0,
        Summary
    };
    int mode = Details;

    auto operator==(const PurchaseList& pl) const
    {
        return enabled == pl.enabled
            && onlyDuringFreezeTime == pl.onlyDuringFreezeTime
            && showPrices == pl.showPrices
            && noTitleBar == pl.noTitleBar
            && mode == pl.mode;
    }
};
