#pragma once

#include "Inconstructible.h"
#include "VirtualMethod.h"

class InputSystem {
public:
    INCONSTRUCTIBLE(InputSystem)
#ifdef _WIN32
    VIRTUAL_METHOD(void, enableInput, 11, (bool enable), (this, enable))
#endif
    VIRTUAL_METHOD(void, resetInputState, 39, (), (this))
};
