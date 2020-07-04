#include <fstream>
#include <memory>

#ifdef _WIN32
#include <ShlObj.h>
#include <Windows.h>
#endif

#include "Config.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "nlohmann/json.hpp"

#ifdef _WIN32
int CALLBACK fontCallback(const LOGFONTA* lpelfe, const TEXTMETRICA*, DWORD, LPARAM lParam)
{
    const auto fontName = (const char*)reinterpret_cast<const ENUMLOGFONTEXA*>(lpelfe)->elfFullName;

    if (fontName[0] == '@')
        return TRUE;

    if (HFONT font = CreateFontA(0, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, fontName)) {

        DWORD fontData = GDI_ERROR;

        if (HDC hdc = CreateCompatibleDC(nullptr)) {
            SelectObject(hdc, font);
            // Do not use TTC fonts as we only support TTF fonts
            fontData = GetFontData(hdc, 'fctt', 0, NULL, 0);
            DeleteDC(hdc);
        }
        DeleteObject(font);

        if (fontData == GDI_ERROR)
            reinterpret_cast<std::vector<std::string>*>(lParam)->emplace_back(fontName);
    }
    return TRUE;
}
#endif

Config::Config(const char* folderName) noexcept
{
#ifdef _WIN32
    if (PWSTR pathToDocuments; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocuments))) {
        path = pathToDocuments;
        path /= folderName;
        CoTaskMemFree(pathToDocuments);
    }

    LOGFONTA logfont;
    logfont.lfCharSet = ANSI_CHARSET;
    logfont.lfPitchAndFamily = DEFAULT_PITCH;
    logfont.lfFaceName[0] = '\0';

    EnumFontFamiliesExA(GetDC(nullptr), &logfont, fontCallback, (LPARAM)&systemFonts, 0);
#endif
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
static constexpr void read_map(const json& j, const char* key, std::unordered_map<std::string, T>& o) noexcept
{
    if (j.contains(key) && j[key].is_object()) {
        for (auto& element : j[key].items())
            o[element.key()] = static_cast<const T&>(element.value());
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
    read<value_t::string>(j, "Name", f.name);

    if (!f.name.empty())
        config->scheduleFontLoad(f.name);

    if (const auto it = std::find_if(std::cbegin(config->systemFonts), std::cend(config->systemFonts), [&f](const auto& e) { return e == f.name; }); it != std::cend(config->systemFonts))
        f.index = std::distance(std::cbegin(config->systemFonts), it);
    else
        f.index = 0;
}

static void from_json(const json& j, Snapline& s)
{
    from_json(j, static_cast<ColorToggleThickness&>(s));

    read_number(j, "Type", s.type);
}

static void from_json(const json& j, Box& b)
{
    from_json(j, static_cast<ColorToggleThicknessRounding&>(b));

    read_number(j, "Type", b.type);
    read<value_t::array>(j, "Scale", b.scale);
}

static void from_json(const json& j, Shared& s)
{
    read<value_t::boolean>(j, "Enabled", s.enabled);
    read<value_t::object>(j, "Font", s.font);
    read<value_t::object>(j, "Snapline", s.snapline);
    read<value_t::object>(j, "Box", s.box);
    read<value_t::object>(j, "Name", s.name);
    read_number(j, "Text Cull Distance", s.textCullDistance);
}

static void from_json(const json& j, Weapon& w)
{
    from_json(j, static_cast<Shared&>(w));

    read<value_t::object>(j, "Ammo", w.ammo);
}

static void from_json(const json& j, Trail& t)
{
    from_json(j, static_cast<ColorToggleThickness&>(t));

    read_number(j, "Type", t.type);
    read_number(j, "Time", t.time);
}

static void from_json(const json& j, Trails& t)
{
    read<value_t::boolean>(j, "Enabled", t.enabled);
    read<value_t::object>(j, "Local Player", t.localPlayer);
    read<value_t::object>(j, "Allies", t.allies);
    read<value_t::object>(j, "Enemies", t.enemies);
}

static void from_json(const json& j, Projectile& p)
{
    from_json(j, static_cast<Shared&>(p));

    read<value_t::object>(j, "Trails", p.trails);
}

static void from_json(const json& j, Player& p)
{
    from_json(j, static_cast<Shared&>(p));

    read<value_t::object>(j, "Weapon", p.weapon);
    read<value_t::object>(j, "Flash Duration", p.flashDuration);
    read<value_t::boolean>(j, "Audible Only", p.audibleOnly);
    read<value_t::boolean>(j, "Spotted Only", p.spottedOnly);
    read<value_t::object>(j, "Skeleton", p.skeleton);
}

static void from_json(const json& j, ImVec2& v)
{
    read_number(j, "X", v.x);
    read_number(j, "Y", v.y);
}

static void from_json(const json& j, PurchaseList& pl)
{
    read<value_t::boolean>(j, "Enabled", pl.enabled);
    read<value_t::boolean>(j, "Only During Freeze Time", pl.onlyDuringFreezeTime);
    read<value_t::boolean>(j, "Show Prices", pl.showPrices);
    read<value_t::boolean>(j, "No Title Bar", pl.noTitleBar);
    read_number(j, "Mode", pl.mode);
    read<value_t::object>(j, "Pos", pl.pos);
    read<value_t::object>(j, "Size", pl.size);
}

static void from_json(const json& j, ObserverList& ol)
{
    read<value_t::boolean>(j, "Enabled", ol.enabled);
    read<value_t::boolean>(j, "No Title Bar", ol.noTitleBar);
    read<value_t::object>(j, "Pos", ol.pos);
    read<value_t::object>(j, "Size", ol.size);
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
    read_map(j, "Weapons", weapons);
    read_map(j, "Projectiles", projectiles);
    read_map(j, "Loot Crates", lootCrates);
    read_map(j, "Other Entities", otherEntities);

    read<value_t::object>(j, "Reload Progress", reloadProgress);
    read<value_t::object>(j, "Recoil Crosshair", recoilCrosshair);
    read<value_t::object>(j, "Noscope Crosshair", noscopeCrosshair);
    read<value_t::object>(j, "Purchase List", purchaseList);
    read<value_t::object>(j, "Observer List", observerList);
    read<value_t::boolean>(j, "Ignore Flashbang", ignoreFlashbang);
}

// WRITE macro requires:
// - json object named 'j'
// - object holding default values named 'dummy'
// - object to write to json named 'o'
#define WRITE(name, valueName) \
if (!(o.valueName == dummy.valueName)) \
    j[name] = o.valueName;

// WRITE_BASE macro requires:
// - json object named 'j'
// - object to write to json named 'o'
#define WRITE_BASE(structName) \
if (!(static_cast<const structName&>(o) == static_cast<const structName&>(dummy))) \
    j = static_cast<const structName&>(o);

static void to_json(json& j, const Color& o)
{
    const Color dummy;

    WRITE("Color", color)
    WRITE("Rainbow", rainbow)
    WRITE("Rainbow Speed", rainbowSpeed)
}

static void to_json(json& j, const ColorToggle& o)
{
    const ColorToggle dummy;

    WRITE_BASE(Color)
    WRITE("Enabled", enabled)
}

static void to_json(json& j, const ColorToggleRounding& o)
{
    const ColorToggleRounding dummy;

    WRITE_BASE(ColorToggle)
    WRITE("Rounding", rounding)
}

static void to_json(json& j, const ColorToggleThickness& o)
{
    const ColorToggleThickness dummy;

    WRITE_BASE(ColorToggle)
    WRITE("Thickness", thickness)
}

static void to_json(json& j, const ColorToggleThicknessRounding& o)
{
    const ColorToggleThicknessRounding dummy;

    WRITE_BASE(ColorToggleRounding)
    WRITE("Thickness", thickness)
}

static void to_json(json& j, const Font& o)
{
    const Font dummy;

    WRITE("Name", name)
}

static void to_json(json& j, const Snapline& o)
{
    const Snapline dummy;

    WRITE_BASE(ColorToggleThickness)
    WRITE("Type", type)
}

static void to_json(json& j, const Box& o)
{
    const Box dummy;

    WRITE_BASE(ColorToggleThicknessRounding)
    WRITE("Type", type)
    WRITE("Scale", scale)
}

static void to_json(json& j, const Shared& o)
{
    const Shared dummy;

    WRITE("Enabled", enabled)
    WRITE("Font", font)
    WRITE("Snapline", snapline)
    WRITE("Box", box)
    WRITE("Name", name)
    WRITE("Text Cull Distance", textCullDistance)
}

static void to_json(json& j, const Player& o)
{
    const Player dummy;

    WRITE_BASE(Shared)
    WRITE("Weapon", weapon)
    WRITE("Flash Duration", flashDuration)
    WRITE("Audible Only", audibleOnly)
    WRITE("Spotted Only", spottedOnly)
    WRITE("Skeleton", skeleton)
}

static void to_json(json& j, const Weapon& o)
{
    j = static_cast<Shared>(o);

    const Weapon dummy;

    WRITE("Ammo", ammo)
}

static void to_json(json& j, const Trail& o)
{
    j = static_cast<ColorToggleThickness>(o);

    const Trail dummy;

    WRITE("Type", type)
    WRITE("Time", time)
}

static void to_json(json& j, const Trails& o)
{
    const Trails dummy;

    WRITE("Enabled", enabled)
    WRITE("Local Player", localPlayer)
    WRITE("Allies", allies)
    WRITE("Enemies", enemies)
}

static void to_json(json& j, const Projectile& o)
{
    j = static_cast<Shared>(o);

    const Projectile dummy;

    WRITE("Trails", trails)
}

static void to_json(json& j, const ImVec2& o)
{
    const ImVec2 dummy;

    WRITE("X", x)
    WRITE("Y", y)
}

static void to_json(json& j, const PurchaseList& o)
{
    const PurchaseList dummy;

    WRITE("Enabled", enabled)
    WRITE("Only During Freeze Time", onlyDuringFreezeTime)
    WRITE("Show Prices", showPrices)
    WRITE("No Title Bar", noTitleBar)
    WRITE("Mode", mode)

    if (const auto window = ImGui::FindWindowByName("Purchases")) {
        j["Pos"] = window->Pos;
        j["Size"] = window->SizeFull;
    }
}

static void to_json(json& j, const ObserverList& o)
{
    const ObserverList dummy;

    WRITE("Enabled", enabled)
    WRITE("No Title Bar", noTitleBar)

    if (const auto window = ImGui::FindWindowByName("Observer List")) {
        j["Pos"] = window->Pos;
        j["Size"] = window->SizeFull;
    }
}

template <typename T>
static void save_map(json& j, const char* name, const std::unordered_map<std::string, T>& map)
{
    const T dummy;

    for (const auto& [key, value] : map) {
        if (!(value == dummy))
            j[name][key] = value;
    }
}

void Config::save() noexcept
{
    json j;

    save_map(j, "Allies", allies);
    save_map(j, "Enemies", enemies);
    save_map(j, "Weapons", weapons);
    save_map(j, "Projectiles", projectiles);
    save_map(j, "Loot Crates", lootCrates);
    save_map(j, "Other Entities", otherEntities);

    if (!(reloadProgress == ColorToggleThickness{ 5.0f }))
        j["Reload Progress"] = reloadProgress;
    if (!(recoilCrosshair == ColorToggleThickness{}))
        j["Recoil Crosshair"] = recoilCrosshair;
    if (!(noscopeCrosshair == ColorToggleThickness{}))
        j["Noscope Crosshair"] = noscopeCrosshair;
    if (!(purchaseList == PurchaseList{}))
        j["Purchase List"] = purchaseList;
    if (!(observerList == ObserverList{}))
        j["Observer List"] = observerList;
    if (ignoreFlashbang)
        j["Ignore Flashbang"] = ignoreFlashbang;

    std::error_code ec; std::filesystem::create_directory(path, ec);

    if (std::ofstream out{ path / "config.txt" }; out.good())
        out << std::setw(2) << j;
}

void Config::scheduleFontLoad(const std::string& name) noexcept
{
    scheduledFonts.push_back(name);
}

#ifdef _WIN32
static auto getFontData(const std::string& fontName) noexcept
{
    HFONT font = CreateFontA(0, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, fontName.c_str());

    std::unique_ptr<std::byte[]> data;
    DWORD dataSize = GDI_ERROR;

    if (font) {
        HDC hdc = CreateCompatibleDC(nullptr);

        if (hdc) {
            SelectObject(hdc, font);
            dataSize = GetFontData(hdc, 0, 0, nullptr, 0);

            if (dataSize != GDI_ERROR) {
                data = std::make_unique<std::byte[]>(dataSize);
                dataSize = GetFontData(hdc, 0, 0, data.get(), dataSize);

                if (dataSize == GDI_ERROR)
                    data.reset();
            }
            DeleteDC(hdc);
        }
        DeleteObject(font);
    }
    return std::make_pair(std::move(data), dataSize);
}
#endif

bool Config::loadScheduledFonts() noexcept
{
    bool result = false;

    for (const auto& font : scheduledFonts) {
        if (font == "Default")
            continue;

#ifdef _WIN32
        const auto [fontData, fontDataSize] = getFontData(font);
        if (fontDataSize == GDI_ERROR)
            continue;

        static constexpr ImWchar ranges[]{ 0x0020, 0xFFFF, 0 };
        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;

        for (int i = 8; i <= 14; i += 2) {
            if (fonts.find(font + ' ' + std::to_string(i)) == fonts.cend()) {
                fonts[font + ' ' + std::to_string(i)] = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, static_cast<float>(i), &cfg, ranges);
                result = true;
            }
        }
#endif
    }
    scheduledFonts.clear();
    return result;
}
