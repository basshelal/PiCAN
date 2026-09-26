#pragma once

#include "test/utils/Tracked.hpp"
#include "test/utils/functions.hpp"

// Only movable, to check that Result's copy operations disappear when a stored type can't be copied
struct MoveOnly {
    MoveOnly() = default;
    MoveOnly(const MoveOnly& rhs) = delete;
    MoveOnly(MoveOnly&& rhs) noexcept = default;
    MoveOnly&
    operator=(const MoveOnly& rhs) = delete;
    MoveOnly&
    operator=(MoveOnly&& rhs) noexcept = default;
    ~MoveOnly() = default;
};

// Movable but not noexcept, to check that Result's noexcept follows the stored types' noexcept
struct PotentiallyThrowingMove {
    PotentiallyThrowingMove() = default;
    PotentiallyThrowingMove(const PotentiallyThrowingMove& rhs) = default;

    PotentiallyThrowingMove(PotentiallyThrowingMove&& rhs) noexcept(false) {
    }

    PotentiallyThrowingMove&
    operator=(const PotentiallyThrowingMove& rhs) = default;

    PotentiallyThrowingMove&
    operator=(PotentiallyThrowingMove&& rhs) noexcept(false) {
        return *this;
    }

    ~PotentiallyThrowingMove() = default;
};
