#pragma once

#include <array>
#include <string>

#include "imgui/imgui.h"

#pragma pack(push, 1)
struct Color {
    std::array<float, 4> color{ 1.0f, 1.0f, 1.0f, 1.0f };
    float rainbowSpeed = 0.6f;
    bool rainbow = false;

    auto operator==(const Color& c) const
    {
        return color == c.color
            && rainbowSpeed == c.rainbowSpeed
            && rainbow == c.rainbow;
    }
};
#pragma pack(pop)

struct ColorToggle : Color {
    bool enabled = false;

    auto operator==(const ColorToggle& ct) const
    {
        return static_cast<const Color&>(*this) == static_cast<const Color&>(ct)
            && enabled == ct.enabled;
    }
};

struct ColorToggleThickness : ColorToggle {
    ColorToggleThickness() = default;
    explicit ColorToggleThickness(float thickness) : thickness{ thickness } { }
    float thickness = 1.0f;

    auto operator==(const ColorToggleThickness& ctt) const
    {
        return static_cast<const ColorToggle&>(*this) == static_cast<const ColorToggle&>(ctt)
            && thickness == ctt.thickness;
    }
};

struct ColorToggleRounding : ColorToggle {
    float rounding = 0.0f;

    auto operator==(const ColorToggleRounding& ctr) const
    {
        return static_cast<const ColorToggle&>(*this) == static_cast<const ColorToggle&>(ctr)
            && rounding == ctr.rounding;
    }
};

struct ColorToggleThicknessRounding : ColorToggleRounding {
    float thickness = 1.0f;

    auto operator==(const ColorToggleThicknessRounding& cttr) const
    {
        return static_cast<const ColorToggleRounding&>(*this) == static_cast<const ColorToggleRounding&>(cttr)
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

struct Snapline : ColorToggleThickness {
    enum Type {
        Bottom = 0,
        Top,
        Crosshair
    };

    int type = Bottom;

    auto operator==(const Snapline& s) const
    {
        return static_cast<const ColorToggleThickness&>(*this) == static_cast<const ColorToggleThickness&>(s)
            && type == s.type;
    }
};

struct Box : ColorToggleThicknessRounding {
    enum Type {
        _2d = 0,
        _2dCorners,
        _3d,
        _3dCorners
    };

    int type = _2d;
    std::array<float, 3> scale{ 0.25f, 0.25f, 0.25f };

    auto operator==(const Box& b) const
    {
        return static_cast<const ColorToggleThicknessRounding&>(*this) == static_cast<const ColorToggleThicknessRounding&>(b)
            && type == b.type
            && scale == b.scale;
    }
};

struct Shared {
    bool enabled = false;
    Font font;
    Snapline snapline;
    Box box;
    ColorToggle name;
    float textCullDistance = 0.0f;
};

struct Bar : ColorToggleRounding {

};

struct Player : Shared {
    Player() : Shared{}
    {
        box.type = Box::_2dCorners;
    }

    ColorToggle weapon;
    ColorToggle flashDuration;
    bool audibleOnly = false;
    bool spottedOnly = false;
    ColorToggleThickness skeleton;

    auto& operator=(const Shared& s)
    {
        static_cast<Shared&>(*this) = s;
        return *this;
    }
};

struct Weapon : Shared {
    ColorToggle ammo;

    auto& operator=(const Shared& s)
    {
        static_cast<Shared&>(*this) = s;
        return *this;
    }
};

struct Trail : ColorToggleThickness {
    enum Type {
        Line = 0,
        Circles,
        FilledCircles
    };

    int type = Line;
    float time = 2.0f;
};

struct Trails {
    bool enabled = false;

    Trail localPlayer;
    Trail allies;
    Trail enemies;
};

struct Projectile : Shared {
    Trails trails;

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

    ImVec2 pos;
    ImVec2 size{ 200.0f, 200.0f };
};

struct ObserverList {
    bool enabled = false;
    bool noTitleBar = false;
    ImVec2 pos;
    ImVec2 size{ 200.0f, 200.0f };
};
