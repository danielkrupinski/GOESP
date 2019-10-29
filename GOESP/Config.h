#pragma once

#include <filesystem>

class Config {
public:
    explicit Config(const char* folderName) noexcept;
private:
    std::filesystem::path path;
};
