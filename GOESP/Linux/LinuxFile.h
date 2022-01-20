#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "LinuxFileMap.h"

class LinuxFile {
public:
    LinuxFile(const char* path, int flags) : descriptor{ open(path, flags) } {}

    [[nodiscard]] bool isValid() const noexcept
    {
        return descriptor >= 0;
    }

    [[nodiscard]] struct stat getStatus() const
    {
        struct stat status;
        fstat(descriptor, &status);
        return status;
    }

    [[nodiscard]] LinuxFileMap map(void* address, std::size_t length, int protection, int flags, off_t offset) const
    {
        return LinuxFileMap{ address, length, protection, flags, descriptor, offset };
    }

    ~LinuxFile()
    {
        close(descriptor);
    }

private:
    int descriptor = -1;
};
