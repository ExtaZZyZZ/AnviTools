#pragma once
#include <cstdint>
#include <QByteArray>
#include <QString>
#include <QWidget>

class QTabWidget;
class QLabel;
class QPushButton;
class QTimer;
class AutonomousModuleTab;
class AnviDriveTab;
class AnviEnergyTab;
class CanDriver;
class SdoClient;
class CanMonitorPanel;

class ModuleHubTab : public QWidget {
    Q_OBJECT
public:
    explicit ModuleHubTab(CanDriver* can, SdoClient* sdo, QWidget* parent = nullptr);

    void setCanConnected(bool connected);

private slots:
    void onScanRequested();
    void onScanFinished();
    void onCanFrame(uint32_t cobId, QByteArray data);

private:
    void resetModuleStates();
    void updateModuleViews();
    void updateSummaryRow(QLabel* badge,
                          QLabel* details,
                          uint8_t nodeId,
                          bool connected,
                          uint8_t nmtState);
    void setModuleState(uint8_t nodeId, bool connected, uint8_t nmtState);
    bool sendNmtStart(uint8_t nodeId);
    bool allKnownModulesDetected() const;
    QString nmtStateText(uint8_t nmtState) const;
    QString formatNodeId(uint8_t nodeId) const;

    CanDriver*             can_             = nullptr;
    QTabWidget*            moduleTabs_      = nullptr;
    AutonomousModuleTab*   autonomousTab_   = nullptr;
    AnviDriveTab*          anviDriveTab_    = nullptr;
    AnviEnergyTab*         anviEnergyTab_   = nullptr;
    CanMonitorPanel*       canMonitorPanel_ = nullptr;
    QPushButton*           scanButton_      = nullptr;
    QLabel*                scanStatusLabel_ = nullptr;
    QLabel*                autonomousBadge_ = nullptr;
    QLabel*                autonomousInfo_  = nullptr;
    QLabel*                driveBadge_      = nullptr;
    QLabel*                driveInfo_       = nullptr;
    QLabel*                energyBadge_     = nullptr;
    QLabel*                energyInfo_      = nullptr;
    QTimer*                scanTimer_       = nullptr;
    bool                   canConnected_    = false;
    bool                   scanInProgress_  = false;
    bool                   autonomousSeen_  = false;
    bool                   driveSeen_       = false;
    bool                   energySeen_      = false;
    uint8_t                autonomousState_ = 0xFF;
    uint8_t                driveState_      = 0xFF;
    uint8_t                energyState_     = 0xFF;

    static constexpr uint8_t AUTONOMOUS_NODE_ID = 0x02;
    static constexpr uint8_t DRIVE_NODE_ID      = 0x07;
    static constexpr uint8_t ENERGY_NODE_ID     = 0x0A;
};
