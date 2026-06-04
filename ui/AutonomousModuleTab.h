#pragma once
#include <QWidget>

class QLabel;
class QTabWidget;
class CanDriver;
class SdoClient;
class ControlTab;
class ConfigTab;
class FirmwareTab;

class AutonomousModuleTab : public QWidget {
    Q_OBJECT
public:
    explicit AutonomousModuleTab(CanDriver* can, SdoClient* sdo, QWidget* parent = nullptr);

    void setModuleConnected(bool connected);

private:
    QLabel*       canStateBadge_ = nullptr;
    QLabel*       canStateHint_  = nullptr;
    QTabWidget*   toolTabs_      = nullptr;

    ControlTab*   controlTab_    = nullptr;
    ConfigTab*    configTab_     = nullptr;
    FirmwareTab*  firmwareTab_   = nullptr;
};
