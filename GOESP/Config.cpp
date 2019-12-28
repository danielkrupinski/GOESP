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

static void from_json(const json& j, Config::Color& c)
{
    if (const auto& color = j["Color"]; color.is_array() && color.size() == c.color.size())
        c.color = color;
    if (const auto& rainbow = j["Rainbow"]; rainbow.is_boolean())
        c.rainbow = rainbow;
    if (const auto& rainbowSpeed = j["Rainbow Speed"]; rainbowSpeed.is_number_float())
        c.rainbowSpeed = rainbowSpeed;
}

static void from_json(const json& j, Config::ColorToggle& ct)
{
    from_json(j, static_cast<Config::Color&>(ct));

    if (const auto& enabled = j["Enabled"]; enabled.is_boolean())
        ct.enabled = enabled;
}

static void from_json(const json& j, Config::ColorToggleRounding& ctr)
{
    from_json(j, static_cast<Config::ColorToggle&>(ctr));

    if (const auto& rounding = j["Rounding"]; rounding.is_number_float())
        ctr.rounding = rounding;
}

static void from_json(const json& j, Config::ColorToggleThickness& ctt)
{
    from_json(j, static_cast<Config::ColorToggle&>(ctt));

    if (const auto& thickness = j["Thickness"]; thickness.is_number_float())
        ctt.thickness = thickness;
}

static void from_json(const json& j, Config::ColorToggleThicknessRounding& cttr)
{
    from_json(j, static_cast<Config::ColorToggleRounding&>(cttr));

    if (const auto& thickness = j["Thickness"]; thickness.is_number_float())
        cttr.thickness = thickness;
}

static void from_json(const json& j, Config::Shared& s)
{
    if (const auto& enabled = j["Enabled"]; enabled.is_boolean())
        s.enabled = enabled;
    if (const auto& font = j["Font"]; font.is_string()) {
        s.font = font;
        if (!s.font.empty())
            config->scheduleFontLoad(s.font);
        if (const auto it = std::find_if(std::cbegin(config->systemFonts), std::cend(config->systemFonts), [&s](const auto& e) { return e.second == s.font; }); it != std::cend(config->systemFonts))
            s.fontIndex = std::distance(std::cbegin(config->systemFonts), it);
        else
            s.fontIndex = 0;
    }
    if (const auto& snaplines = j["Snaplines"]; snaplines.is_object())
        s.snaplines = snaplines;
    if (const auto& box = j["Box"]; box.is_object())
        s.box = box;
    if (const auto& boxType = j["Box Type"]; boxType.is_number_integer())
        s.boxType = boxType;
    if (const auto& name = j["Name"]; name.is_object())
        s.name = name;
}

static void from_json(const json& j, Config::Weapon& w)
{
    if (const auto& ammo = j["Ammo"]; ammo.is_object())
        w.ammo = ammo;
    from_json(j, static_cast<Config::Shared&>(w));
}

static void from_json(const json& j, Config::Player& p)
{
    from_json(j, static_cast<Config::Shared&>(p));

    if (const auto& weapon = j["Weapon"]; weapon.is_object())
        p.weapon = weapon;
}

void Config::load() noexcept
{
    json j;

    if (std::ifstream in{ path / "config.txt" }; in.good())
        in >> j;
    else
        return;

    if (const auto& players = j["Players"]; players.is_array() && players.size() == this->players.size())
        this->players = players;

    if (const auto& weapons = j["Weapons"]; weapons.is_object())
        this->weapons = weapons;
    if (const auto& pistols = j["Pistols"]; pistols.is_array() && pistols.size() == this->pistols.size())
        this->pistols = pistols;
    if (const auto& smgs = j["SMGs"]; smgs.is_array() && smgs.size() == this->smgs.size())
        this->smgs = smgs;
    if (const auto& rifles = j["Rifles"]; rifles.is_array() && rifles.size() == this->rifles.size())
        this->rifles = rifles;
    if (const auto& sniperRifles = j["Sniper Rifles"]; sniperRifles.is_array() && sniperRifles.size() == this->sniperRifles.size())
        this->sniperRifles = sniperRifles;
    if (const auto& shotguns = j["Shotguns"]; shotguns.is_array() && shotguns.size() == this->shotguns.size())
        this->shotguns = shotguns;
    if (const auto& machineguns = j["Machineguns"]; machineguns.is_array() && machineguns.size() == this->machineguns.size())
        this->machineguns = machineguns;
    if (const auto& grenades = j["Grenades"]; grenades.is_array() && grenades.size() == this->grenades.size())
        this->grenades = grenades;

    if (const auto& projectiles = j["Projectiles"]; projectiles.is_array() && projectiles.size() == this->projectiles.size())
        this->projectiles = projectiles;
    if (const auto& otherEntities = j["Other Entities"]; otherEntities.is_array() && otherEntities.size() == this->otherEntities.size())
        this->otherEntities = otherEntities;

    if (const auto& reloadProgress = j["Reload Progress"]; reloadProgress.is_object())
        this->reloadProgress = reloadProgress;
    if (const auto& recoilCrosshair = j["Recoil Crosshair"]; recoilCrosshair.is_object())
        this->recoilCrosshair = recoilCrosshair;
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
              { "Box", s.box },
              { "Box Type", s.boxType },
              { "Name", s.name }
    };
}

static void to_json(json& j, const Config::Player& p)
{
    j = static_cast<Config::Shared>(p);
    j["Weapon"] = p.weapon;
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
            static constexpr ImWchar ranges[]{
                0x0020, 0x00FF, // Basic Latin + Latin Supplement
                0x0100, 0x017F, // Latin Extended-A
                0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
                0x2DE0, 0x2DFF, // Cyrillic Extended-A
                0xA640, 0xA69F, // Cyrillic Extended-B
                0
            };
            fonts[font] = ImGui::GetIO().Fonts->AddFontFromFileTTF((fontsPath / font).string().c_str(), 15.0f, nullptr, ranges);
            result = true;
        }
    }
    scheduledFonts.clear();
    return result;
}
