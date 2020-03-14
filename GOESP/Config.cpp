#include "Config.h"

#include "imgui/imgui.h"
#include "nlohmann/json.hpp"

#include <fstream>
#include <memory>
#include <ShlObj.h>
#include <Windows.h>
#include "Memory.h"

int CALLBACK fontCallback(const LOGFONTA* lpelfe, const TEXTMETRICA*, DWORD, LPARAM lParam)
{
    std::string fontName = (const char*)reinterpret_cast<const ENUMLOGFONTEXA*>(lpelfe)->elfFullName;

    if (fontName[0] == '@')
        return TRUE;

    HFONT fontHandle = CreateFontA(0, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, fontName.c_str());

    if (fontHandle) {
        HDC hdc = CreateCompatibleDC(nullptr);

        DWORD fontData = GDI_ERROR;

        if (hdc) {
            SelectObject(hdc, fontHandle);
            // Do not use TTC fonts as we only support TTF fonts
            fontData = GetFontData(hdc, 'fctt', 0, NULL, 0);
            DeleteDC(hdc);
        }
        DeleteObject(fontHandle);
        
        if (fontData != GDI_ERROR)
            return TRUE;
    }
    reinterpret_cast<std::vector<std::string>*>(lParam)->push_back(fontName);
    return TRUE;
}

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

    LOGFONTA logfont;
    logfont.lfCharSet = ANSI_CHARSET;
    logfont.lfFaceName[0] = '\0';
    logfont.lfPitchAndFamily = 0;

    EnumFontFamiliesExA(GetDC(nullptr), &logfont, fontCallback, (LPARAM)&systemFonts, 0);
    std::sort(std::next(systemFonts.begin()), systemFonts.end());
}

using json = nlohmann::json;
using value_t = json::value_t;

template <value_t Type, typename T>
static constexpr void read(const json& j, const char* key, T& o) noexcept
{
    if (j.contains(key) && j[key].type() == Type)
        o = j[key];
}

template <typename T>
static constexpr void read_number(const json& j, const char* key, T& o) noexcept
{
    if (j.contains(key) && j[key].is_number())
        o = j[key];
}

template <value_t Type, typename T, size_t Size>
static constexpr void read(const json& j, const char* key, std::array<T, Size>& o) noexcept
{
    if (j.contains(key) && j[key].type() == Type && j[key].size() == o.size())
        o = j[key];
}

static void from_json(const json& j, Color& c)
{
    read<value_t::array>(j, "Color", c.color);
    read<value_t::boolean>(j, "Rainbow", c.rainbow);
    read_number(j, "Rainbow Speed", c.rainbowSpeed);
}

static void from_json(const json& j, ColorToggle& ct)
{
    from_json(j, static_cast<Color&>(ct));

    read<value_t::boolean>(j, "Enabled", ct.enabled);
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

static void from_json(const json& j, Font& f)
{
    read_number(j, "Size", f.size);
    read<value_t::string>(j, "Name", f.name);

    if (!f.name.empty()) {
        f.fullName = f.name + ' ' + std::to_string(f.size);
        config->scheduleFontLoad(f.name);
    }
    if (const auto it = std::find_if(std::cbegin(config->systemFonts), std::cend(config->systemFonts), [&f](const auto& e) { return e == f.name; }); it != std::cend(config->systemFonts))
        f.index = std::distance(std::cbegin(config->systemFonts), it);
    else
        f.index = 0;
}

static void from_json(const json& j, Shared& s)
{
    read<value_t::boolean>(j, "Enabled", s.enabled);
    read<value_t::object>(j, "Font", s.font);
    read<value_t::object>(j, "Snaplines", s.snaplines);
    read_number(j, "Snapline Type", s.snaplineType);
    read<value_t::object>(j, "Box", s.box);
    read_number(j, "Box Type", s.boxType);
    read<value_t::object>(j, "Name", s.name);
    read<value_t::object>(j, "Text Background", s.textBackground);
    read_number(j, "Text Cull Distance", s.textCullDistance);
}

static void from_json(const json& j, Weapon& w)
{
    from_json(j, static_cast<Shared&>(w));

    read<value_t::object>(j, "Ammo", w.ammo);
}

static void from_json(const json& j, Player& p)
{
    from_json(j, static_cast<Shared&>(p));

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
    read<value_t::boolean>(j, "Purchase List", purchaseList);
    read_number(j, "Purchase List Mode", purchaseListMode);
    read<value_t::boolean>(j, "Purchase List Prices", purchaseListPrices);
}

static void to_json(json& j, const Color& c)
{
    j = json{ { "Color", c.color },
              { "Rainbow", c.rainbow },
              { "Rainbow Speed", c.rainbowSpeed }
    };
}

static void to_json(json& j, const ColorToggle& ct)
{
    j = static_cast<Color>(ct);
    j["Enabled"] = ct.enabled;
}

static void to_json(json& j, const ColorToggleRounding& ctr)
{
    j = static_cast<ColorToggle>(ctr);
    j["Rounding"] = ctr.rounding;
}

static void to_json(json& j, const ColorToggleThickness& ctt)
{
    j = static_cast<ColorToggle>(ctt);
    j["Thickness"] = ctt.thickness;
}

static void to_json(json& j, const ColorToggleThicknessRounding& cttr)
{
    j = static_cast<ColorToggleRounding>(cttr);
    j["Thickness"] = cttr.thickness;
}

static void to_json(json& j, const Font& f)
{
    j["Size"] = f.size;
    j["Name"] = f.name;
}

static void to_json(json& j, const Shared& s)
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

static void to_json(json& j, const Player& p)
{
    j = static_cast<Shared>(p);
    j["Weapon"] = p.weapon;
    j["Flash Duration"] = p.flashDuration;
}

static void to_json(json& j, const Weapon& w)
{
    j = static_cast<Shared>(w);
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
    j["Purchase List"] = purchaseList;
    j["Purchase List Mode"] = purchaseListMode;
    j["Purchase List Prices"] = purchaseListPrices;

    if (std::ofstream out{ path / "config.txt" }; out.good())
        out << std::setw(4) << j;
}

void Config::scheduleFontLoad(const std::string& name, int size) noexcept
{
    scheduledFonts.emplace_back(name, size);
}

bool Config::loadScheduledFonts() noexcept
{
    bool result = false;

    for (const auto& [font, size] : scheduledFonts) {
        if (font == "Default")
            continue;

        const std::string fullName = font + ' ' + std::to_string(size);

        if (fonts.find(fullName) != fonts.end())
            continue;

        HFONT fontHandle = CreateFontA(0, 0, 0, 0,
            FW_NORMAL, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH, font.c_str());

        if (fontHandle) {
            HDC hdc = CreateCompatibleDC(nullptr);

            if (hdc) {
                SelectObject(hdc, fontHandle);
                auto fontDataSize = GetFontData(hdc, 0, 0, nullptr, 0);

                if (fontDataSize != GDI_ERROR) {
                    std::unique_ptr<std::byte> fontData{ new std::byte[fontDataSize] };
                    fontDataSize = GetFontData(hdc, 0, 0, fontData.get(), fontDataSize);

                    if (fontDataSize != GDI_ERROR) {
                        static constexpr ImWchar ranges[]{ 0x0020, 0xFFFF, 0 };
                        // imgui handles fontData memory release
                        fonts[fullName] = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.release(), fontDataSize, static_cast<float>(size), nullptr, ranges);
                        result = true;
                    }
                }
                DeleteDC(hdc);
            }
            DeleteObject(fontHandle);
        }
    }
    scheduledFonts.clear();
    return result;
}
