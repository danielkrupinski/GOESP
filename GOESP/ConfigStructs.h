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

#define WRITE_OBJ(name, valueName) to_json(j[name], o.valueName, dummy.valueName)

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

static void from_json(const json& j, ImVec2& v)
{
    read_number(j, "X", v.x);
    read_number(j, "Y", v.y);
}

struct HealthBar : ColorToggle {
    enum Type {
        Gradient = 0,
        Solid
    };

    int type = Type::Gradient;
};

static void to_json(json& j, const HealthBar& o, const HealthBar& dummy = {})
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Type", type);
}

static void from_json(const json& j, HealthBar& o)
{
    from_json(j, static_cast<ColorToggle&>(o));
    read_number(j, "Type", o.type);
}
