#pragma once
#include <cstdint>
#include <QWidget>

class QLabel;
class QTabWidget;
class QDoubleSpinBox;
class QPushButton;
class QTextEdit;
class CanDriver;
class SdoClient;
class FirmwareTab;

class AnviDriveTab : public QWidget {
    Q_OBJECT
public:
    explicit AnviDriveTab(CanDriver* can, SdoClient* sdo, QWidget* parent = nullptr);

    void setModuleConnected(bool connected);

public slots:
    void onCanFrame(uint32_t cobId, QByteArray data);

private slots:
    void onCmdOpen();
    void onCmdStop();
    void onCmdClose();
    void onCmdPercent();
    void onCmdCloseVT();
    void onCmdSetZero();
    void onCmdOpenVT();
    void onCmdCalcVT();

private:
    void log(const QString& msg);
    void setBusy(bool busy);

    static constexpr uint8_t  NODE_ID      = 0x07;
    static constexpr uint32_t COB_PERCENT   = 0x487;  // REAL32 percent (0x6001)
    static constexpr uint32_t COB_SPD_MOTOR = 0x287;  // INT16  speed_motor (0x6002)
    static constexpr uint32_t COB_SPD_DRIVE = 0x687;  // INT16  speed_electrodrive (0x6003)
    static constexpr uint32_t COB_TEMPS     = 0x187;  // 4×INT16 temp+humidity (0x6004-0x6007)

    SdoClient* sdo_;

    // Hero card
    QLabel*     canStateBadge_ = nullptr;
    QLabel*     canStateHint_  = nullptr;
    QTabWidget* toolTabs_      = nullptr;

    // Telemetry labels
    QLabel* percentLabel_       = nullptr;
    QLabel* speedMotorLabel_    = nullptr;
    QLabel* speedDriveLabel_    = nullptr;
    QLabel* tempElecLabel_      = nullptr;
    QLabel* tempMotorLabel_     = nullptr;
    QLabel* tempTransLabel_     = nullptr;
    QLabel* humidityLabel_      = nullptr;
    QLabel* lastUpdateLabel_    = nullptr;
    QLabel* pdoCountLabel_      = nullptr;

    // Control — card 1
    QPushButton*    openBtn_       = nullptr;
    QPushButton*    stopBtn_       = nullptr;
    QPushButton*    closeBtn_      = nullptr;
    QDoubleSpinBox* percentSpin_   = nullptr;
    QPushButton*    percentBtn_    = nullptr;

    // Control — card 2 (Valve Travel)
    QDoubleSpinBox* closeTorqueSpin_ = nullptr;
    QPushButton*    closeVTBtn_      = nullptr;
    QPushButton*    setZeroBtn_      = nullptr;
    QDoubleSpinBox* openTorqueSpin_  = nullptr;
    QPushButton*    openVTBtn_       = nullptr;
    QDoubleSpinBox* calcVTSpin_      = nullptr;
    QPushButton*    calcVTBtn_       = nullptr;

    QTextEdit*      cmdLog_        = nullptr;
    FirmwareTab*    firmwareTab_   = nullptr;

    int pdoCount_ = 0;
};
