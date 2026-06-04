#include "CanWorker.h"
#include <QThread>

CanWorker::CanWorker(CanDriver* driver, QObject* parent)
    : QObject(parent), driver_(driver)
{}

void CanWorker::start()
{
    running_ = true;
    VCI_CAN_OBJ frames[MAX_RX_FRAMES];

    while (running_) {
        int n = driver_->receive(frames, MAX_RX_FRAMES);
        if (n > 0) {
            for (int i = 0; i < n; ++i) {
                QByteArray data(reinterpret_cast<const char*>(frames[i].Data),
                                static_cast<int>(frames[i].DataLen));
                emit frameReceived(frames[i].ID, data);
            }
        } else {
            QThread::msleep(SLEEP_EMPTY_MS);
        }
    }
}

void CanWorker::stop()
{
    running_ = false;
}
