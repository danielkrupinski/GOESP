#include "Netvars.h"

#include "fnv.h"
#include "SDK/Recv.h"

#include <cctype>

void Netvars::traverseTable(const char* networkName, RecvTable* recvTable, const std::size_t offset) noexcept
{
    for (int i = 0; i < recvTable->propCount; i++) {
        auto& prop = recvTable->props[i];

        if (std::isdigit(prop.name[0]) || fnv::hashRuntime(prop.name) == fnv::hash("baseclass"))
            continue;

        if (prop.type == 6 && prop.dataTable && prop.dataTable->netTableName[0] == 'D')
            traverseTable(networkName, prop.dataTable, prop.offset + offset);

        const auto hash = fnv::hashRuntime((networkName + std::string{ "->" } + prop.name).c_str());

        offsets[hash] = uint16_t(offset + prop.offset);
    }
}
