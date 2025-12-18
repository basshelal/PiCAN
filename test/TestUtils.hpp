#pragma once

#define GTEST_DONT_DEFINE_TEST 1
#define GTEST_DONT_DEFINE_FAIL 1

#include <gtest/gtest.h>

#include "core/Types.hpp"

#define ASSERT_NULL(pointer) ASSERT_EQ(nullptr, pointer)
#define ASSERT_NOT_NULL(pointer) ASSERT_NE(nullptr, pointer)

#define ASSERT_EQUAL(val1, val2) ASSERT_EQ(val1, val2)
#define ASSERT_NOT_EQUAL(val1, val2) ASSERT_NE(val1, val2)

#define FAIL_TEST(Message) GTEST_FAIL() << (Message)
