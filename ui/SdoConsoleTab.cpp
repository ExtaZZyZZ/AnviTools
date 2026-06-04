#include "SdoConsoleTab.h"
#include <cstdint>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDateTime>

SdoConsoleTab::SdoConsoleTab(SdoClient* sdo, QWidget* parent)
    : QWidget(parent), sdo_(sdo)
{
    auto* layout = new QVBoxLayout(this);

    auto* inputGroup = new QGroupBox("SDO Запрос", this);
    auto* form = new QFormLayout(inputGroup);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);

    nodeIdEdit_ = new QLineEdit("0x02", this);
    nodeIdEdit_->setMaximumWidth(80);
    form->addRow("ID узла (hex):", nodeIdEdit_);

    indexEdit_ = new QLineEdit("0x6000", this);
    indexEdit_->setMaximumWidth(80);
    form->addRow("Индекс (hex):", indexEdit_);

    subIndexSpin_ = new QSpinBox(this);
    subIndexSpin_->setRange(0, 255);
    subIndexSpin_->setMaximumWidth(80);
    form->addRow("Субиндекс:", subIndexSpin_);

    dataTypeBox_ = new QComboBox(this);
    dataTypeBox_->addItems({"int8", "uint8", "uint16", "uint32", "string"});
    dataTypeBox_->setMaximumWidth(100);
    form->addRow("Тип данных:", dataTypeBox_);

    valueEdit_ = new QLineEdit(this);
    valueEdit_->setPlaceholderText("Значение (запись) / Результат (чтение)");
    form->addRow("Значение:", valueEdit_);

    auto* btnRow = new QHBoxLayout();
    readBtn_  = new QPushButton("Читать (SDO Upload)", this);
    writeBtn_ = new QPushButton("Записать (SDO Download)", this);
    btnRow->addWidget(readBtn_);
    btnRow->addWidget(writeBtn_);
    btnRow->addStretch();
    form->addRow("", btnRow);

    layout->addWidget(inputGroup);

    historyLog_ = new QTextEdit(this);
    historyLog_->setReadOnly(true);
    historyLog_->setFont(QFont("Courier New", 9));
    layout->addWidget(historyLog_);

    connect(readBtn_,  &QPushButton::clicked, this, &SdoConsoleTab::onRead);
    connect(writeBtn_, &QPushButton::clicked, this, &SdoConsoleTab::onWrite);
    connect(indexEdit_, &QLineEdit::textChanged, this, [this](const QString&) {
        applyKnownTypePreset();
    });

    applyKnownTypePreset();
}

void SdoConsoleTab::onRead()
{
    bool ok1, ok2;
    uint8_t  nodeId  = static_cast<uint8_t>(nodeIdEdit_->text().toUInt(&ok1, 16));
    uint16_t index   = static_cast<uint16_t>(indexEdit_->text().toUInt(&ok2, 16));
    uint8_t  subIdx  = static_cast<uint8_t>(subIndexSpin_->value());
    QString  type    = dataTypeBox_->currentText();

    if (!ok1 || !ok2) { log("[ERR] Неверный ID узла или индекс"); return; }

    setBusy(true);
    QString err;

    if (type == "string") {
        QString str;
        if (sdo_->sdoReadString(nodeId, index, subIdx, str, err)) {
            valueEdit_->setText(str);
            log(QString("READ  node=0x%1 idx=0x%2 sub=%3 -> \"%4\"")
                .arg(nodeId, 2, 16, QChar('0'))
                .arg(index, 4, 16, QChar('0'))
                .arg(subIdx).arg(str));
        } else {
            log("[ERR] " + err);
        }
    } else {
        QByteArray data;
        if (sdo_->sdoRead(nodeId, index, subIdx, data, err)) {
            QString val = bytesToString(data, type);
            valueEdit_->setText(val);
            log(QString("READ  node=0x%1 idx=0x%2 sub=%3 -> %4")
                .arg(nodeId, 2, 16, QChar('0'))
                .arg(index, 4, 16, QChar('0'))
                .arg(subIdx).arg(val));
        } else {
            log("[ERR] " + err);
        }
    }
    setBusy(false);
}

void SdoConsoleTab::onWrite()
{
    bool ok1, ok2;
    uint8_t  nodeId = static_cast<uint8_t>(nodeIdEdit_->text().toUInt(&ok1, 16));
    uint16_t index  = static_cast<uint16_t>(indexEdit_->text().toUInt(&ok2, 16));
    uint8_t  subIdx = static_cast<uint8_t>(subIndexSpin_->value());
    QString  type   = dataTypeBox_->currentText();

    if (!ok1 || !ok2) { log("[ERR] Неверный ID узла или индекс"); return; }

    setBusy(true);
    QString err;

    if (type == "string") {
        if (sdo_->sdoWriteString(nodeId, index, subIdx, valueEdit_->text(), err))
            log(QString("WRITE node=0x%1 idx=0x%2 sub=%3 = \"%4\" OK")
                .arg(nodeId, 2, 16, QChar('0'))
                .arg(index, 4, 16, QChar('0'))
                .arg(subIdx).arg(valueEdit_->text()));
        else
            log("[ERR] " + err);
    } else {
        QByteArray data = valueToBytes(valueEdit_->text(), type, err);
        if (!err.isEmpty()) { log("[ERR] " + err); setBusy(false); return; }
        if (sdo_->sdoWrite(nodeId, index, subIdx, data, err))
            log(QString("WRITE node=0x%1 idx=0x%2 sub=%3 = %4 OK")
                .arg(nodeId, 2, 16, QChar('0'))
                .arg(index, 4, 16, QChar('0'))
                .arg(subIdx).arg(valueEdit_->text()));
        else
            log("[ERR] " + err);
    }
    setBusy(false);
}

void SdoConsoleTab::log(const QString& msg)
{
    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    historyLog_->append(QString("[%1] %2").arg(ts, msg));
}

void SdoConsoleTab::setBusy(bool busy)
{
    readBtn_->setEnabled(!busy);
    writeBtn_->setEnabled(!busy);
}

void SdoConsoleTab::applyKnownTypePreset()
{
    bool ok = false;
    uint16_t index = static_cast<uint16_t>(indexEdit_->text().toUInt(&ok, 16));
    if (!ok) return;

    if (index == 0x6000 || index == 0x6004) {
        const int int8Index = dataTypeBox_->findText("int8");
        if (int8Index >= 0) {
            dataTypeBox_->setCurrentIndex(int8Index);
        }
    }
}

QByteArray SdoConsoleTab::valueToBytes(const QString& text, const QString& type, QString& err)
{
    bool ok;
    if (type == "int8") {
        int value = text.toInt(&ok, 0);
        if (!ok || value < -128 || value > 127) { err = "Invalid int8"; return {}; }
        return QByteArray(1, static_cast<char>(static_cast<int8_t>(value)));
    }
    if (type == "uint8") {
        uint8_t v = static_cast<uint8_t>(text.toUInt(&ok, 0));
        if (!ok) { err = "Invalid uint8"; return {}; }
        return QByteArray(1, static_cast<char>(v));
    }
    if (type == "uint16") {
        uint16_t v = static_cast<uint16_t>(text.toUInt(&ok, 0));
        if (!ok) { err = "Invalid uint16"; return {}; }
        QByteArray d(2, 0); memcpy(d.data(), &v, 2); return d;
    }
    if (type == "uint32") {
        uint32_t v = text.toULong(&ok, 0);
        if (!ok) { err = "Invalid uint32"; return {}; }
        QByteArray d(4, 0); memcpy(d.data(), &v, 4); return d;
    }
    err = "Unknown type";
    return {};
}

QString SdoConsoleTab::bytesToString(const QByteArray& data, const QString& type)
{
    if (type == "int8" && data.size() >= 1) {
        const int8_t v = static_cast<int8_t>(data[0]);
        return QString("0x%1 (%2)")
            .arg(static_cast<uint8_t>(v), 2, 16, QChar('0'))
            .arg(static_cast<int>(v));
    }
    if (type == "uint8"  && data.size() >= 1)
        return QString("0x%1 (%2)").arg((uint8_t)data[0], 2, 16, QChar('0'))
                                    .arg((uint8_t)data[0]);
    if (type == "uint16" && data.size() >= 2) {
        uint16_t v = 0; memcpy(&v, data.constData(), 2);
        return QString("0x%1 (%2)").arg(v, 4, 16, QChar('0')).arg(v);
    }
    if (type == "uint32" && data.size() >= 4) {
        uint32_t v = 0; memcpy(&v, data.constData(), 4);
        return QString("0x%1 (%2)").arg(v, 8, 16, QChar('0')).arg(v);
    }
    // raw hex fallback
    QString hex;
    for (uint8_t b : data) hex += QString("%1 ").arg(b, 2, 16, QChar('0'));
    return hex.trimmed();
}
