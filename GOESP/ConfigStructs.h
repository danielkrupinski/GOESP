#pragma once

#include <array>
#include <string>

#include "imgui/imgui.h"

#pragma pack(push, 1)
struct Color {
    std::array<float, 4> color{ 1.0f, 1.0f, 1.0f, 1.0f };
    float rainbowSpeed = 0.6f;
    bool rainbow = false;
};
#pragma pack(pop)

struct ColorToggle : Color {
    ColorToggle() = default;
    explicit ColorToggle(float r, float g, float b, float a) { color[0] = r; color[1] = g; color[2] = b; color[3] = a; }

    bool enabled = false;
};

struct ColorToggleThickness : ColorToggle {
    ColorToggleThickness() = default;
    explicit ColorToggleThickness(float thickness) : thickness{ thickness } { }

    float thickness = 1.0f;
};

struct ColorToggleRounding : ColorToggle {
    float rounding = 0.0f;
};

struct ColorToggleThicknessRounding : ColorToggleRounding {
    float thickness = 1.0f;
};

struct Font {
    int index = 0; // do not save
    std::string name = "Default";
};

struct Snapline : ColorToggleThickness {
    enum Type {
        Bottom = 0,
        Top,
        Crosshair
    };

    int type = Bottom;
};

struct Box : ColorToggleRounding {
    enum Type {
        _2d = 0,
        _2dCorners,
        _3d,
        _3dCorners
    };

    int type = _2d;
    std::array<float, 3> scale{ 0.25f, 0.25f, 0.25f };
    ColorToggle fill{ 1.0f, 1.0f, 1.0f, 0.4f };
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
    Box headBox;
    bool healthBar = false;

    using Shared::operator=;
};

struct Weapon : Shared {
    ColorToggle ammo;

    using Shared::operator=;
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

    using Shared::operator=;
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

struct OverlayWindow {
    OverlayWindow() = default;
    OverlayWindow(const char* windowName) : name{ windowName } {}
    bool enabled = false;
    const char* name = "";
    ImVec2 pos;
};

struct OffscreenEnemies {
    bool enabled = false;
};

#include "imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"
#include "nlohmann/json.hpp"

using json = nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int64_t, std::uint64_t, float>;
using value_t = json::value_t;

// WRITE macro requires:
// - json object named 'j'
// - object holding default values named 'dummy'
// - object to write to json named 'o'
#define WRITE(name, valueName) \
if (!(o.valueName == dummy.valueName)) \
    j[name] = o.valueName;

static void to_json(json& j, const Color& o, const Color& dummy = {})
{
    WRITE("Color", color)
    WRITE("Rainbow", rainbow)
    WRITE("Rainbow Speed", rainbowSpeed)
}

static void to_json(json& j, const ColorToggle& o, const ColorToggle& dummy = {})
{
    to_json(j, static_cast<const Color&>(o), dummy);
    WRITE("Enabled", enabled)
}

static void to_json(json& j, const ColorToggleRounding& o, const ColorToggleRounding& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Rounding", rounding)
}

static void to_json(json& j, const ColorToggleThickness& o, const ColorToggleThickness& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Thickness", thickness)
}

static void to_json(json& j, const ColorToggleThicknessRounding& o, const ColorToggleThicknessRounding& dummy = {})
{
    to_json(j, static_cast<const ColorToggleRounding&>(o), dummy);
    WRITE("Thickness", thickness)
}

static void to_json(json& j, const Font& o, const Font& dummy = {})
{
    WRITE("Name", name)
}

static void to_json(json& j, const Snapline& o, const Snapline& dummy = {})
{
    to_json(j, static_cast<const ColorToggleThickness&>(o), dummy);
    WRITE("Type", type)
}

static void to_json(json& j, const Box& o, const Box& dummy = {})
{
    to_json(j, static_cast<const ColorToggleRounding&>(o), dummy);
    WRITE("Type", type)
    WRITE("Scale", scale)
    to_json(j["Fill"], o.fill, dummy.fill);
}

static void to_json(json& j, const Shared& o, const Shared& dummy = {})
{
    WRITE("Enabled", enabled)
    to_json(j["Font"], o.font, dummy.font);
    to_json(j["Snapline"], o.snapline, dummy.snapline);
    to_json(j["Box"], o.box, dummy.box);
    to_json(j["Name"], o.name, dummy.name);
    WRITE("Text Cull Distance", textCullDistance)
}

static void to_json(json& j, const Player& o, const Player& dummy = {})
{
    to_json(j, static_cast<const Shared&>(o), dummy);
    to_json(j["Weapon"], o.weapon, dummy.weapon);
    to_json(j["Flash Duration"], o.flashDuration, dummy.flashDuration);
    WRITE("Audible Only", audibleOnly)
    WRITE("Spotted Only", spottedOnly)
    to_json(j["Skeleton"], o.skeleton, dummy.skeleton);
    to_json(j["Head Box"], o.headBox, dummy.headBox);
    WRITE("Health Bar", healthBar)
}

static void to_json(json& j, const Weapon& o, const Weapon& dummy = {})
{
    to_json(j, static_cast<const Shared&>(o), dummy);
    to_json(j["Ammo"], o.ammo, dummy.ammo);
}

static void to_json(json& j, const Trail& o, const Trail& dummy = {})
{
    to_json(j, static_cast<const ColorToggleThickness&>(o), dummy);
    WRITE("Type", type)
    WRITE("Time", time)
}

static void to_json(json& j, const Trails& o, const Trails& dummy = {})
{
    WRITE("Enabled", enabled)
    to_json(j["Local Player"], o.localPlayer, dummy.localPlayer);
    to_json(j["Allies"], o.allies, dummy.allies);
    to_json(j["Enemies"], o.enemies, dummy.enemies);
}

static void to_json(json& j, const Projectile& o, const Projectile& dummy = {})
{
    to_json(j, static_cast<const Shared&>(o), dummy);
    to_json(j["Trails"], o.trails, dummy.trails);
}

static void to_json(json& j, const ImVec2& o, const ImVec2& dummy = {})
{
    WRITE("X", x)
    WRITE("Y", y)
}

template <value_t Type, typename T>
static void read(const json& j, const char* key, T& o) noexcept
{
    if (!j.contains(key))
        return;

    if (const auto& val = j[key]; val.type() == Type)
        val.get_to(o);
}

static void read(const json& j, const char* key, bool& o) noexcept
{
    if (!j.contains(key))
        return;

    if (const auto& val = j[key]; val.type() == value_t::boolean)
        val.get_to(o);
}

template <typename T, size_t Size>
static void read(const json& j, const char* key, std::array<T, Size>& o) noexcept
{
    if (!j.contains(key))
        return;

    if (const auto& val = j[key]; val.type() == value_t::array && val.size() == o.size())
        val.get_to(o);
}

template <typename T>
static void read_number(const json& j, const char* key, T& o) noexcept
{
    if (!j.contains(key))
        return;

    if (const auto& val = j[key]; val.is_number())
        val.get_to(o);
}

template <typename T>
static void read_map(const json& j, const char* key, std::unordered_map<std::string, T>& o) noexcept
{
    if (!j.contains(key))
        return;

    if (const auto& val = j[key]; val.is_object()) {
        for (auto& element : val.items())
            element.value().get_to(o[element.key()]);
    }
}

static void from_json(const json& j, Color& c)
{
    read(j, "Color", c.color);
    read(j, "Rainbow", c.rainbow);
    read_number(j, "Rainbow Speed", c.rainbowSpeed);
}

static void from_json(const json& j, ColorToggle& ct)
{
    from_json(j, static_cast<Color&>(ct));

    read(j, "Enabled", ct.enabled);
}

static void from_json(const json& j, ColorToggleRounding& ctr)
{
    from_json(j, static_cast<ColorToggle&>(ctr));

    read_number(j, "Rounding", ctr.rounding);
}

static void from_json(const json& j, ColorToggleThickness& ctt)
{
    from_json(j, static_cast<ColorToggle&>(ctt));

    read_number(j, "Thickness", ctt.thickness);
}

static void from_json(const json& j, ColorToggleThicknessRounding& cttr)
{
    from_json(j, static_cast<ColorToggleRounding&>(cttr));

    read_number(j, "Thickness", cttr.thickness);
}

static void from_json(const json& j, Snapline& s)
{
    from_json(j, static_cast<ColorToggleThickness&>(s));

    read_number(j, "Type", s.type);
}

static void from_json(const json& j, Box& b)
{
    from_json(j, static_cast<ColorToggleRounding&>(b));

    read_number(j, "Type", b.type);
    read(j, "Scale", b.scale);
    read<value_t::object>(j, "Fill", b.fill);
}

static void from_json(const json& j, Trail& t)
{
    from_json(j, static_cast<ColorToggleThickness&>(t));

    read_number(j, "Type", t.type);
    read_number(j, "Time", t.time);
}

static void from_json(const json& j, Trails& t)
{
    read(j, "Enabled", t.enabled);
    read<value_t::object>(j, "Local Player", t.localPlayer);
    read<value_t::object>(j, "Allies", t.allies);
    read<value_t::object>(j, "Enemies", t.enemies);
}

static void from_json(const json& j, ImVec2& v)
{
    read_number(j, "X", v.x);
    read_number(j, "Y", v.y);
}
