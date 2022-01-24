#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class LinuxFile {
public:
    LinuxFile(const char* path, int flags) : descriptor{ open(path, flags) } {}

    [[nodiscard]] bool isValid() const noexcept
    {
        return descriptor >= 0;
    }

    [[nodiscard]] int getDescriptor() const noexcept
    {
        return descriptor;
    }

    [[nodiscard]] struct stat getStatus() const
    {
        struct stat status;
        fstat(descriptor, &status);
        return status;
    }

    ~LinuxFile()
    {
        close(descriptor);
    }

private:
    int descriptor = -1;
};
