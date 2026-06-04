#include "FlashLogTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QApplication>

FlashLogTab::FlashLogTab(SdoClient* sdo, QWidget* parent)
    : QWidget(parent), sdo_(sdo)
{
    auto* layout = new QVBoxLayout(this);

    // Controls
    auto* ctrlRow = new QHBoxLayout();
    ctrlRow->addWidget(new QLabel("Количество записей:", this));
    countSpin_ = new QSpinBox(this);
    countSpin_->setRange(1, 255);
    countSpin_->setValue(50);
    ctrlRow->addWidget(countSpin_);
    dumpBtn_   = new QPushButton("Считать журнал", this);
    exportBtn_ = new QPushButton("Экспорт CSV", this);
    exportBtn_->setEnabled(false);
    ctrlRow->addWidget(dumpBtn_);
    ctrlRow->addWidget(exportBtn_);
    ctrlRow->addStretch();
    layout->addLayout(ctrlRow);

    // Table
    table_ = new QTableWidget(0, 5, this);
    table_->setHorizontalHeaderLabels({"#", "Время (UTC)", "Датчик", "Давление (кПа)", "Валидно"});
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setColumnWidth(0, 60);
    table_->setColumnWidth(1, 180);
    table_->setColumnWidth(2, 100);
    table_->setColumnWidth(3, 130);
    layout->addWidget(table_);

    statusLabel_ = new QLabel("", this);
    layout->addWidget(statusLabel_);

    connect(dumpBtn_,   &QPushButton::clicked, this, &FlashLogTab::onDump);
    connect(exportBtn_, &QPushButton::clicked, this, &FlashLogTab::onExportCsv);
}

void FlashLogTab::onDump()
{
    setBusy(true);
    setStatus("Запрос журнала Flash...");
    entries_.clear();
    table_->setRowCount(0);

    QString err;
    uint8_t n = static_cast<uint8_t>(countSpin_->value());

    // Step 1: trigger dump
    if (!sdo_->writeUInt8(NODE_ID, OD_TRIGGER, 0, n, err)) {
        setStatus("[ERR] Запрос: " + err, false); setBusy(false); return;
    }

    // Step 2: read count available
    uint32_t remaining = 0;
    if (!sdo_->readUInt32(NODE_ID, OD_REMAINING, 0, remaining, err)) {
        setStatus("[ERR] Количество записей: " + err, false); setBusy(false); return;
    }

    setStatus(QString("Чтение %1 записей...").arg(remaining));
    QApplication::processEvents();

    // Step 3: read entries one by one
    int readCount = 0;
    while (remaining > 0) {
        QByteArray raw;
        if (!sdo_->sdoRead(NODE_ID, OD_ENTRY, 0, raw, err)) {
            setStatus(QString("[ERR] Чтение записи %1: ").arg(readCount) + err, false);
            break;
        }
        // Entry is 8 bytes — may need segmented read if firmware sends it that way
        // For expedited: raw contains 4 bytes (SDO expedited max). Use full 8-byte via
        // segmented upload — or firmware must return 4-byte chunks in 2 reads.
        // Here we assume firmware returns 8 bytes via segmented upload:
        if (raw.size() >= 8) {
            entries_ << FlashLogEntry::fromBytes(raw);
        }
        ++readCount;

        // Update remaining
        uint32_t rem2 = 0;
        if (!sdo_->readUInt32(NODE_ID, OD_REMAINING, 0, rem2, err)) break;
        remaining = rem2;

        if (readCount % 10 == 0) {
            setStatus(QString("Прочитано %1 записей...").arg(readCount));
            QApplication::processEvents();
        }
    }

    populateTable(entries_);
    setStatus(QString("Готово: %1 записей считано").arg(entries_.size()));
    exportBtn_->setEnabled(!entries_.isEmpty());
    setBusy(false);
}

void FlashLogTab::onExportCsv()
{
    QString path = QFileDialog::getSaveFileName(this, "Экспорт CSV", "flash_log.csv",
                                                 "CSV файлы (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setStatus("[ERR] Невозможно записать файл", false); return;
    }
    QTextStream out(&f);
    out << "Index,Timestamp,TimestampUnix,Sensor,Pressure_kPa,Valid\n";
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        out << i << ","
            << e.timestampString() << ","
            << e.timestamp_unix << ","
            << e.sensorName() << ","
            << e.pressure_kPa << ","
            << (e.isValid() ? "1" : "0") << "\n";
    }
    setStatus("Экспортировано в " + path);
}

void FlashLogTab::populateTable(const QList<FlashLogEntry>& entries)
{
    table_->setRowCount(entries.size());
    for (int i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        table_->setItem(i, 0, new QTableWidgetItem(QString::number(i)));
        table_->setItem(i, 1, new QTableWidgetItem(e.timestampString()));
        table_->setItem(i, 2, new QTableWidgetItem(e.sensorName()));
        table_->setItem(i, 3, new QTableWidgetItem(QString::number(e.pressure_kPa)));
        auto* validItem = new QTableWidgetItem(e.isValid() ? "Да" : "Нет");
        validItem->setForeground(e.isValid() ? Qt::green : Qt::red);
        table_->setItem(i, 4, validItem);
    }
}

void FlashLogTab::setStatus(const QString& msg, bool ok)
{
    statusLabel_->setText(msg);
    statusLabel_->setStyleSheet(ok ? "color: green;" : "color: red;");
}

void FlashLogTab::setBusy(bool busy)
{
    dumpBtn_->setEnabled(!busy);
    countSpin_->setEnabled(!busy);
}
