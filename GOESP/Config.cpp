#include <fstream>
#include <memory>

#ifdef _WIN32
#include <ShlObj.h>
#include <Windows.h>
#endif

#include "Config.h"
#include "Helpers.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "nlohmann/json.hpp"

#include "Hacks/ESP.h"
#include "Hacks/Misc.h"

#ifdef _WIN32
int CALLBACK fontCallback(const LOGFONTW* lpelfe, const TEXTMETRICW*, DWORD, LPARAM lParam)
{
    const wchar_t* const fontName = reinterpret_cast<const ENUMLOGFONTEXW*>(lpelfe)->elfFullName;

    if (fontName[0] == L'@')
        return TRUE;

    if (HFONT font = CreateFontW(0, 0, 0, 0,
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

        if (fontData == GDI_ERROR) {
            if (char buff[1024]; WideCharToMultiByte(CP_UTF8, 0, fontName, -1, buff, sizeof(buff), nullptr, nullptr))
                reinterpret_cast<std::vector<std::string>*>(lParam)->emplace_back(buff);
        }
    }
    return TRUE;
}
#endif

Config::Config(const char* folderName) noexcept
{
#ifdef _WIN32
    if (PWSTR pathToDocuments; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &pathToDocuments))) {
        path = pathToDocuments;
        CoTaskMemFree(pathToDocuments);
    }

    LOGFONTW logfont;
    logfont.lfCharSet = ANSI_CHARSET;
    logfont.lfPitchAndFamily = DEFAULT_PITCH;
    logfont.lfFaceName[0] = L'\0';

    EnumFontFamiliesExW(GetDC(nullptr), &logfont, fontCallback, (LPARAM)&systemFonts, 0);
#elif __linux__
    if (const char* homeDir = getenv("HOME"))
        path = homeDir;

    if (auto pipe = popen("fc-list :lang=en -f \"%{family[0]} %{style[0]} %{file}\\n\" | grep .ttf", "r")) {
        char* line = nullptr;
        std::size_t n = 0;
        while (getline(&line, &n, pipe) != -1) {
            auto path = strstr(line, "/");
            if (path <= line)
                continue;
           
            path[-1] = path[strlen(path) - 1] = '\0';
            systemFonts.emplace_back(line);
            systemFontPaths.emplace_back(path);
        }
        if (line)
            free(line);
        pclose(pipe);
    }
#endif
    path /= folderName;
    std::sort(std::next(systemFonts.begin()), systemFonts.end());
}

static void from_json(const json& j, Font& f)
{
    read<value_t::string>(j, "Name", f.name);

    if (const auto it = std::find_if(config->getSystemFonts().begin(), config->getSystemFonts().end(), [&f](const auto& e) { return e == f.name; }); it != config->getSystemFonts().end()) {
        f.index = std::distance(config->getSystemFonts().begin(), it);
        config->scheduleFontLoad(f.index);
    } else {
        f.index = 0;
    }
}

static void from_json(const json& j, Shared& s)
{
    read(j, "Enabled", s.enabled);
    read<value_t::object>(j, "Font", s.font);
    read<value_t::object>(j, "Snapline", s.snapline);
    read<value_t::object>(j, "Box", s.box);
    read<value_t::object>(j, "Name", s.name);
    read_number(j, "Text Cull Distance", s.textCullDistance);
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
    read(j, "Audible Only", p.audibleOnly);
    read(j, "Spotted Only", p.spottedOnly);
    read<value_t::object>(j, "Skeleton", p.skeleton);
    read<value_t::object>(j, "Head Box", p.headBox);
    read(j, "Health Bar", p.healthBar);
}

static void from_json(const json& j, Weapon& w)
{
    from_json(j, static_cast<Shared&>(w));

    read<value_t::object>(j, "Ammo", w.ammo);
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

    Misc::fromJSON(j["Misc"]);
}

void removeEmptyObjects(json& j) noexcept
{
    for (auto it = j.begin(); it != j.end();) {
        auto& val = it.value();
        if (val.is_object())
            removeEmptyObjects(val);
        if (val.empty())
            it = j.erase(it);
        else
            ++it;
    }
}

void Config::save() noexcept
{
    json j;

    j["ESP"] = ESP::toJSON();
    j["Misc"] = Misc::toJSON();

    removeEmptyObjects(j);

    std::error_code ec; std::filesystem::create_directory(path, ec);

    if (std::ofstream out{ path / "config.txt" }; out.good())
        out << std::setw(2) << j;
}

void Config::scheduleFontLoad(std::size_t index) noexcept
{
    scheduledFonts.push_back(index);
}

static auto getFontData(const std::string& fontName) noexcept
{
#ifdef _WIN32
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
#else
    std::size_t dataSize = (std::size_t)-1;
    auto data = (std::byte*)ImFileLoadToMemory(fontName.c_str(), "rb", &dataSize);
    return std::make_pair(std::unique_ptr<std::byte[]>{ data }, dataSize);
#endif

}

bool Config::loadScheduledFonts() noexcept
{
    bool result = false;

    for (const auto fontIndex : scheduledFonts) {
        const auto& fontName = systemFonts[fontIndex];

        if (fonts.contains(fontName))
            continue;

        ImFontConfig cfg;
        Font newFont;

        if (fontName == "Default") {
            cfg.SizePixels = 13.0f;
            newFont.big = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

            cfg.SizePixels = 10.0f;
            newFont.medium = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

            cfg.SizePixels = 8.0f;
            newFont.tiny = ImGui::GetIO().Fonts->AddFontDefault(&cfg);

            fonts.emplace(fontName, newFont);
        } else {
#ifdef _WIN32
            const auto& fontPath = fontName;
#else
            const auto& fontPath = systemFontPaths[fontIndex];
#endif
            const auto [fontData, fontDataSize] = getFontData(fontPath);
            if (fontDataSize == -1)
                continue;

            cfg.FontDataOwnedByAtlas = false;
            const auto ranges = Helpers::getFontGlyphRanges();

            newFont.tiny = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 8.0f, &cfg, ranges);
            newFont.medium = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 10.0f, &cfg, ranges);
            newFont.big = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(fontData.get(), fontDataSize, 13.0f, &cfg, ranges);
            fonts.emplace(fontName, newFont);
        }
        result = true;
    }
    scheduledFonts.clear();
    return result;
}
