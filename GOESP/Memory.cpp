#include "Memory.h"

Memory::Memory() noexcept
{
    present = findPattern<>(L"gameoverlayrenderer", "\xFF\x15????\x8B\xF8\x85\xDB", 2);
    reset = findPattern<>(L"gameoverlayrenderer", "\xC7\x45?????\xFF\x15????\x8B\xF8", 9);
}
