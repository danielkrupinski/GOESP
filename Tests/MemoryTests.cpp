#include <gtest/gtest.h>

#include "../GOESP/Helpers.h"

class RelativeToAbsoluteTestFixture : public testing::TestWithParam<std::int32_t> {};

TEST_P(RelativeToAbsoluteTestFixture, CorrectAddressIsComputed) {
    const std::int32_t offset = GetParam();
    const std::uintptr_t addressOfOffset = reinterpret_cast<std::uintptr_t>(&offset);

    ASSERT_EQ(Helpers::relativeToAbsolute<std::uintptr_t>(addressOfOffset), addressOfOffset + offset + sizeof(offset));
}

INSTANTIATE_TEST_SUITE_P(RelativeToAbsoluteTest, RelativeToAbsoluteTestFixture, testing::Values(-1, 0, 1, 10'000));
