#pragma once

#include "Inconstructible.h"
#include "VirtualMethod.h"

struct ConVar {
    INCONSTRUCTIBLE(ConVar)

    VIRTUAL_METHOD(float, getFloat, (IS_WIN32() ? 12 : 15), (), (this))
};
