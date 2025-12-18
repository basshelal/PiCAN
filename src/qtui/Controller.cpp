#include "Controller.hpp"

#include <QDebug>

StopwatchBridge::StopwatchBridge(QObject* parent) : QObject(parent), m_accumulatedTimeMs(0), m_isRunning(false) {
    // Update the UI every ~16ms (approx 60 FPS)
    m_uiTimer.setInterval(16);
    connect(&m_uiTimer, &QTimer::timeout, this, &StopwatchBridge::updateTick);
}

bool
StopwatchBridge::isRunning() const {
    return m_isRunning;
}

bool
StopwatchBridge::hasTime() const {
    // Returns true if the stopwatch is not at 00:00:000
    return m_accumulatedTimeMs > 0 || (m_isRunning && m_elapsedTimer.isValid());
}

void
StopwatchBridge::togglePauseResume() {
    if (m_isRunning) {
        // Pause logic
        m_uiTimer.stop();
        // Save the time elapsed in the current session
        m_accumulatedTimeMs += m_elapsedTimer.elapsed();
        m_isRunning = false;
    } else {
        // Resume logic
        // Restart the elapsed timer for the new session
        m_elapsedTimer.restart();
        m_uiTimer.start();
        m_isRunning = true;
    }
    // Notify QML that state changed
    emit runningStateChanged();
    // Ensure time display is updated immediately upon pause
    emit timeChanged();
}

void
StopwatchBridge::stopReset() {
    if (m_isRunning) {
        // Stop action: Just pause it, don't clear time yet.
        togglePauseResume();
    } else {
        // Reset action: Clear everything back to zero.
        m_accumulatedTimeMs = 0;
        m_elapsedTimer.invalidate();
        // Notify QML to update the display to 00:00:000
        emit timeChanged();
    }
}

void
StopwatchBridge::updateTick() {
    // This slot is called repeatedly while running.
    // We only need to tell QML that time has changed.
    if (m_isRunning) {
        emit timeChanged();
    }
}

QString
StopwatchBridge::getElapsedTimeString() const {
    qint64 currentSessionMs = 0;
    if (m_isRunning && m_elapsedTimer.isValid()) {
        currentSessionMs = m_elapsedTimer.elapsed();
    }
    qint64 totalMs = m_accumulatedTimeMs + currentSessionMs;
    return formatTime(totalMs);
}

QString
StopwatchBridge::formatTime(qint64 totalMs) const {
    qint64 milliseconds = totalMs % 1'000;
    qint64 seconds = (totalMs / 1'000) % 60;
    qint64 minutes = (totalMs / 60'000);

    // Use standard QString formatting with zero padding
    // arg(value, fieldWidth, base, fillChar)
    return QString("%1:%2:%3")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(milliseconds, 3, 10, QChar('0'));
}
