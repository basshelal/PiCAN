#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QTimer>

class StopwatchBridge : public QObject {
    Q_OBJECT
    // Q_PROPERTY connects C++ variables to QML.
    // READ = getter function, NOTIFY = signal emitted when value changes.
    Q_PROPERTY(QString elapsedTimeString READ getElapsedTimeString NOTIFY timeChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY runningStateChanged)
    Q_PROPERTY(bool hasTime READ hasTime NOTIFY timeChanged)

public:
    explicit StopwatchBridge(QObject* parent = nullptr);

    // Getter functions used by Q_PROPERTY
    QString
    getElapsedTimeString() const;
    bool
    isRunning() const;
    bool
    hasTime() const;

    // Functions callable from QML buttons
    Q_INVOKABLE void
    togglePauseResume();
    Q_INVOKABLE void
    stopReset();

signals:
    // Signals to tell QML to update bindings
    void
    timeChanged();
    void
    runningStateChanged();

private slots:
    void
    updateTick();  // Called repeatedly by the m_uiTimer

private:
    // Helper to format ms to MM:SS:mmm
    QString
    formatTime(qint64 totalMs) const;

    QTimer m_uiTimer;              // The heartbeat that updates the UI regularly
    QElapsedTimer m_elapsedTimer;  // The high-precision timer for the current run
    qint64 m_accumulatedTimeMs;    // Time stored from previous runs before pausing
    bool m_isRunning;
};
