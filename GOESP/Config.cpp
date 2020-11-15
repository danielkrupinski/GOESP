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

Config::Config() noexcept
{

}
