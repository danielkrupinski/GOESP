#pragma once

struct Vector;

class Matrix3x4 {
    float mat[3][4];
public:
    constexpr auto operator[](int i) const noexcept { return mat[i]; }

    constexpr auto origin() const noexcept;
};

#include "Vector.h"

constexpr auto Matrix3x4::origin() const noexcept
{
    return Vector{ mat[0][3], mat[1][3], mat[2][3] };
}
