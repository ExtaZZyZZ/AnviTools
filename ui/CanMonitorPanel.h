#pragma once
#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

/**
 * CanMonitorPanel — CAN bus frame log with per-module color coding.
 *
 * Color scheme (by source node ID):
 *   Autonomous Module 0x02 → teal   #4ec9b0
 *   AnviDrive         0x07 → pink   #ff7eb6
 *   AnviEnergy        0x0A → blue   #5599ff
 *   NMT / system            → yellow #ffdd55
 *   unknown                 → gray   #888888
 *
 * Filter combo lets the user show frames from one module only.
 */
class CanMonitorPanel : public QWidget {
    Q_OBJECT
public:
    explicit CanMonitorPanel(QWidget* parent = nullptr);

public slots:
    /** Log an incoming (RX) CAN frame. */
    void appendFrame(uint32_t cobId, QByteArray data);
    /** Log an outgoing (TX) CAN frame. */
    void appendFrameTx(uint32_t cobId, QByteArray data);

private slots:
    void onClear();

private:
    // Returns the node ID that produced/targets this frame, or 0xFF for system frames.
    static uint8_t nodeIdFromCob(uint32_t cobId);

    // Returns a fixed-width display name for a node ID.
    static QString nameForNode(uint8_t nodeId);

    // Returns the color for a given node ID.
    static QColor colorForNode(uint8_t nodeId);

    // Returns a short frame-type tag string.
    static QString tagForCob(uint32_t cobId);

    // Returns true if this frame passes the current filter.
    bool passesFilter(uint8_t nodeId) const;

    void appendFormatted(const QString& dir, uint32_t cobId,
                         const QByteArray& data, uint8_t nodeId);

    QTextEdit*   log_;
    QPushButton* clearBtn_;
    QCheckBox*   autoScrollChk_;
    QComboBox*   filterBox_;

    static constexpr uint8_t NODE_AUTONOMOUS = 0x02;
    static constexpr uint8_t NODE_DRIVE      = 0x07;
    static constexpr uint8_t NODE_ENERGY     = 0x0A;
    static constexpr uint8_t NODE_SYSTEM     = 0xFF;
};
