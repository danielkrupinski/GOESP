#pragma once

#include "ClassId.h"

#include <type_traits>

class Entity;
struct RecvTable;

struct ClientClass {
#ifdef _WIN32
    std::add_pointer_t<Entity* __cdecl(int, int)> createFunction;
#else
    std::add_pointer_t<Entity*(int, int)> createFunction;
#endif
    void* createEventFunction;
    char* networkName;
    RecvTable* recvTable;
    ClientClass* next;
    ClassId classId;
};
