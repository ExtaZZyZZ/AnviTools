#pragma once
#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>

/**
 * MonitorTab — UART2 debug output viewer with color-coded tags:
 *   [OK]    → green
 *   [ERR]   → red
 *   [WARN]  → yellow
 *   [SLEEP] → blue
 *   [RTC]   → cyan
 *   [SPI]   → orange
 *   [INFO]  → light blue
 *   ***     → magenta (critical events)
 *   [BOOT]  → yellow-white
 *   default → light gray
 */
class MonitorTab : public QWidget {
    Q_OBJECT
public:
    explicit MonitorTab(QWidget* parent = nullptr);

public slots:
    void appendLine(const QString& line);

private slots:
    void onClear();
    void onCopyAll();
    void onSearchChanged(const QString& term);
    void onFindNext();
    void onFindPrev();

private:
    QTextEdit*   textEdit_;
    QPushButton* clearBtn_;
    QPushButton* copyAllBtn_;
    QCheckBox*   autoScrollChk_;

    QLineEdit*   searchEdit_;
    QToolButton* findPrevBtn_;
    QToolButton* findNextBtn_;
    QLabel*      matchCountLabel_;

    void highlightAllMatches(const QString& term);
    int  countMatches(const QString& term) const;
    QColor colorForLine(const QString& line) const;
};
