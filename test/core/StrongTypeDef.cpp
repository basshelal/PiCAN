#include <cstdint>

#include <catch2/catch_all.hpp>

import pican.core;

using pican::StrongTypeDef;
using ValueType = uint64_t;

struct Tag {};

using TypeDef = StrongTypeDef<ValueType, Tag>;

TEST_CASE("StrongTypeDef") {
    CHECK(sizeof(TypeDef) == sizeof(ValueType));
    SECTION("Owns Value on const") {
        const ValueType value = 69'420;
        const TypeDef def{value};
        const uint64_t& gotValue = def.get();
        const auto* typedefAddress = reinterpret_cast<const std::byte*>(std::addressof(def));
        const auto* valueAddress = reinterpret_cast<const std::byte*>(std::addressof(gotValue));
        CHECK(typedefAddress == valueAddress);
    }
    SECTION("Owns Value on non const") {
        ValueType value = 69'420;
        TypeDef const_typedef{value};
        uint64_t& gotValue = const_typedef.get();
        auto* typedefAddress = reinterpret_cast<std::byte*>(std::addressof(const_typedef));
        auto* valueAddress = reinterpret_cast<std::byte*>(std::addressof(gotValue));
        CHECK(typedefAddress == valueAddress);
    }
}
