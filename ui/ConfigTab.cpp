#include "ConfigTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSignalMapper>
#include <QIcon>
#include <QSize>

static QPushButton* makeSmallBtn(const QIcon& icon, QWidget* parent)
{
    auto* btn = new QPushButton(parent);
    btn->setFixedSize(55, 55);
    btn->setIcon(icon);
    btn->setIconSize(QSize(50, 50));
    btn->setFlat(true);
    btn->setStyleSheet("background: transparent; border: none;");
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFocusPolicy(Qt::NoFocus);
    return btn;
}

ConfigTab::ConfigTab(SdoClient* sdo, QWidget* parent)
    : QWidget(parent), sdo_(sdo)
{
    auto* mainLayout = new QVBoxLayout(this);
    static const QIcon readIcon(":/resources/icon_read.png");
    static const QIcon writeIcon(":/resources/icon_write.png");

    // ── General Settings ──────────────────────────────────────────────────────
    auto* generalGroup = new QGroupBox("Общие настройки", this);
    auto* genForm = new QFormLayout(generalGroup);
    genForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    genForm->setRowWrapPolicy(QFormLayout::WrapLongRows);

    // APN (основной канал)
    apnEdit_ = new QLineEdit(this);
    apnEdit_->setMaxLength(DeviceConfig::MAX_APN_LEN);
    apnEdit_->setPlaceholderText("e.g. internet.mts.ru");
    apnReadBtn_  = makeSmallBtn(readIcon, this);
    apnWriteBtn_ = makeSmallBtn(writeIcon, this);
    apnReadBtn_->setToolTip("Прочитать APN основного канала с устройства");
    apnWriteBtn_->setToolTip("Записать APN основного канала на устройство");
    {
        auto* row = new QHBoxLayout();
        row->addWidget(apnEdit_);
        row->addWidget(apnReadBtn_);
        row->addWidget(apnWriteBtn_);
        genForm->addRow("APN (основной канал):", row);
    }

    // APN2 (резервный канал)
    apn2Edit_ = new QLineEdit(this);
    apn2Edit_->setMaxLength(DeviceConfig::MAX_APN_LEN);
    apn2Edit_->setPlaceholderText("e.g. internet.mts.ru");
    apn2ReadBtn_  = makeSmallBtn(readIcon, this);
    apn2WriteBtn_ = makeSmallBtn(writeIcon, this);
    apn2ReadBtn_->setToolTip("Прочитать APN резервного канала с устройства");
    apn2WriteBtn_->setToolTip("Записать APN резервного канала на устройство");
    {
        auto* row = new QHBoxLayout();
        row->addWidget(apn2Edit_);
        row->addWidget(apn2ReadBtn_);
        row->addWidget(apn2WriteBtn_);
        genForm->addRow("APN (резервный канал):", row);
    }

    // Poll interval row
    pollIntervalSpin_ = new QSpinBox(this);
    pollIntervalSpin_->setRange(1, 3600);
    pollIntervalSpin_->setValue(5);
    pollIntervalSpin_->setSuffix(" сек");
    pollReadBtn_  = makeSmallBtn(readIcon, this);
    pollWriteBtn_ = makeSmallBtn(writeIcon, this);
    pollReadBtn_->setToolTip("Прочитать интервал опроса с устройства");
    pollWriteBtn_->setToolTip("Записать интервал опроса на устройство");
    {
        auto* row = new QHBoxLayout();
        row->addWidget(pollIntervalSpin_);
        row->addWidget(pollReadBtn_);
        row->addWidget(pollWriteBtn_);
        genForm->addRow("Интервал опроса датчиков:", row);
    }

    // Connectivity check interval row
    connectivityIntervalSpin_ = new QSpinBox(this);
    connectivityIntervalSpin_->setRange(5, 86400);
    connectivityIntervalSpin_->setValue(60);
    connectivityIntervalSpin_->setSuffix(" сек");
    connReadBtn_  = makeSmallBtn(readIcon, this);
    connWriteBtn_ = makeSmallBtn(writeIcon, this);
    connReadBtn_->setToolTip("Прочитать интервал проверки сети с устройства");
    connWriteBtn_->setToolTip("Записать интервал проверки сети на устройство");
    {
        auto* row = new QHBoxLayout();
        row->addWidget(connectivityIntervalSpin_);
        row->addWidget(connReadBtn_);
        row->addWidget(connWriteBtn_);
        genForm->addRow("Интервал проверки сети:", row);
    }

    mainLayout->addWidget(generalGroup);

    // ── Phone Whitelist ───────────────────────────────────────────────────────
    auto* wlGroup  = new QGroupBox("Белый список номеров (до 10)", this);
    auto* wlLayout = new QVBoxLayout(wlGroup);
    auto* wlForm   = new QFormLayout();
    wlForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    wlForm->setRowWrapPolicy(QFormLayout::WrapLongRows);

    for (int i = 0; i < DeviceConfig::MAX_WHITELIST_ENTRIES; ++i) {
        whitelistEdits_[i] = new QLineEdit(this);
        whitelistEdits_[i]->setMaxLength(DeviceConfig::MAX_NUMBER_LEN);
        whitelistEdits_[i]->setPlaceholderText("+7XXXXXXXXXX");

        phoneReadBtns_[i]  = makeSmallBtn(readIcon, this);
        phoneWriteBtns_[i] = makeSmallBtn(writeIcon, this);
        phoneReadBtns_[i]->setToolTip(QString("Прочитать номер %1 с устройства").arg(i + 1));
        phoneWriteBtns_[i]->setToolTip(QString("Записать номер %1 на устройство").arg(i + 1));

        auto* row = new QHBoxLayout();
        row->addWidget(whitelistEdits_[i]);
        row->addWidget(phoneReadBtns_[i]);
        row->addWidget(phoneWriteBtns_[i]);
        wlForm->addRow(QString("  %1:").arg(i + 1), row);
    }
    wlLayout->addLayout(wlForm);
    mainLayout->addWidget(wlGroup);

    // ── Bottom buttons ────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout();
    readBtn_  = new QPushButton(QString::fromUtf8("Считать всё"), this);
    writeBtn_ = new QPushButton(QString::fromUtf8("Записать всё"), this);
    saveBtn_  = new QPushButton(QString::fromUtf8("Сохранить"), this);
    readBtn_->setMinimumWidth(100);
    writeBtn_->setMinimumWidth(100);
    saveBtn_->setMinimumWidth(100);
    saveBtn_->setToolTip(QString::fromUtf8("Сохранить во Flash"));
    btnRow->addWidget(readBtn_);
    btnRow->addWidget(writeBtn_);
    btnRow->addWidget(saveBtn_);
    btnRow->addStretch();
    mainLayout->addLayout(btnRow);

    statusLabel_ = new QLabel("", this);
    statusLabel_->setWordWrap(true);
    mainLayout->addWidget(statusLabel_);
    mainLayout->addStretch();

    // ── Connections ───────────────────────────────────────────────────────────
    connect(readBtn_,     &QPushButton::clicked, this, &ConfigTab::onReadAll);
    connect(writeBtn_,    &QPushButton::clicked, this, &ConfigTab::onWriteAll);
    connect(saveBtn_,     &QPushButton::clicked, this, &ConfigTab::onSaveToFlash);
    connect(apnReadBtn_,  &QPushButton::clicked, this, &ConfigTab::onReadApn);
    connect(apnWriteBtn_, &QPushButton::clicked, this, &ConfigTab::onWriteApn);
    connect(apn2ReadBtn_,  &QPushButton::clicked, this, &ConfigTab::onReadApn2);
    connect(apn2WriteBtn_, &QPushButton::clicked, this, &ConfigTab::onWriteApn2);
    connect(pollReadBtn_,  &QPushButton::clicked, this, &ConfigTab::onReadPollInterval);
    connect(pollWriteBtn_, &QPushButton::clicked, this, &ConfigTab::onWritePollInterval);
    connect(connReadBtn_,  &QPushButton::clicked, this, &ConfigTab::onReadConnectivityInterval);
    connect(connWriteBtn_, &QPushButton::clicked, this, &ConfigTab::onWriteConnectivityInterval);

    for (int i = 0; i < DeviceConfig::MAX_WHITELIST_ENTRIES; ++i) {
        connect(phoneReadBtns_[i],  &QPushButton::clicked, this, [this, i]() { onReadPhone(i); });
        connect(phoneWriteBtns_[i], &QPushButton::clicked, this, [this, i]() { onWritePhone(i); });
    }
}

// ── Read All ──────────────────────────────────────────────────────────────────
void ConfigTab::onReadAll()
{
    setBusy(true);
    setStatus("Reading...");

    QString apn, apn2, err;
    if (!sdo_->sdoReadString(NODE_ID, OD_APN, 0, apn, err)) {
        setStatus("[ERR] APN read: " + err, false); setBusy(false); return;
    }
    if (!sdo_->sdoReadString(NODE_ID, OD_APN2, 0, apn2, err)) {
        setStatus("[ERR] APN2 read: " + err, false); setBusy(false); return;
    }

    uint32_t pollInt = 5;
    if (!sdo_->readUInt32(NODE_ID, OD_POLL_INT, 0, pollInt, err)) {
        setStatus("[ERR] Poll interval read: " + err, false); setBusy(false); return;
    }

    uint32_t connInt = 60;
    if (!sdo_->readUInt32(NODE_ID, OD_CONN_INT, 0, connInt, err)) {
        setStatus("[ERR] Connectivity interval read: " + err, false); setBusy(false); return;
    }

    uint8_t count = 0;
    sdo_->readUInt8(NODE_ID, OD_WHITELIST, 0, count, err);

    DeviceConfig cfg;
    cfg.apn  = apn;
    cfg.apn2 = apn2;
    cfg.sensorPollIntervalSec = pollInt;
    cfg.connectivityCheckSec  = connInt;
    for (int i = 0; i < qMin((int)count, DeviceConfig::MAX_WHITELIST_ENTRIES); ++i) {
        QString num;
        if (sdo_->sdoReadString(NODE_ID, OD_WHITELIST, static_cast<uint8_t>(i + 1), num, err))
            cfg.whitelist << num;
    }
    applyConfig(cfg);
    setStatus("Чтение выполнено");
    setBusy(false);
}

// ── Write All ─────────────────────────────────────────────────────────────────
void ConfigTab::onWriteAll()
{
    setBusy(true);
    setStatus("Writing...");

    DeviceConfig cfg = collectConfig();
    QString err;

    if (!sdo_->sdoWriteString(NODE_ID, OD_APN, 0, cfg.apn, err)) {
        setStatus("[ERR] APN write: " + err, false); setBusy(false); return;
    }
    if (!sdo_->sdoWriteString(NODE_ID, OD_APN2, 0, cfg.apn2, err)) {
        setStatus("[ERR] APN2 write: " + err, false); setBusy(false); return;
    }
    if (!sdo_->writeUInt32(NODE_ID, OD_POLL_INT, 0, cfg.sensorPollIntervalSec, err)) {
        setStatus("[ERR] Poll interval write: " + err, false); setBusy(false); return;
    }
    if (!sdo_->writeUInt32(NODE_ID, OD_CONN_INT, 0, cfg.connectivityCheckSec, err)) {
        setStatus("[ERR] Connectivity interval write: " + err, false); setBusy(false); return;
    }

    uint8_t count = static_cast<uint8_t>(cfg.whitelist.size());
    sdo_->writeUInt8(NODE_ID, OD_WHITELIST, 0, count, err);
    for (int i = 0; i < cfg.whitelist.size(); ++i) {
        if (!sdo_->sdoWriteString(NODE_ID, OD_WHITELIST,
                                   static_cast<uint8_t>(i + 1), cfg.whitelist[i], err)) {
            setStatus(QString("[ERR] Phone[%1]: ").arg(i + 1) + err, false);
            setBusy(false); return;
        }
    }
    setStatus("Записано — нажмите 'Сохранить во Flash' для постоянного хранения");
    setBusy(false);
}

// ── Save to Flash ─────────────────────────────────────────────────────────────
void ConfigTab::onSaveToFlash()
{
    setBusy(true);
    setStatus("Сохранение во Flash...");
    QString err;
    if (sdo_->writeUInt8(NODE_ID, OD_SAVE, 0, 1, err))
        setStatus("Сохранено во Flash успешно");
    else
        setStatus("[ERR] Ошибка сохранения: " + err, false);
    setBusy(false);
}

// ── Individual: APN ───────────────────────────────────────────────────────────
void ConfigTab::onReadApn()
{
    setBusy(true);
    QString apn, err;
    if (sdo_->sdoReadString(NODE_ID, OD_APN, 0, apn, err)) {
        apnEdit_->setText(apn);
        setStatus("APN прочитан");
    } else {
        setStatus("[ERR] APN read: " + err, false);
    }
    setBusy(false);
}

void ConfigTab::onWriteApn()
{
    setBusy(true);
    QString err;
    if (sdo_->sdoWriteString(NODE_ID, OD_APN, 0, apnEdit_->text().trimmed(), err))
        setStatus("APN записан");
    else
        setStatus("[ERR] APN write: " + err, false);
    setBusy(false);
}

// ── Individual: APN2 ─────────────────────────────────────────────────────────
void ConfigTab::onReadApn2()
{
    setBusy(true);
    QString apn2, err;
    if (sdo_->sdoReadString(NODE_ID, OD_APN2, 0, apn2, err)) {
        apn2Edit_->setText(apn2);
        setStatus("APN резервного канала прочитан");
    } else {
        setStatus("[ERR] APN2 read: " + err, false);
    }
    setBusy(false);
}

void ConfigTab::onWriteApn2()
{
    setBusy(true);
    QString err;
    if (sdo_->sdoWriteString(NODE_ID, OD_APN2, 0, apn2Edit_->text().trimmed(), err))
        setStatus("APN резервного канала записан");
    else
        setStatus("[ERR] APN2 write: " + err, false);
    setBusy(false);
}

// ── Individual: Poll Interval ─────────────────────────────────────────────────
void ConfigTab::onReadPollInterval()
{
    setBusy(true);
    uint32_t val = 5;
    QString err;
    if (sdo_->readUInt32(NODE_ID, OD_POLL_INT, 0, val, err)) {
        pollIntervalSpin_->setValue(static_cast<int>(val));
        setStatus("Интервал опроса прочитан");
    } else {
        setStatus("[ERR] Poll read: " + err, false);
    }
    setBusy(false);
}

void ConfigTab::onWritePollInterval()
{
    setBusy(true);
    QString err;
    uint32_t val = static_cast<uint32_t>(pollIntervalSpin_->value());
    if (sdo_->writeUInt32(NODE_ID, OD_POLL_INT, 0, val, err))
        setStatus("Интервал опроса записан");
    else
        setStatus("[ERR] Poll write: " + err, false);
    setBusy(false);
}

// ── Individual: Connectivity Interval ────────────────────────────────────────
void ConfigTab::onReadConnectivityInterval()
{
    setBusy(true);
    uint32_t val = 60;
    QString err;
    if (sdo_->readUInt32(NODE_ID, OD_CONN_INT, 0, val, err)) {
        connectivityIntervalSpin_->setValue(static_cast<int>(val));
        setStatus("Интервал проверки сети прочитан");
    } else {
        setStatus("[ERR] Connectivity read: " + err, false);
    }
    setBusy(false);
}

void ConfigTab::onWriteConnectivityInterval()
{
    setBusy(true);
    QString err;
    uint32_t val = static_cast<uint32_t>(connectivityIntervalSpin_->value());
    if (sdo_->writeUInt32(NODE_ID, OD_CONN_INT, 0, val, err))
        setStatus("Интервал проверки сети записан");
    else
        setStatus("[ERR] Connectivity write: " + err, false);
    setBusy(false);
}

// ── Individual: Phone ─────────────────────────────────────────────────────────
void ConfigTab::onReadPhone(int idx)
{
    setBusy(true);
    QString num, err;
    if (sdo_->sdoReadString(NODE_ID, OD_WHITELIST, static_cast<uint8_t>(idx + 1), num, err)) {
        whitelistEdits_[idx]->setText(num);
        setStatus(QString("Номер %1 прочитан").arg(idx + 1));
    } else {
        setStatus(QString("[ERR] Phone %1 read: ").arg(idx + 1) + err, false);
    }
    setBusy(false);
}

void ConfigTab::onWritePhone(int idx)
{
    setBusy(true);
    QString err;
    QString num = whitelistEdits_[idx]->text().trimmed();

    // Update count on device: read current count, extend if needed
    uint8_t count = 0;
    sdo_->readUInt8(NODE_ID, OD_WHITELIST, 0, count, err);
    if (idx + 1 > count) {
        count = static_cast<uint8_t>(idx + 1);
        sdo_->writeUInt8(NODE_ID, OD_WHITELIST, 0, count, err);
    }

    if (sdo_->sdoWriteString(NODE_ID, OD_WHITELIST, static_cast<uint8_t>(idx + 1), num, err))
        setStatus(QString("Номер %1 записан").arg(idx + 1));
    else
        setStatus(QString("[ERR] Phone %1 write: ").arg(idx + 1) + err, false);
    setBusy(false);
}

// ── Helpers ───────────────────────────────────────────────────────────────────
void ConfigTab::applyConfig(const DeviceConfig& cfg)
{
    apnEdit_->setText(cfg.apn);
    apn2Edit_->setText(cfg.apn2);
    pollIntervalSpin_->setValue(static_cast<int>(cfg.sensorPollIntervalSec));
    connectivityIntervalSpin_->setValue(static_cast<int>(cfg.connectivityCheckSec));
    for (int i = 0; i < DeviceConfig::MAX_WHITELIST_ENTRIES; ++i)
        whitelistEdits_[i]->setText(i < cfg.whitelist.size() ? cfg.whitelist[i] : "");
}

DeviceConfig ConfigTab::collectConfig() const
{
    DeviceConfig cfg;
    cfg.apn  = apnEdit_->text().trimmed();
    cfg.apn2 = apn2Edit_->text().trimmed();
    cfg.sensorPollIntervalSec = static_cast<uint32_t>(pollIntervalSpin_->value());
    cfg.connectivityCheckSec  = static_cast<uint32_t>(connectivityIntervalSpin_->value());
    for (int i = 0; i < DeviceConfig::MAX_WHITELIST_ENTRIES; ++i) {
        QString num = whitelistEdits_[i]->text().trimmed();
        if (!num.isEmpty()) cfg.whitelist << num;
    }
    return cfg;
}

void ConfigTab::setStatus(const QString& msg, bool ok)
{
    statusLabel_->setText(msg);
    statusLabel_->setStyleSheet(ok ? "color: green;" : "color: red;");
}

void ConfigTab::setBusy(bool busy)
{
    readBtn_->setEnabled(!busy);
    writeBtn_->setEnabled(!busy);
    saveBtn_->setEnabled(!busy);
    apnReadBtn_->setEnabled(!busy);
    apnWriteBtn_->setEnabled(!busy);
    apn2ReadBtn_->setEnabled(!busy);
    apn2WriteBtn_->setEnabled(!busy);
    pollReadBtn_->setEnabled(!busy);
    pollWriteBtn_->setEnabled(!busy);
    connReadBtn_->setEnabled(!busy);
    connWriteBtn_->setEnabled(!busy);
    for (int i = 0; i < DeviceConfig::MAX_WHITELIST_ENTRIES; ++i) {
        phoneReadBtns_[i]->setEnabled(!busy);
        phoneWriteBtns_[i]->setEnabled(!busy);
    }
}
