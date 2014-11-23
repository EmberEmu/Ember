#include <gtest/gtest.h>

// A test that's expected to fail.
TEST(Foo, Bar) {
	EXPECT_EQ(2, 3);
}