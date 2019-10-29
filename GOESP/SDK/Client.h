#pragma once

#include "Utils.h"

struct ClientClass;

class Client {
public:
    constexpr auto getAllClasses() noexcept
    {
        return callVirtualMethod<ClientClass*>(this, 8);
    }
};
