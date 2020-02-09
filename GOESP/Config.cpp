#include "Config.h"

#include "imgui/imgui.h"
#include "nlohmann/json.hpp"

#include <fstream>
#include <ShlObj.h>
#include <Windows.h>

Config::Config(const char* folderName) noexcept
{
    if (PWSTR pathToDocuments; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocuments))) {
        path = pathToDocuments;
        path /= folderName;
        CoTaskMemFree(pathToDocuments);
    }

    if (!std::filesystem::is_directory(path)) {
        std::filesystem::remove(path);
        std::filesystem::create_directory(path);
    }

    if (HKEY key; RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &key) == ERROR_SUCCESS) {
        if (DWORD values; RegQueryInfoKeyW(key, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &values, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            for (DWORD i = 0; i < values; ++i) {
                CHAR fontName[200], fontFilename[50];
                DWORD fontNameLength = 200, fontFilenameLength = 50;

                if (RegEnumValueA(key, i, fontName, &fontNameLength, nullptr, nullptr, PBYTE(fontFilename), &fontFilenameLength) == ERROR_SUCCESS && (std::strstr(fontFilename, ".ttf") || std::strstr(fontFilename, ".TTF")))
                    systemFonts.emplace_back(fontName, fontFilename);
            }
        }
        RegCloseKey(key);
    }

    if (PWSTR pathToFonts; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &pathToFonts))) {
        fontsPath = pathToFonts;
        CoTaskMemFree(pathToFonts);
    }
}

using json = nlohmann::json;
using value_t = json::value_t;

template <value_t Type, typename T>
static constexpr std::enable_if_t<Type != value_t::array> read(const json& j, const char* key, T& o) noexcept
{
    if (j.contains(key) && j[key].type() == Type)
        o = j[key];
}

template <value_t Type, typename T, size_t Size>
static constexpr void read(const json& j, const char* key, std::array<T, Size>& o) noexcept
{
    if (j.contains(key) && j[key].type() == Type && j[key].size() == o.size())
        o = j[key];
}

static void from_json(const json& j, Config::Color& c)
{
    read<value_t::array>(j, "Color", c.color);
    read<value_t::boolean>(j, "Rainbow", c.rainbow);
    read<value_t::number_float>(j, "Rainbow Speed", c.rainbowSpeed);
}

static void from_json(const json& j, Config::ColorToggle& ct)
{
    from_json(j, static_cast<Config::Color&>(ct));

    read<value_t::boolean>(j, "Enabled", ct.enabled);
}

static void from_json(const json& j, Config::ColorToggleRounding& ctr)
{
    from_json(j, static_cast<Config::ColorToggle&>(ctr));

    read<value_t::number_float>(j, "Rounding", ctr.rounding);
}

static void from_json(const json& j, Config::ColorToggleThickness& ctt)
{
    from_json(j, static_cast<Config::ColorToggle&>(ctt));

    read<value_t::number_float>(j, "Thickness", ctt.thickness);
}

static void from_json(const json& j, Config::ColorToggleThicknessRounding& cttr)
{
    from_json(j, static_cast<Config::ColorToggleRounding&>(cttr));

    read<value_t::number_float>(j, "Thickness", cttr.thickness);
}

static void from_json(const json& j, Config::Shared& s)
{
    read<value_t::boolean>(j, "Enabled", s.enabled);
    read<value_t::string>(j, "Font", s.font);

    if (!s.font.empty())
        config->scheduleFontLoad(s.font);
    if (const auto it = std::find_if(std::cbegin(config->systemFonts), std::cend(config->systemFonts), [&s](const auto& e) { return e.second == s.font; }); it != std::cend(config->systemFonts))
        s.fontIndex = std::distance(std::cbegin(config->systemFonts), it);
    else
        s.fontIndex = 0;

    read<value_t::object>(j, "Snaplines", s.snaplines);
    read<value_t::number_integer>(j, "Snapline Type", s.snaplineType);
    read<value_t::object>(j, "Box", s.box);
    read<value_t::number_integer>(j, "Box Type", s.boxType);
    read<value_t::object>(j, "Name", s.name);
    read<value_t::object>(j, "Text Background", s.textBackground);
    read<value_t::number_float>(j, "Text Cull Distance", s.textCullDistance);
}

static void from_json(const json& j, Config::Weapon& w)
{
    from_json(j, static_cast<Config::Shared&>(w));

    read<value_t::object>(j, "Ammo", w.ammo);
}

static void from_json(const json& j, Config::Player& p)
{
    from_json(j, static_cast<Config::Shared&>(p));

    read<value_t::object>(j, "Weapon", p.weapon);
    read<value_t::object>(j, "Flash Duration", p.flashDuration);
}

void Config::load() noexcept
{
    json j;

    if (std::ifstream in{ path / "config.txt" }; in.good())
        in >> j;
    else
        return;

    read<value_t::array>(j, "Players", players);

    read<value_t::object>(j, "Weapons", weapons);
    read<value_t::array>(j, "Pistols", pistols);
    read<value_t::array>(j, "SMGs", smgs);
    read<value_t::array>(j, "Rifles", rifles);
    read<value_t::array>(j, "Sniper Rifles", sniperRifles);
    read<value_t::array>(j, "Shotguns", shotguns);
    read<value_t::array>(j, "Machineguns", machineguns);
    read<value_t::array>(j, "Grenades", grenades);

    read<value_t::array>(j, "Projectiles", projectiles);
    read<value_t::array>(j, "Other Entities", otherEntities);

    read<value_t::object>(j, "Reload Progress", reloadProgress);
    read<value_t::object>(j, "Recoil Crosshair", recoilCrosshair);
    read<value_t::boolean>(j, "Normalize Player Names", normalizePlayerNames);
}

static void to_json(json& j, const Config::Color& c)
{
    j = json{ { "Color", c.color },
              { "Rainbow", c.rainbow },
              { "Rainbow Speed", c.rainbowSpeed }
    };
}

static void to_json(json& j, const Config::ColorToggle& ct)
{
    j = static_cast<Config::Color>(ct);
    j["Enabled"] = ct.enabled;
}

static void to_json(json& j, const Config::ColorToggleRounding& ctr)
{
    j = static_cast<Config::ColorToggle>(ctr);
    j["Rounding"] = ctr.rounding;
}

static void to_json(json& j, const Config::ColorToggleThickness& ctt)
{
    j = static_cast<Config::ColorToggle>(ctt);
    j["Thickness"] = ctt.thickness;
}

static void to_json(json& j, const Config::ColorToggleThicknessRounding& cttr)
{
    j = static_cast<Config::ColorToggleRounding>(cttr);
    j["Thickness"] = cttr.thickness;
}

static void to_json(json& j, const Config::Shared& s)
{
    j = json{ { "Enabled", s.enabled },
              { "Font", s.font },
              { "Snaplines", s.snaplines },
              { "Snapline Type", s.snaplineType },
              { "Box", s.box },
              { "Box Type", s.boxType },
              { "Name", s.name }, 
              { "Text Background", s.textBackground },
              { "Text Cull Distance", s.textCullDistance }
    };
}

static void to_json(json& j, const Config::Player& p)
{
    j = static_cast<Config::Shared>(p);
    j["Weapon"] = p.weapon;
    j["Flash Duration"] = p.flashDuration;
}

static void to_json(json& j, const Config::Weapon& w)
{
    j = static_cast<Config::Shared>(w);
    j["Ammo"] = w.ammo;
}

void Config::save() noexcept
{
    json j;
    j["Players"] = players;

    j["Weapons"] = weapons;
    j["Pistols"] = pistols;
    j["SMGs"] = smgs;
    j["Rifles"] = rifles;
    j["Sniper Rifles"] = sniperRifles;
    j["Shotguns"] = shotguns;
    j["Machineguns"] = machineguns;
    j["Grenades"] = grenades;

    j["Projectiles"] = projectiles;
    j["Other Entities"] = otherEntities;

    j["Reload Progress"] = reloadProgress;
    j["Recoil Crosshair"] = recoilCrosshair;
    j["Normalize Player Names"] = normalizePlayerNames;

    if (std::ofstream out{ path / "config.txt" }; out.good())
        out << std::setw(4) << j;
}

void Config::scheduleFontLoad(const std::string& name) noexcept
{
    scheduledFonts.push_back(name);
}

bool Config::loadScheduledFonts() noexcept
{
    bool result = false;

    for (const auto& font : scheduledFonts) {
        if (!fonts[font] && !font.empty()) {
            static constexpr ImWchar ranges[]{ 0x0020, 0xFFFF, 0 };
            fonts[font] = ImGui::GetIO().Fonts->AddFontFromFileTTF((fontsPath / font).string().c_str(), 15.0f, nullptr, ranges);
            result = true;
        }
    }
    scheduledFonts.clear();
    return result;
}
