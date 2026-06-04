#pragma once
#include <cstdint>
#include <QWidget>

class QLabel;
class QTabWidget;
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QTextEdit;
class QTimer;
class CanDriver;
class SdoClient;
class FirmwareTab;

class AnviEnergyTab : public QWidget {
    Q_OBJECT
public:
    explicit AnviEnergyTab(CanDriver* can, SdoClient* sdo, QWidget* parent = nullptr);

    void setModuleConnected(bool connected);

public slots:
    void onCanFrame(uint32_t cobId, QByteArray data);

private slots:
    void onSendConfig();
    void onReadConfig();
    void onSaveToFlash();
    void onSdoPoll();

private:
    void log(const QString& msg);
    void setBusy(bool busy);

    static constexpr uint8_t  NODE_ID   = 0x0A;
    static constexpr uint32_t COB_TEMPS = 0x28A;  // 3×INT16: tempElec + humElec + tempAkku
    static constexpr uint32_t COB_SOC   = 0x18A;  // INT32: SoC

    SdoClient* sdo_;
    QTimer*    pollTimer_ = nullptr;

    // Hero card
    QLabel*     canStateBadge_ = nullptr;
    QLabel*     canStateHint_  = nullptr;
    QTabWidget* toolTabs_      = nullptr;

    // Unified telemetry labels
    QLabel* tempElecLabel_   = nullptr;
    QLabel* humElecLabel_    = nullptr;
    QLabel* tempAkkuLabel_   = nullptr;
    QLabel* socLabel_        = nullptr;
    QLabel* ch1CurrentLabel_ = nullptr;  // 0x600F
    QLabel* ch2CurrentLabel_ = nullptr;  // 0x6010
    QLabel* battVoltLabel_   = nullptr;  // 0x6011
    QLabel* outVoltLabel_    = nullptr;  // 0x6012
    QLabel* moduleStateLabel_= nullptr;  // 0x6013
    QLabel* lastUpdateLabel_ = nullptr;
    QLabel* pdoCountLabel_   = nullptr;

    // Config inputs (SDO write)
    QDoubleSpinBox* maxOverVoltDischSpin_   = nullptr;  // 0x6000
    QDoubleSpinBox* maxUnderVoltDischSpin_  = nullptr;  // 0x6005
    QDoubleSpinBox* voltMaxChargeSpin_      = nullptr;  // 0x6006
    QDoubleSpinBox* maxUnderVoltChargeSpin_ = nullptr;  // 0x6007
    QSpinBox*       timeOfTurnOnSpin_       = nullptr;  // 0x600A (uint16)
    QDoubleSpinBox* diffCurrentSpin_        = nullptr;  // 0x600B
    QDoubleSpinBox* maxChargeCurrentSpin_   = nullptr;  // 0x6015
    QDoubleSpinBox* maxDischargeCurrentSpin_= nullptr;  // 0x6016
    QDoubleSpinBox* batteryCapacity_     = nullptr;  // 0x6024
    QPushButton*    readConfigBtn_           = nullptr;
    QPushButton*    sendConfigBtn_           = nullptr;
    QPushButton*    saveToFlashBtn_          = nullptr;
    QLabel*         flashStatusLabel_        = nullptr;
    QTimer*         flashStatusTimer_        = nullptr;
    QTextEdit*      cmdLog_                  = nullptr;

    // Calibration tab (0x6018 / 0x6019 / 0x6014)
    QDoubleSpinBox* packVoltCalSpin_         = nullptr;
    QDoubleSpinBox* outVoltCalSpin_          = nullptr;
    QPushButton*    calSaveBtn_              = nullptr;
    QLabel*         calStatusLabel_          = nullptr;
    QTimer*         calStatusTimer_          = nullptr;
    QPushButton*    calCurrentBtn_           = nullptr;
    QLabel*         calCurrentStatusLabel_   = nullptr;
    QTimer*         calCurrentStatusTimer_   = nullptr;
    QTextEdit*      calLog_                  = nullptr;

    FirmwareTab*    firmwareTab_             = nullptr;

    int  pdoCount_    = 0;
    bool sdoBusy_     = false;
    bool connected_   = false;
};
