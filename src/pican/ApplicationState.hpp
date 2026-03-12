#pragma once

#include "pican/mem/Block.hpp"

#include "pican/Result.hpp"
#include "pican/TripleBuffer.hpp"
#include "pican/can/CanInfo.hpp"

namespace pican {
class ApplicationState {
public:  // types
    enum Error : std::uint8_t {
        ALREADY_INITIALIZED
    };

public:  // fields
    TripleBuffer<can::CanInfo> canInfo_f;

private:  // constructor
    ApplicationState();

private:  // static fields
    static ApplicationState* instance_sf;

public:  // lifetime
    ApplicationState(const ApplicationState& rhs) = delete;

    ApplicationState(ApplicationState&& rhs) noexcept = delete;

    ApplicationState&
    operator=(const ApplicationState& rhs) & = delete;

    ApplicationState&
    operator=(ApplicationState&& rhs) & noexcept = delete;

    ~ApplicationState() = default;

public:  // member functions
public:  // static functions
    static SimpleResult<Error>
    initialize(mem::Block block);

    [[nodiscard]]
    static ApplicationState&
    get();
};

}  // namespace pican
