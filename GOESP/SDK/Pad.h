#pragma once

#include <cstddef>

#define PAD(size) \
private: \
    [[maybe_unused]] std::byte _pad_##size[size]; \
public:
