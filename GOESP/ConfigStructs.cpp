#include "ConfigStructs.h"

void to_json(json& j, const Color& o, const Color& dummy)
{
    WRITE("Color", color)
    WRITE("Rainbow", rainbow)
    WRITE("Rainbow Speed", rainbowSpeed)
}

void to_json(json& j, const ColorToggle& o, const ColorToggle& dummy)
{
    to_json(j, static_cast<const Color&>(o), dummy);
    WRITE("Enabled", enabled)
}

void to_json(json& j, const ColorToggleRounding& o, const ColorToggleRounding& dummy)
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Rounding", rounding)
}

void to_json(json& j, const ColorToggleThickness& o, const ColorToggleThickness& dummy)
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Thickness", thickness)
}

void to_json(json& j, const ColorToggleThicknessRounding& o, const ColorToggleThicknessRounding& dummy)
{
    to_json(j, static_cast<const ColorToggleRounding&>(o), dummy);
    WRITE("Thickness", thickness)
}

void to_json(json& j, const ImVec2& o, const ImVec2& dummy)
{
    WRITE("X", x)
    WRITE("Y", y)
}

void read(const json& j, const char* key, bool& o) noexcept
{
    if (!j.contains(key))
        return;

    if (const auto& val = j[key]; val.type() == value_t::boolean)
        val.get_to(o);
}

void from_json(const json& j, Color& c)
{
    read(j, "Color", c.color);
    read(j, "Rainbow", c.rainbow);
    read_number(j, "Rainbow Speed", c.rainbowSpeed);
}

void from_json(const json& j, ColorToggle& ct)
{
    from_json(j, static_cast<Color&>(ct));

    read(j, "Enabled", ct.enabled);
}

void from_json(const json& j, ColorToggleRounding& ctr)
{
    from_json(j, static_cast<ColorToggle&>(ctr));

    read_number(j, "Rounding", ctr.rounding);
}

void from_json(const json& j, ColorToggleThickness& ctt)
{
    from_json(j, static_cast<ColorToggle&>(ctt));

    read_number(j, "Thickness", ctt.thickness);
}

void from_json(const json& j, ColorToggleThicknessRounding& cttr)
{
    from_json(j, static_cast<ColorToggleRounding&>(cttr));

    read_number(j, "Thickness", cttr.thickness);
}

void from_json(const json& j, ImVec2& v)
{
    read_number(j, "X", v.x);
    read_number(j, "Y", v.y);
}

void to_json(json& j, const HealthBar& o, const HealthBar& dummy)
{
    to_json(j, static_cast<const ColorToggle&>(o), dummy);
    WRITE("Type", type);
}

void from_json(const json& j, HealthBar& o)
{
    from_json(j, static_cast<ColorToggle&>(o));
    read_number(j, "Type", o.type);
}
