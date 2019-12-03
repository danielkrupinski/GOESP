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
    if (const auto& enabled = j["Enabled"]; enabled.is_boolean())
        ct.enabled = enabled;
    from_json(j, static_cast<Config::Color&>(ct));
}

static void from_json(const json& j, Config::Shared& s)
{
    if (const auto& enabled = j["Enabled"]; enabled.is_boolean())
        s.enabled = enabled;
    if (const auto& font = j["Font"]; font.is_string()) {
        s.font = font;
        if (!s.font.empty())
            config.scheduleFontLoad(s.font);
        if (const auto it = std::find_if(std::cbegin(config.systemFonts), std::cend(config.systemFonts), [&s](const auto& e) { return e.second == s.font; }); it != std::cend(config.systemFonts))
            s.fontIndex = std::distance(std::cbegin(config.systemFonts), it);
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

void Config::load() noexcept
{
    json j;

    if (std::ifstream in{ path / "test.txt" }; in.good())
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
    if (const auto& heavy = j["Heavy"]; heavy.is_array() && heavy.size() == this->heavy.size())
        this->heavy = heavy;
    if (const auto& grenades = j["Grenades"]; grenades.is_array() && grenades.size() == this->grenades.size())
        this->grenades = grenades;
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
    j["Heavy"] = heavy;
    j["Grenades"] = grenades;

    if (std::ofstream out{ path / "test.txt" }; out.good())
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
            fonts[font] = ImGui::GetIO().Fonts->AddFontFromFileTTF((fontsPath / font).string().c_str(), 15.0f);
            result = true;
        }
    }
    scheduledFonts.clear();
    return result;
}
