#include "CanMonitorPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QFont>
#include <QLabel>

CanMonitorPanel::CanMonitorPanel(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    // ── Toolbar ──────────────────────────────────────────────────────────────
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);

    auto* title = new QLabel(QString::fromUtf8("CAN монитор"), this);
    title->setStyleSheet("font-weight: 600; font-size: 10pt; color: #4A1A6E;");

    autoScrollChk_ = new QCheckBox(QString::fromUtf8("Прокрутка"), this);
    autoScrollChk_->setChecked(true);

    clearBtn_ = new QPushButton(QString::fromUtf8("Очистить"), this);
    clearBtn_->setFixedWidth(100);

    auto* filterLabel = new QLabel(QString::fromUtf8("Фильтр:"), this);
    filterBox_ = new QComboBox(this);
    filterBox_->addItem(QString::fromUtf8("Все модули"),             static_cast<int>(-1));
    filterBox_->addItem(QString::fromUtf8("Автономный (0x02)"),      static_cast<int>(NODE_AUTONOMOUS));
    filterBox_->addItem(QString::fromUtf8("AnviDrive (0x07)"),       static_cast<int>(NODE_DRIVE));
    filterBox_->addItem(QString::fromUtf8("AnviEnergy (0x0A)"),      static_cast<int>(NODE_ENERGY));

    toolbar->addWidget(title);
    toolbar->addStretch();
    toolbar->addWidget(autoScrollChk_);
    toolbar->addWidget(clearBtn_);
    toolbar->addWidget(filterLabel);
    toolbar->addWidget(filterBox_);
    layout->addLayout(toolbar);

    // ── Color legend ─────────────────────────────────────────────────────────
    auto* legend = new QHBoxLayout();
    legend->setSpacing(12);
    legend->setContentsMargins(2, 0, 2, 2);

    auto makeDot = [&](const QString& color, const QString& label) {
        auto* lbl = new QLabel(
            QString::fromUtf8("● ") + label, this);
        lbl->setStyleSheet(
            QString("color: %1; font-size: 9pt; font-family: 'Courier New';").arg(color));
        legend->addWidget(lbl);
    };
    makeDot("#4ec9b0", QString::fromUtf8("Modem 0x02"));
    makeDot("#ff7eb6", QString::fromUtf8("Drive 0x07"));
    makeDot("#5599ff", QString::fromUtf8("Energy 0x0A"));
    makeDot("#ffdd55", QString::fromUtf8("System / NMT"));
    makeDot("#888888", QString::fromUtf8("Unknown"));
    legend->addStretch();
    layout->addLayout(legend);

    // ── Log area ─────────────────────────────────────────────────────────────
    log_ = new QTextEdit(this);
    log_->setReadOnly(true);
    log_->setFont(QFont("Courier New", 9));
    log_->setStyleSheet(
        "QTextEdit {"
        "  background-color: #1a1a2e;"
        "  color: #c8c8d0;"
        "  border: 1px solid #3a2a50;"
        "  border-radius: 10px;"
        "}");
    layout->addWidget(log_);

    connect(clearBtn_, &QPushButton::clicked, this, &CanMonitorPanel::onClear);
}

// ── Public slots ─────────────────────────────────────────────────────────────

void CanMonitorPanel::appendFrame(uint32_t cobId, QByteArray data)
{
    const uint8_t nodeId = nodeIdFromCob(cobId);
    if (!passesFilter(nodeId)) return;
    appendFormatted(QString::fromUtf8("\u2190"), cobId, data, nodeId);  // ←
}

void CanMonitorPanel::appendFrameTx(uint32_t cobId, QByteArray data)
{
    // For TX frames, the nodeId is the *target* node.
    const uint8_t nodeId = nodeIdFromCob(cobId);
    if (!passesFilter(nodeId)) return;
    appendFormatted(QString::fromUtf8("\u2192"), cobId, data, nodeId);  // →
}

// ── Private slots ────────────────────────────────────────────────────────────

void CanMonitorPanel::onClear()
{
    log_->clear();
}

QString CanMonitorPanel::nameForNode(uint8_t nodeId)
{
    switch (nodeId) {
    case NODE_AUTONOMOUS: return "Autonomous module";
    case NODE_DRIVE:      return "Drive    ";
    case NODE_ENERGY:     return "Energy   ";
    case NODE_SYSTEM:     return "System   ";
    default:              return QString("0x%1     ").arg(nodeId, 2, 16, QChar('0')).toUpper().left(9);
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

uint8_t CanMonitorPanel::nodeIdFromCob(uint32_t cobId)
{
    if (cobId == 0x000)                             return NODE_SYSTEM;   // NMT
    if (cobId >= 0x700 && cobId <= 0x77F)           return static_cast<uint8_t>(cobId - 0x700); // heartbeat
    if (cobId >= 0x580 && cobId <= 0x5FF)           return static_cast<uint8_t>(cobId - 0x580); // SDO resp
    if (cobId >= 0x600 && cobId <= 0x67F)           return static_cast<uint8_t>(cobId - 0x600); // SDO req
    if (cobId >= 0x180 && cobId <= 0x4FF)           return static_cast<uint8_t>(cobId & 0x7F);  // PDO
    return 0xFE; // unknown
}

QColor CanMonitorPanel::colorForNode(uint8_t nodeId)
{
    switch (nodeId) {
    case NODE_AUTONOMOUS: return QColor("#4ec9b0");  // teal
    case NODE_DRIVE:      return QColor("#ff7eb6");  // pink
    case NODE_ENERGY:     return QColor("#5599ff");  // blue
    case NODE_SYSTEM:     return QColor("#ffdd55");  // yellow
    default:              return QColor("#888888");  // gray
    }
}

QString CanMonitorPanel::tagForCob(uint32_t cobId)
{
    if (cobId == 0x000)                   return "[NMT]";
    if (cobId >= 0x700 && cobId <= 0x77F) return "[HB ]";
    if (cobId >= 0x580 && cobId <= 0x5FF) return "[SDO]";
    if (cobId >= 0x600 && cobId <= 0x67F) return "[SDO]";
    if (cobId >= 0x480 && cobId <= 0x4FF) return "[PDO]";
    if (cobId >= 0x380 && cobId <= 0x3FF) return "[PDO]";
    if (cobId >= 0x280 && cobId <= 0x2FF) return "[PDO]";
    if (cobId >= 0x180 && cobId <= 0x1FF) return "[PDO]";
    return "[CAN]";
}

bool CanMonitorPanel::passesFilter(uint8_t nodeId) const
{
    const int selected = filterBox_->currentData().toInt();
    if (selected == -1) return true;          // "Все модули"
    if (nodeId == NODE_SYSTEM) return true;   // NMT/system always shown
    return nodeId == static_cast<uint8_t>(selected);
}

void CanMonitorPanel::appendFormatted(const QString& dir, uint32_t cobId,
                                      const QByteArray& data, uint8_t nodeId)
{
    QString hex;
    for (int i = 0; i < data.size(); ++i) {
        if (i) hex += ' ';
        hex += QString("%1").arg(static_cast<uint8_t>(data[i]), 2, 16, QChar('0')).toUpper();
    }

    const QString line = QString("%1 %2 %3  COB 0x%4  %5")
                             .arg(nameForNode(nodeId))
                             .arg(dir)
                             .arg(tagForCob(cobId))
                             .arg(cobId, 3, 16, QChar('0')).toUpper()
                             .arg(hex);

    QTextCharFormat fmt;
    fmt.setForeground(colorForNode(nodeId));

    QTextCursor cursor(log_->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(line + "\n", fmt);

    if (autoScrollChk_->isChecked())
        log_->verticalScrollBar()->setValue(log_->verticalScrollBar()->maximum());
}
