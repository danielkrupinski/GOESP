#pragma once

#include <array>
#include <string>

struct ImVec2;

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

void to_json(json& j, const Color& o, const Color& dummy = {});
void to_json(json& j, const ColorToggle& o, const ColorToggle& dummy = {});
void to_json(json& j, const ColorToggleRounding& o, const ColorToggleRounding& dummy = {});
void to_json(json& j, const ColorToggleThickness& o, const ColorToggleThickness& dummy = {});
void to_json(json& j, const ColorToggleThicknessRounding& o, const ColorToggleThicknessRounding& dummy = {});
void to_json(json& j, const ImVec2& o, const ImVec2& dummy = {});

template <value_t Type, typename T>
void read(const json& j, const char* key, T& o) noexcept
{
    if (!j.contains(key))
        return;

    if (const auto& val = j[key]; val.type() == Type)
        val.get_to(o);
}

void read(const json& j, const char* key, bool& o) noexcept;

template <typename T, size_t Size>
void read(const json& j, const char* key, std::array<T, Size>& o) noexcept
{
    if (!j.contains(key))
        return;

    if (const auto& val = j[key]; val.type() == value_t::array && val.size() == o.size())
        val.get_to(o);
}

template <typename T>
void read_number(const json& j, const char* key, T& o) noexcept
{
    if (!j.contains(key))
        return;

    if (const auto& val = j[key]; val.is_number())
        val.get_to(o);
}

template <typename T>
void read_map(const json& j, const char* key, std::unordered_map<std::string, T>& o) noexcept
{
    if (!j.contains(key))
        return;

    if (const auto& val = j[key]; val.is_object()) {
        for (auto& element : val.items())
            element.value().get_to(o[element.key()]);
    }
}

void from_json(const json& j, Color& c);
void from_json(const json& j, ColorToggle& ct);
void from_json(const json& j, ColorToggleRounding& ctr);
void from_json(const json& j, ColorToggleThickness& ctt);
void from_json(const json& j, ColorToggleThicknessRounding& cttr);
void from_json(const json& j, ImVec2& v);

struct HealthBar : ColorToggle {
    enum Type {
        Gradient = 0,
        Solid,
        HealthBased
    };

    int type = Type::Gradient;
};

void to_json(json& j, const HealthBar& o, const HealthBar& dummy = {});
void from_json(const json& j, HealthBar& o);
