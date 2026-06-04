#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QList>
#include "../canopen/SdoClient.h"
#include "../model/FlashLogEntry.h"

class FlashLogTab : public QWidget {
    Q_OBJECT
public:
    explicit FlashLogTab(SdoClient* sdo, QWidget* parent = nullptr);

private slots:
    void onDump();
    void onExportCsv();

private:
    void populateTable(const QList<FlashLogEntry>& entries);
    void setStatus(const QString& msg, bool ok = true);
    void setBusy(bool busy);

    SdoClient*    sdo_;
    QSpinBox*     countSpin_;
    QPushButton*  dumpBtn_;
    QPushButton*  exportBtn_;
    QTableWidget* table_;
    QLabel*       statusLabel_;

    QList<FlashLogEntry> entries_; // last dump result

    static constexpr uint8_t  NODE_ID       = 0x02;
    static constexpr uint16_t OD_TRIGGER    = 0x2020;
    static constexpr uint16_t OD_ENTRY      = 0x2021;
    static constexpr uint16_t OD_REMAINING  = 0x2022;
};
