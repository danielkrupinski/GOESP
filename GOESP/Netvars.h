#pragma once

#include <cstdint>
#include <unordered_map>

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
