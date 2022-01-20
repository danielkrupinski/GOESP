#pragma once

#include <cstddef>
#include <sys/mman.h>

class LinuxFileMap {
public:
    LinuxFileMap(void* address, std::size_t length, int protection, int flags, int fileDescriptor, off_t offset) : map{ mmap(address, length, protection, flags, fileDescriptor, offset) }, length{ length } {}

    [[nodiscard]] bool isValid() const noexcept
    {
        return map != MAP_FAILED;
    }

    [[nodiscard]] void* get() const noexcept
    {
        return map;
    }

    ~LinuxFileMap()
    {
        munmap(map, length);
    }

private:
    void* map = MAP_FAILED;
    std::size_t length = 0;
};
