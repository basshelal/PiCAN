#pragma once

#include <cassert>

#define INLINE_FLATTEN __attribute((always_inline, flatten))
#define STATIC_FAIL(Message) static_assert(false, Message)
#define PREPROCESSOR_BLOCK(...) \
    do {                        \
        __VA_ARGS__             \
    } while (false)

#define TODO(Message) pican::todo(Message)

#define TODO_NOT_IMPLEMENTED() pican::todo("Not Implemented")

#define SANITY_CHECK(condition) assert(condition)

