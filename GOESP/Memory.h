#pragma once

#include <cstdint>
#include <Windows.h>
#include <Psapi.h>
#include <sstream>

class Memory {
public:
    Memory() noexcept;

    uintptr_t present;
    uintptr_t reset;

private:
    template <typename T = uintptr_t>
    static auto findPattern(const wchar_t* module, const char* pattern, size_t offset = 0) noexcept
    {
        if (MODULEINFO moduleInfo; GetModuleInformation(GetCurrentProcess(), GetModuleHandleW(module), &moduleInfo, sizeof(moduleInfo))) {
            auto start{ static_cast<const char*>(moduleInfo.lpBaseOfDll) };
            auto end{ start + moduleInfo.SizeOfImage };

            auto first{ start };
            auto second{ pattern };

            while (first < end && *second) {
                if (*first == *second || *second == '?') {
                    first++;
                    second++;
                } else {
                    first = ++start;
                    second = pattern;
                }
            }

            if (!*second)
                return reinterpret_cast<T>(const_cast<char*>(start) + offset);
        }
        MessageBoxA(NULL, (std::ostringstream{ } << "Failed to find pattern in " << module << '!').str().c_str(), "GOESP", MB_OK | MB_ICONERROR);
        std::exit(EXIT_FAILURE);
    }
};
