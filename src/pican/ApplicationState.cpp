#include "pican/ApplicationState.hpp"

#include "pican/Contracts.hpp"

namespace pican {

ApplicationState* ApplicationState::instance_sf = nullptr;

ApplicationState::ApplicationState() : canInfo_f{} {
}

SimpleResult<ApplicationState::Error>
ApplicationState::initialize(mem::Block block) {
    CONTRACTS_ASSERT(block.size_bytes() >= sizeof(ApplicationState));
    if (ApplicationState::instance_sf != nullptr) {
        return SimpleResult<ApplicationState::Error>::failure_by_copy(ALREADY_INITIALIZED);
    }
    ApplicationState::instance_sf = new (block.address_to_ptr<ApplicationState>()) ApplicationState{};
    return SimpleResult<ApplicationState::Error>::success_default();
}

ApplicationState&
ApplicationState::get() {
    if (ApplicationState::instance_sf == nullptr) {
        pican::panic("ApplicationState not initialized!");
    }
    return *ApplicationState::instance_sf;
}
}  // namespace pican
