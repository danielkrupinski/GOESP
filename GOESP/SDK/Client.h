#pragma once

#include "VirtualMethod.h"

struct ClientClass;

class Client {
public:
    VIRTUAL_METHOD(getAllClasses, ClientClass*, 8)
};
