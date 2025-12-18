#pragma once

#include <atomic>
#include <sstream>

#include <magic_enum/magic_enum.hpp>

enum class LifetimeOperation : uint8_t {
    CONSTRUCTOR,
    COPY_CONSTRUCTOR,
    MOVE_CONSTRUCTOR,
    COPY_ASSIGNMENT,
    MOVE_ASSIGNMENT,
};

template<typename TP>
class Tracked {
public:  // member fields
    TP data;
    std::atomic_int32_t copyCount = 0;
    std::atomic_int32_t moveCount = 0;
    LifetimeOperation lastOperation = LifetimeOperation::CONSTRUCTOR;

public:  // constructors
    inline Tracked(TP data) : data(std::move(data)) {
    }

    inline Tracked() : Tracked(TP{}) {
    }

public:  // copy-control
    inline Tracked(const Tracked& rhs) {
        this->data = rhs.data;
        this->copyCount++;
        this->lastOperation = LifetimeOperation::COPY_CONSTRUCTOR;
    }

    inline Tracked(Tracked&& rhs) noexcept {
        this->data = std::move(rhs.data);
        this->moveCount++;
        this->lastOperation = LifetimeOperation::MOVE_CONSTRUCTOR;
    }

    inline Tracked&
    operator=(const Tracked& rhs) {
        this->data = rhs.data;
        this->copyCount++;
        this->lastOperation = LifetimeOperation::COPY_ASSIGNMENT;
        return *this;
    }

    inline Tracked&
    operator=(Tracked&& rhs) noexcept {
        this->data = std::move(rhs.data);
        this->moveCount++;
        this->lastOperation = LifetimeOperation::MOVE_ASSIGNMENT;
        return *this;
    }

    ~Tracked() = default;

public:  // member functions
    inline const Tracked*
    address() const {
        return this;
    }

    inline bool
    operator==(const Tracked& rhs) const {
        return this->data == rhs.data && this->copyCount == rhs.copyCount && this->moveCount == rhs.moveCount &&
               this->lastOperation == rhs.lastOperation;
    }

public:  // friends
    inline friend std::string
    to_string(const Tracked& tracked) {
        std::stringstream stream;
        stream << "Tracked { " << "data = \"" << tracked.data << "\", copyCount = " << tracked.copyCount
               << ", moveCount = " << tracked.moveCount
               << ", lastOperation = " << magic_enum::enum_name(tracked.lastOperation) << " }";
        return stream.str();
    }

    inline friend std::ostream&
    operator<<(std::ostream& stream, const Tracked& tracked) {
        return stream << to_string(tracked);
    }
};
