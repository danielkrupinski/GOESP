#pragma once

#include <cstdint>
#include <unordered_map>

#include "fnv.h"
#include "Interfaces.h"
#include "SDK/Client.h"
#include "SDK/ClientClass.h"

struct RecvTable;

class Netvars {
public:
    auto operator[](const uint32_t hash) noexcept
    {
        if (!initialized) {
            for (auto clientClass = interfaces.client->getAllClasses(); clientClass; clientClass = clientClass->next)
                traverseTable(clientClass->networkName, clientClass->recvTable);

            initialized = true;
        }
        return offsets[hash];
    }
private:
    bool initialized = false;
    void traverseTable(const char*, RecvTable*, const size_t = 0) noexcept;
    std::unordered_map<uint32_t, uint16_t> offsets;
};

extern Netvars netvars;

#define PNETVAR_OFFSET(funcname, class_name, var_name, offset, type) \
auto funcname() noexcept \
{ \
    constexpr auto hash = fnv::hash(class_name "->" var_name); \
    static auto netvarOffset = netvars[hash]; \
	return reinterpret_cast<std::add_pointer_t<type>>(this + netvarOffset + offset); \
}

#define PNETVAR(funcname, class_name, var_name, type) \
	PNETVAR_OFFSET(funcname, class_name, var_name, 0, type)

#define NETVAR_OFFSET(funcname, class_name, var_name, offset, type) \
std::add_lvalue_reference_t<type> funcname() noexcept \
{ \
    constexpr auto hash = fnv::hash(class_name "->" var_name); \
    static auto netvarOffset = netvars[hash]; \
	return *reinterpret_cast<std::add_pointer_t<type>>(this + netvarOffset + offset); \
}

#define NETVAR(funcname, class_name, var_name, type) \
	NETVAR_OFFSET(funcname, class_name, var_name, 0, type)
