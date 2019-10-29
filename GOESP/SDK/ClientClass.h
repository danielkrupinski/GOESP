#pragma once

#include "ClassId.h"

#include <type_traits>

class Entity;
struct RecvTable;

struct ClientClass {
    std::add_pointer_t<Entity* __cdecl(int, int)> createFunction;
    void* createEventFunction;
    char* networkName;
    RecvTable* recvTable;
    ClientClass* next;
    ClassId classId;
};
