#pragma once
#include <QObject>
#include "CanDriver.h"

/**
 * CanWorker — runs in a dedicated QThread.
 * Polls VCI_Receive every 5ms and emits frameReceived for each frame.
 * Connect to SdoClient::onFrameReceived via Qt::QueuedConnection.
 */
class CanWorker : public QObject {
    Q_OBJECT
public:
    explicit CanWorker(CanDriver* driver, QObject* parent = nullptr);

public slots:
    /** Call this slot after moving worker to thread: thread.start() → worker.start() */
    void start();
    void stop();

signals:
    void frameReceived(uint32_t cobId, QByteArray data);

private:
    CanDriver* driver_;
    bool       running_ = false;

    static constexpr int MAX_RX_FRAMES   = 64;
    static constexpr int SLEEP_EMPTY_MS  = 5;
};
