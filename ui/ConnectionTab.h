#pragma once
#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QTextEdit>
#include "../can/CanDriver.h"
#include "../serial/SerialMonitor.h"

class ConnectionTab : public QWidget {
    Q_OBJECT
public:
    explicit ConnectionTab(CanDriver* can, SerialMonitor* serial, QWidget* parent = nullptr);

signals:
    void canConnected();
    void canDisconnected();
    void serialConnected();
    void serialDisconnected();

private slots:
    void onCanRefresh();
    void onCanConnect();
    void onSerialRefresh();
    void onSerialConnect();
    void onRawBytes(QByteArray data);

private:
    void setCanConnected(bool connected);
    void setSerialConnected(bool connected);
    void loadSettings();
    void saveSettings();

    CanDriver*     canDriver_;
    SerialMonitor* serialMon_;
    bool           canOpen_    = false;
    bool           serialOpen_ = false;

    // CAN group (Waveshare USB-CAN-A via CH341 virtual COM)
    QComboBox*   canPortBox_;
    QPushButton* canRefreshBtn_;
    QComboBox*   canBaudBox_;      // Serial baud: 2000000 / 115200
    QPushButton* canConnectBtn_;
    QLabel*      canStatusLabel_;

    // UART2 debug monitor
    QComboBox*   serialPortBox_;
    QPushButton* serialRefreshBtn_;
    QPushButton* serialConnectBtn_;
    QLabel*      serialStatusLabel_;

    // Raw bytes diagnostic panel
    QTextEdit*   rawHexView_;
    int          rawByteCount_ = 0;
    static constexpr int MAX_RAW_BYTES = 128; // show first N bytes then stop
};
