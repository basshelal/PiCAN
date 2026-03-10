#pragma once

#include <cstdint>

#include "pican/CopyableAtomic.hpp"
#include "pican/File.hpp"
#include "pican/IApplicationThread.hpp"
#include "pican/Result.hpp"
#include "pican/info/Info.hpp"

namespace pican::info {
class InfoThread : public IApplicationThread {
public:  // types
    enum class Error : std::uint8_t {
    };

private:  // fields
    Thread thread_f;
    CopyableAtomic<bool> isRunning_f;
    ThreadCounter threadCounter_f;
    Milliseconds sleepMillis_f;

    pican::File memInfoFile_f;
    pican::File selfStatusFile_f;

private:  // constructors
    InfoThread(Milliseconds sleepMillis);

public:   // lifetime
    InfoThread(const InfoThread& rhs) = delete;

    InfoThread(InfoThread&& rhs) noexcept = default;

    InfoThread&
    operator=(const InfoThread& rhs) & = delete;

    InfoThread&
    operator=(InfoThread&& rhs) & noexcept = default;

    ~InfoThread() override = default;

public:  // member functions
    // clang-format off
    virtual ThreadState
    start() & override;

    virtual ThreadState
    stop() & override;
    // clang-format on

    [[nodiscard]]
    virtual const ThreadIdentity&
    thread_identity() const& override;

    [[nodiscard]]
    virtual ThreadState
    thread_state() const& override;

    [[nodiscard]]
    virtual ThreadCounterValue
    thread_counter_value() const& override;

    [[nodiscard]]
    virtual const Thread&
    backing_thread() const& override;

private:  // member functions
public:   // static functions
    [[nodiscard]]
    static pican::Result<InfoThread*, Error>
    create();

private:  // static functions
    static void
    runnable(InfoThread* self);
};
}  // namespace pican::info
