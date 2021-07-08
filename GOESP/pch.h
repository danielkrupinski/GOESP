#pragma once

#include <intrin.h>

#ifdef _WIN32
#ifdef __cplusplus
#include <ShlObj.h>
#endif
#include <Windows.h>
#endif

#ifdef __cplusplus
#include <mutex>

#include "nlohmann/json.hpp"

#include "imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"
#endif
