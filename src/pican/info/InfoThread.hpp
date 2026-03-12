#pragma once

#include <cstdint>

#include "pican/CopyableAtomic.hpp"
#include "pican/File.hpp"
#include "pican/IApplicationThread.hpp"
#include "pican/Result.hpp"
#include "pican/info/Info.hpp"
#include "pican/EventFD.hpp"

namespace pican::info {
class InfoThread : public IApplicationThread {
public:  // types
    enum class Error : std::uint8_t {
        FAILED_TO_OPEN_FILE,
        FILE_READ_ERROR,
    };

private:  // fields
    Thread thread_f;
    CopyableAtomic<bool> isRunning_f;
    ThreadCounter counter_f;
    Milliseconds sleepMillis_f;
    mem::Block fileBuffer_f;
    pican::File memInfoFile_f;
    pican::File statusFile_f;
    pican::EventFD stopFd_f;

private:  // constructors
    InfoThread(Milliseconds sleepMillis, mem::Block fileBuffer,
        pican::File&& meminfoFile, pican::File&& statusFile);

public:  // lifetime
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
    virtual ThreadCounterValue
    counter_value() const& override;

    [[nodiscard]]
    virtual const Thread&
    backing_thread() const& override;

private:  // member functions
    SimpleResult<InfoThread::Error>
    read_meminfo_file() &;

    SimpleResult<InfoThread::Error>
    read_status_file() &;
public:   // static functions
    [[nodiscard]]
    static pican::Result<InfoThread*, Error>
    create(mem::Block block, mem::Block fileBuffer,Milliseconds sleepMillis);

private:  // static functions
    static void
    runnable(InfoThread* self);
};
}  // namespace pican::info
