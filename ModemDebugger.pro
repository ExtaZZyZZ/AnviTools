QT += core gui widgets serialport

TARGET   = AnviTools
TEMPLATE = app
CONFIG  += c++14

# Suppress deprecation warnings for Qt 5/6 compatibility
DEFINES += QT_DEPRECATED_WARNINGS

# vcpkg ships Qt release libraries only (no Qt5Widgetsd.lib etc.).
# qmake's QT+= mechanism auto-appends 'd' suffix in Debug builds — there is no
# workaround other than forcing a Release build when using vcpkg Qt on Windows.
win32 {
    CONFIG -= debug_and_release debug
    CONFIG += release

    VCPKG_ROOT = $$(VCPKG_ROOT)
    isEmpty(VCPKG_ROOT): VCPKG_ROOT = C:/Users/RGUser/vcpkg

    QT_INSTALL = $$VCPKG_ROOT/installed/x64-windows
    INCLUDEPATH += $$QT_INSTALL/include
    LIBS += -L$$QT_INSTALL/lib
    QMAKE_CXXFLAGS += /Zm200

    DESTDIR = $$OUT_PWD/AnviTools
}

INCLUDEPATH += can canopen serial ui model

HEADERS += \
    can/CanDriver.h           \
    canopen/SdoClient.h       \
    serial/SerialMonitor.h    \
    ui/MainWindow.h           \
    ui/ConnectionTab.h        \
    ui/MonitorTab.h           \
    ui/ModuleHubTab.h         \
    ui/CanMonitorPanel.h      \
    ui/AutonomousModuleTab.h  \
    ui/AnviDriveTab.h         \
    ui/AnviEnergyTab.h        \
    ui/ModulePlaceholderTab.h \
    ui/SensorTab.h            \
    ui/ControlTab.h           \
    ui/ConfigTab.h            \
    ui/FlashLogTab.h          \
    ui/SdoConsoleTab.h        \
    ui/FirmwareTab.h          \
    model/FlashLogEntry.h     \
    model/DeviceConfig.h

SOURCES += \
    main.cpp                    \
    can/CanDriver.cpp           \
    canopen/SdoClient.cpp       \
    serial/SerialMonitor.cpp    \
    ui/MainWindow.cpp           \
    ui/ConnectionTab.cpp        \
    ui/MonitorTab.cpp           \
    ui/ModuleHubTab.cpp         \
    ui/CanMonitorPanel.cpp      \
    ui/AutonomousModuleTab.cpp  \
    ui/AnviDriveTab.cpp         \
    ui/AnviEnergyTab.cpp        \
    ui/ModulePlaceholderTab.cpp \
    ui/SensorTab.cpp            \
    ui/ControlTab.cpp           \
    ui/ConfigTab.cpp            \
    ui/FlashLogTab.cpp          \
    ui/SdoConsoleTab.cpp        \
    ui/FirmwareTab.cpp

RESOURCES += resources.qrc

win32:RC_ICONS = resources/app.ico

# Waveshare USB-CAN-A uses CH341 virtual COM port — no DLL needed.
# Install CH341SER.EXE driver, then select the COM port in the Connection tab.
