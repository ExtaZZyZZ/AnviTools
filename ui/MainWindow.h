#pragma once
#include <QMainWindow>
#include <QtCore/QString>
#include <QTabWidget>
#include <QThread>
#include "../can/CanDriver.h"
#include "../canopen/SdoClient.h"
#include "../serial/SerialMonitor.h"

class ConnectionTab;
class MonitorTab;
class ModuleHubTab;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    void onCanConnected();
    void onCanDisconnected();
    void onSerialConnected();
    void onSerialDisconnected();
    void onCanError(QString msg);

private:
    void setupTabs();
    void applyModernStyle();
    void updateTabStyle();

    QTabWidget*            tabs_               = nullptr;
    ConnectionTab*         connTab_            = nullptr;
    MonitorTab*            monitorTab_         = nullptr;
    ModuleHubTab*          moduleHubTab_       = nullptr;

    // Shared resources
    CanDriver*     canDriver_          = nullptr;
    SdoClient*     sdoClient_          = nullptr;
    SerialMonitor* serialMon_          = nullptr;
    QThread*       serialThread_       = nullptr;
};
