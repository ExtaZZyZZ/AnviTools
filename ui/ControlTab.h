#pragma once
#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include "../can/CanDriver.h"
#include "../canopen/SdoClient.h"

class ControlTab : public QWidget {
    Q_OBJECT
public:
    explicit ControlTab(CanDriver* can, SdoClient* sdo, QWidget* parent = nullptr);

private slots:
    void onNmtStart();
    void onAllowSleep();
    void onSpiSwitch();
    void onSpiAcquire();
    void onEraseFlash();
    void onPingEnable();
    void onPingDisable();

private:
    void log(const QString& msg);
    void setBusy(bool busy);

    CanDriver*   can_;
    SdoClient*   sdo_;
    QPushButton* nmtStartBtn_;
    QPushButton* allowSleepBtn_;
    QPushButton* spiSwitchBtn_;
    QPushButton* spiAcquireBtn_;
    QPushButton* eraseFlashBtn_;
    QPushButton* pingEnableBtn_;
    QPushButton* pingDisableBtn_;
    QTextEdit*   logView_;

    static constexpr uint8_t  NODE_ID        = 0x02;
    static constexpr uint16_t OD_ALLOW_SLEEP = 0x6000;
    static constexpr uint16_t OD_SPI_SWITCH  = 0x6005;
    static constexpr uint16_t OD_ERASE_FLASH = 0x6009;
    static constexpr uint16_t OD_PING_MODE   = 0x600C;
    static constexpr uint32_t NMT_COB_ID     = 0x000;
};
