#pragma once
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include "../canopen/SdoClient.h"

class SensorTab : public QWidget {
    Q_OBJECT
public:
    explicit SensorTab(SdoClient* sdo, QWidget* parent = nullptr);

private slots:
    void onPollToggle();
    void onPollTimer();

private:
    void updateSensor(uint8_t nodeId, uint16_t odIndex,
                      QLabel* valueLabel, QProgressBar* bar, QLabel* statusLabel);
    void setControlsEnabled(bool en);

    SdoClient*   sdo_;
    QTimer*      pollTimer_;

    QPushButton* pollToggleBtn_;
    QSpinBox*    intervalSpin_;

    // Sensor 1 (STM32 OD 0x6001 — updated by PDO from sensor node)
    QLabel*      s1ValueLabel_;
    QProgressBar* s1Bar_;
    QLabel*      s1StatusLabel_;

    // Sensor 2 (STM32 OD 0x6002 — updated by PDO from sensor node)
    QLabel*      s2ValueLabel_;
    QProgressBar* s2Bar_;
    QLabel*      s2StatusLabel_;

    static constexpr uint8_t  S1_NODE  = 0x0A; // STM32 node (caches sensor PDO)
    static constexpr uint16_t S1_INDEX = 0x6001;
    static constexpr uint8_t  S2_NODE  = 0x0A;
    static constexpr uint16_t S2_INDEX = 0x6002;
    static constexpr int      MAX_KPA  = 1000;
};
