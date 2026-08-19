#pragma once

#include <atomic>
#include <memory>
#include <sstream>

#include <magic_enum/magic_enum.hpp>

enum class LifetimeOperation : uint8_t {
    NONE,
    CONSTRUCTOR,
    COPY_CONSTRUCTOR,
    MOVE_CONSTRUCTOR,
    COPY_ASSIGNMENT,
    MOVE_ASSIGNMENT,
};

template<typename TP>
class Tracked {
public:  // types
    using LifetimeCallback = std::function<void(const Tracked<TP>&)>;

    struct LifetimeCallbacks {
        LifetimeCallback onConstructor;
        LifetimeCallback onCopyConstructor;
        LifetimeCallback onMoveConstructor;
        LifetimeCallback onCopyAssignment;
        LifetimeCallback onMoveAssignment;
        LifetimeCallback onDestructor;
    };

public:  // constants
    static constexpr auto DEFAULT_LIFETIME_CALLBACK = [](const Tracked<TP>&) -> void {
    };

public:  // member fields
    TP data;
    std::atomic_int32_t copyCount;
    std::atomic_int32_t moveCount;
    LifetimeOperation lastOperation = LifetimeOperation::NONE;
    LifetimeCallbacks callbacks;

public:  // constructors
    explicit Tracked(
        TP data, LifetimeCallback onConstructor = DEFAULT_LIFETIME_CALLBACK,
        LifetimeCallback onCopyConstructor = DEFAULT_LIFETIME_CALLBACK,
        LifetimeCallback onMoveConstructor = DEFAULT_LIFETIME_CALLBACK,
        LifetimeCallback onCopyAssignment = DEFAULT_LIFETIME_CALLBACK,
        LifetimeCallback onMoveAssignment = DEFAULT_LIFETIME_CALLBACK,
        LifetimeCallback onDestructor = DEFAULT_LIFETIME_CALLBACK
    ) :
        data{std::move(data)}, copyCount{0}, moveCount{0}, lastOperation{LifetimeOperation::CONSTRUCTOR},
        callbacks{std::move(onConstructor),    std::move(onCopyConstructor), std::move(onMoveConstructor),
                  std::move(onCopyAssignment), std::move(onMoveAssignment),  std::move(onDestructor)} {
        this->callbacks.onConstructor(*this);
    }

    Tracked() : Tracked(TP{}) {
    }

public:  // copy-control
    Tracked(const Tracked& rhs) {
        this->data = rhs.data;
        this->copyCount = 1;
        this->moveCount = 0;
        this->lastOperation = LifetimeOperation::COPY_CONSTRUCTOR;
        this->callbacks = rhs.callbacks;
        this->callbacks.onCopyConstructor(*this);
    }

    Tracked(Tracked&& rhs) noexcept {
        this->data = std::move(rhs.data);
        this->copyCount = 0;
        this->moveCount = 1;
        this->lastOperation = LifetimeOperation::MOVE_CONSTRUCTOR;
        this->callbacks = rhs.callbacks;  // don't do std::move so that we can use destructor callback
        this->callbacks.onMoveConstructor(*this);
    }

    Tracked&
    operator=(const Tracked& rhs) & {
        this->data = rhs.data;
        this->copyCount++;
        this->lastOperation = LifetimeOperation::COPY_ASSIGNMENT;
        this->callbacks = rhs.callbacks;
        this->callbacks.onCopyAssignment(*this);
        return *this;
    }

    Tracked&
    operator=(Tracked&& rhs) & noexcept {
        this->data = std::move(rhs.data);
        this->moveCount++;
        this->lastOperation = LifetimeOperation::MOVE_ASSIGNMENT;
        this->callbacks = rhs.callbacks;  // don't do std::move so that we can use destructor callback
        this->callbacks.onMoveAssignment(*this);
        return *this;
    }

    ~Tracked() {
        this->callbacks.onDestructor(*this);  // safe because callbacks don't get std::move'd
    }

public:  // member functions
    const Tracked*
    address() const {
        return this;
    }

    bool
    operator==(const Tracked& rhs) const {
        return this->data == rhs.data && this->copyCount == rhs.copyCount && this->moveCount == rhs.moveCount &&
               this->lastOperation == rhs.lastOperation;
    }

public:  // friends
    friend std::string
    to_string(const Tracked& tracked) {
        std::stringstream stream;
        stream << "Tracked { " << "data = \"" << tracked.data << "\", copyCount = " << tracked.copyCount
               << ", moveCount = " << tracked.moveCount
               << ", lastOperation = " << magic_enum::enum_name(tracked.lastOperation) << " }";
        return stream.str();
    }

    friend std::ostream&
    operator<<(std::ostream& stream, const Tracked& tracked) {
        return stream << to_string(tracked);
    }
};
