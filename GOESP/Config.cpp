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

using json = nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int64_t, std::uint64_t, float>;
using value_t = json::value_t;

template <value_t Type, typename T>
static constexpr void read(const json& j, const char* key, T& o) noexcept
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

template <typename T>
static constexpr void read_number(const json& j, const char* key, T& o) noexcept
{
    if (j.contains(key) && j[key].is_number())
        o = j[key];
}

template <typename T>
static constexpr void read_map(const json& j, const char* key, T& o) noexcept
{
    if (j.contains(key) && j[key].is_object()) {
        for (auto& element : j[key].items())
            o[element.key()] = element.value();
    }
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

static void from_json(const json& j, Trail& t)
{
    read<value_t::boolean>(j, "Enabled", t.enabled);
    read_number(j, "Local Player Time", t.localPlayerTime);
    read_number(j, "Allies Time", t.alliesTime);
    read_number(j, "Enemies Time", t.enemiesTime);
    read<value_t::object>(j, "Local Player", t.localPlayer);
    read<value_t::object>(j, "Allies", t.allies);
    read<value_t::object>(j, "Enemies", t.enemies);
}

static void from_json(const json& j, Projectile& p)
{
    from_json(j, static_cast<Shared&>(p));

    read<value_t::object>(j, "Trail", p.trail);
}

static void from_json(const json& j, Player& p)
{
    from_json(j, static_cast<Shared&>(p));

    read<value_t::object>(j, "Weapon", p.weapon);
    read<value_t::object>(j, "Flash Duration", p.flashDuration);
    read<value_t::boolean>(j, "Audible Only", p.audibleOnly);
}

static void from_json(const json& j, PurchaseList& pl)
{
    read<value_t::boolean>(j, "Enabled", pl.enabled);
    read<value_t::boolean>(j, "Only During Freeze Time", pl.onlyDuringFreezeTime);
    read<value_t::boolean>(j, "Show Prices", pl.showPrices);
    read_number(j, "Mode", pl.mode);
}

void Config::load() noexcept
{
    json j;

    if (std::ifstream in{ path / "config.txt" }; in.good())
        in >> j;
    else
        return;

    read_map(j, "Allies", allies);
    read_map(j, "Enemies", enemies);
    read_map(j, "Weapons", _weapons);
    read_map(j, "Projectiles", _projectiles);
    read_map(j, "Other Entities", _otherEntities);

    read<value_t::object>(j, "Reload Progress", reloadProgress);
    read<value_t::object>(j, "Recoil Crosshair", recoilCrosshair);
    read<value_t::boolean>(j, "Normalize Player Names", normalizePlayerNames);
    read<value_t::boolean>(j, "Bomb Zone Hint", bombZoneHint);
    read<value_t::object>(j, "Purchase List", purchaseList);
}

static void to_json(json& j, const Color& c)
{
    const Color dummy;

    if (c.color != dummy.color)
        j["Color"] = c.color;
    if (c.rainbow != dummy.rainbow)
        j["Rainbow"] = c.rainbow;
    if (c.rainbowSpeed != dummy.rainbowSpeed)
        j["Rainbow Speed"] = c.rainbowSpeed;
}

static void to_json(json& j, const ColorToggle& ct)
{
    j = static_cast<Color>(ct);

    const ColorToggle dummy;

    if (ct.enabled != dummy.enabled)
        j["Enabled"] = ct.enabled;
}

static void to_json(json& j, const ColorToggleRounding& ctr)
{
    j = static_cast<ColorToggle>(ctr);

    const ColorToggleRounding dummy;

    if (ctr.rounding != dummy.rounding)
        j["Rounding"] = ctr.rounding;
}

static void to_json(json& j, const ColorToggleThickness& ctt)
{
    j = static_cast<ColorToggle>(ctt);

    const ColorToggleThickness dummy;

    if (ctt.thickness != dummy.thickness)
        j["Thickness"] = ctt.thickness;
}

static void to_json(json& j, const ColorToggleThicknessRounding& cttr)
{
    j = static_cast<ColorToggleRounding>(cttr);

    const ColorToggleThicknessRounding dummy;

    if (cttr.thickness != dummy.thickness)
        j["Thickness"] = cttr.thickness;
}

static void to_json(json& j, const Font& f)
{
    const Font dummy;

    if (f.size != dummy.size)
        j["Size"] = f.size;
    if (f.name != dummy.name)
        j["Name"] = f.name;
}

static void to_json(json& j, const Shared& s)
{
    const Shared dummy;

    if (s.enabled != dummy.enabled)
        j["Enabled"] = s.enabled;
    if (s.font != dummy.font)
        j["Font"] = s.font;
    if (s.snaplines != dummy.snaplines)
        j["Snaplines"] = s.snaplines;
    if (s.snaplineType != dummy.snaplineType)
        j["Snapline Type"] = s.snaplineType;
    if (s.box != dummy.box)
        j["Box"] = s.box;
    if (s.boxType != dummy.boxType)
        j["Box Type"] = s.boxType;
    if (s.name != dummy.name)
        j["Name"] = s.name;
    if (s.textBackground != dummy.textBackground)
        j["Text Background"] = s.textBackground;
    if (s.textCullDistance != dummy.textCullDistance)
        j["Text Cull Distance"] = s.textCullDistance;
}

static void to_json(json& j, const Player& p)
{
    j = static_cast<Shared>(p);

    const Player dummy;
    
    if (p.weapon != dummy.weapon)
        j["Weapon"] = p.weapon;
    if (p.flashDuration != dummy.flashDuration)
        j["Flash Duration"] = p.flashDuration;
    if (p.audibleOnly != dummy.audibleOnly)
        j["Audible Only"] = p.audibleOnly;
}

static void to_json(json& j, const Weapon& w)
{
    j = static_cast<Shared>(w);

    const Weapon dummy;

    if (w.ammo != dummy.ammo)
        j["Ammo"] = w.ammo;
}

static void to_json(json& j, const Trail& t)
{
    const Trail dummy;

    if (t.enabled != dummy.enabled)
        j["Enabled"] = t.enabled;
    if (t.localPlayerTime != dummy.localPlayerTime)
        j["Local Player Time"] = t.localPlayerTime;
    if (t.alliesTime != dummy.alliesTime)
        j["Allies Time"] = t.alliesTime;
    if (t.enemiesTime != dummy.enemiesTime)
        j["Enemies Time"] = t.enemiesTime;
    if (t.localPlayer != dummy.localPlayer)
        j["Local Player"] = t.localPlayer;
    if (t.allies != dummy.allies)
        j["Allies"] = t.allies;
    if (t.enemies != dummy.enemies)
        j["Enemies"] = t.enemies;
}

static void to_json(json& j, const Projectile& p)
{
    j = static_cast<Shared>(p);

    const Projectile dummy;

    if (p.trail != dummy.trail)
        j["Trail"] = p.trail;
}

static void to_json(json& j, const PurchaseList& pl)
{
    const PurchaseList dummy;

    if (pl.enabled != dummy.enabled)
        j["Enabled"] = pl.enabled;
    if (pl.onlyDuringFreezeTime != dummy.onlyDuringFreezeTime)
        j["Only During Freeze Time"] = pl.onlyDuringFreezeTime;
    if (pl.showPrices != dummy.showPrices)
        j["Show Prices"] = pl.showPrices;
    if (pl.mode != dummy.mode)
        j["Mode"] = pl.mode;
}

void Config::save() noexcept
{
    json j;

    for (const auto& [key, value] : allies)
        if (value != Player{})
            j["Allies"][key] = value;

    for (const auto& [key, value] : enemies)
        if (value != Player{})
            j["Enemies"][key] = value;

    for (const auto& [key, value] : _weapons)
        if (value != Weapon{})
            j["Weapons"][key] = value;

    for (const auto& [key, value] : _projectiles)
        if (value != Projectile{})
            j["Projectiles"][key] = value;

    for (const auto& [key, value] : _otherEntities)
        if (value != Shared{})
            j["Other Entities"][key] = value;

    if (reloadProgress != ColorToggleThickness{ 5.0f })
        j["Reload Progress"] = reloadProgress;
    if (recoilCrosshair != ColorToggleThickness{})
        j["Recoil Crosshair"] = recoilCrosshair;
    if (normalizePlayerNames != true)
        j["Normalize Player Names"] = normalizePlayerNames;
    if (bombZoneHint != false)
        j["Bomb Zone Hint"] = bombZoneHint;
    if (purchaseList != PurchaseList{})
        j["Purchase List"] = purchaseList;

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
