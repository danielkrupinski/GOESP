#include "Config.h"

#include "imgui/imgui.h"

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
            for (DWORD i = 0; i < values; i++) {
                CHAR fontName[200], fontFilename[50];
                DWORD fontNameLength = 200, fontFilenameLength = 50;

                if (RegEnumValueA(key, i, fontName, &fontNameLength, nullptr, nullptr, PBYTE(fontFilename), &fontFilenameLength) == ERROR_SUCCESS)
                    systemFonts.push_back(std::make_pair(fontName, fontFilename));
            }
        }
        RegCloseKey(key);
    }

    if (PWSTR pathToFonts; SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &pathToFonts))) {
        fontsPath = pathToFonts;
        CoTaskMemFree(pathToFonts);
    }
}

void Config::scheduleFontLoad(const std::string& name) noexcept
{
    scheduledFonts.push_back(name);
}

bool Config::loadScheduledFonts() noexcept
{
    bool result = false;

    for (const auto& font : scheduledFonts) {
        if (!fonts[font]) {
            fonts[font] = ImGui::GetIO().Fonts->AddFontFromFileTTF((fontsPath / font).string().c_str(), 30.0f);
            result = true;
        }
    }
    scheduledFonts.clear();
    return result;
}
