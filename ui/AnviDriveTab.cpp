#include "AnviDriveTab.h"
#include "FirmwareTab.h"
#include <cstring>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include "../can/CanDriver.h"
#include "../canopen/SdoClient.h"

AnviDriveTab::AnviDriveTab(CanDriver* can, SdoClient* sdo, QWidget* parent)
    : QWidget(parent), sdo_(sdo)
{
    setObjectName("modulePage");

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 12, 0, 12);
    rootLayout->setSpacing(16);

    // --- Hero card ---
    auto* heroCard = new QFrame(this);
    heroCard->setObjectName("heroCard");
    auto* heroLayout = new QVBoxLayout(heroCard);
    heroLayout->setContentsMargins(24, 22, 24, 22);
    heroLayout->setSpacing(12);

    auto* titleLabel = new QLabel(QString::fromUtf8("AnviDrive"), heroCard);
    titleLabel->setObjectName("pageTitle");
    heroLayout->addWidget(titleLabel);

    auto* stateRow = new QHBoxLayout();
    stateRow->setSpacing(10);

    canStateBadge_ = new QLabel(QString::fromUtf8("Модуль не подключен"), heroCard);
    canStateBadge_->setObjectName("offlineBadge");
    stateRow->addWidget(canStateBadge_, 0, Qt::AlignLeft);

    canStateHint_ = new QLabel(
        QString::fromUtf8("Модуль 0x07 не ответил на NMT-команду сканирования."),
        heroCard);
    canStateHint_->setObjectName("hintText");
    canStateHint_->setWordWrap(true);
    stateRow->addWidget(canStateHint_, 1);

    heroLayout->addLayout(stateRow);
    rootLayout->addWidget(heroCard);

    // --- Tool tabs ---
    toolTabs_ = new QTabWidget(this);
    toolTabs_->setObjectName("moduleToolTabs");
    toolTabs_->setDocumentMode(true);
    toolTabs_->setUsesScrollButtons(true);
    toolTabs_->tabBar()->setExpanding(false);
    toolTabs_->tabBar()->setElideMode(Qt::ElideRight);
    rootLayout->addWidget(toolTabs_, 1);

    // ---- Telemetry tab ----
    auto* telPage = new QWidget(this);
    auto* telLayout = new QVBoxLayout(telPage);
    telLayout->setContentsMargins(16, 16, 16, 16);
    telLayout->setSpacing(12);

    auto* pdoGroup = new QGroupBox(QString::fromUtf8("PDO данные"), telPage);
    auto* pdoForm = new QFormLayout(pdoGroup);
    pdoForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    percentLabel_    = new QLabel("—", pdoGroup);
    speedMotorLabel_ = new QLabel("—", pdoGroup);
    speedDriveLabel_ = new QLabel("—", pdoGroup);
    tempElecLabel_   = new QLabel("—", pdoGroup);
    tempMotorLabel_  = new QLabel("—", pdoGroup);
    tempTransLabel_  = new QLabel("—", pdoGroup);
    humidityLabel_   = new QLabel("—", pdoGroup);

    pdoForm->addRow(QString::fromUtf8("Положение (%):"),               percentLabel_);
    pdoForm->addRow(QString::fromUtf8("Скорость вала мотора (об/мин):"), speedMotorLabel_);
    pdoForm->addRow(QString::fromUtf8("Скорость привода (об/мин):"),     speedDriveLabel_);
    pdoForm->addRow(QString::fromUtf8("Температура электроники (°C):"), tempElecLabel_);
    pdoForm->addRow(QString::fromUtf8("Температура эл.двигателя (°C):"), tempMotorLabel_);
    pdoForm->addRow(QString::fromUtf8("Температура транзисторов (°C):"), tempTransLabel_);
    pdoForm->addRow(QString::fromUtf8("Влажность (%):"),                humidityLabel_);

    telLayout->addWidget(pdoGroup);

    auto* statGroup = new QGroupBox(QString::fromUtf8("Статус"), telPage);
    auto* statForm = new QFormLayout(statGroup);

    lastUpdateLabel_ = new QLabel("—", statGroup);
    pdoCountLabel_   = new QLabel("0", statGroup);

    statForm->addRow(QString::fromUtf8("Последнее обновление:"), lastUpdateLabel_);
    statForm->addRow(QString::fromUtf8("Принято PDO:"),          pdoCountLabel_);

    telLayout->addWidget(statGroup);
    telLayout->addStretch();

    toolTabs_->addTab(telPage, QString::fromUtf8("Телеметрия"));

    // ---- Control tab ----
    auto* ctrlPage = new QWidget(this);
    auto* ctrlLayout = new QVBoxLayout(ctrlPage);
    ctrlLayout->setContentsMargins(16, 16, 16, 16);
    ctrlLayout->setSpacing(14);

    // Helper: circular icon label
    auto makeCircleIcon = [&](const QString& symbol, QWidget* parent) {
        auto* lbl = new QLabel(symbol, parent);
        lbl->setFixedSize(56, 56);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet(
            "background: #7B3FA0;"
            "color: white;"
            "border-radius: 28px;"
            "font-size: 18pt;"
            "font-weight: bold;");
        return lbl;
    };

    // Helper: make spinbox
    auto makeSpin = [&](double min, double max, double step, int dec, QWidget* parent) {
        auto* s = new QDoubleSpinBox(parent);
        s->setRange(min, max);
        s->setSingleStep(step);
        s->setDecimals(dec);
        s->setMaximumWidth(90);
        return s;
    };

    // ── Card 1: Управление ──────────────────────────────────────────────────
    auto* card1 = new QFrame(ctrlPage);
    card1->setObjectName("heroCard");
    auto* c1Layout = new QVBoxLayout(card1);
    c1Layout->setContentsMargins(20, 18, 20, 18);
    c1Layout->setSpacing(12);

    c1Layout->addWidget(makeCircleIcon(QString::fromUtf8("⌘"), card1), 0, Qt::AlignHCenter);

    auto* c1Title = new QLabel(QString::fromUtf8("Управление"), card1);
    c1Title->setAlignment(Qt::AlignHCenter);
    c1Title->setStyleSheet("font-size: 13pt; font-weight: 700;");
    c1Layout->addWidget(c1Title);

    // Открыть / Стоп / Закрыть
    {
        auto* row = new QHBoxLayout();
        row->setSpacing(10);
        openBtn_  = new QPushButton(QString::fromUtf8("Открыть"), card1);
        stopBtn_  = new QPushButton(QString::fromUtf8("Стоп"),    card1);
        closeBtn_ = new QPushButton(QString::fromUtf8("Закрыть"), card1);
        row->addWidget(openBtn_);
        row->addWidget(stopBtn_);
        row->addWidget(closeBtn_);
        c1Layout->addLayout(row);
    }

    // Открыть на % [spin] [Старт]
    {
        auto* row = new QHBoxLayout();
        row->setSpacing(8);
        auto* lbl = new QLabel(QString::fromUtf8("Открыть на"), card1);
        percentSpin_ = makeSpin(0.0, 100.0, 1.0, 1, card1);
        auto* pct = new QLabel(QString::fromUtf8("%"), card1);
        percentBtn_ = new QPushButton(QString::fromUtf8("Старт"), card1);
        percentBtn_->setMinimumWidth(80);
        row->addWidget(lbl);
        row->addWidget(percentSpin_);
        row->addWidget(pct);
        row->addWidget(percentBtn_);
        row->addStretch();
        c1Layout->addLayout(row);
    }

    // ── Card 2: Настройка Valve Travel ──────────────────────────────────────
    auto* card2 = new QFrame(ctrlPage);
    card2->setObjectName("heroCard");
    auto* c2Layout = new QVBoxLayout(card2);
    c2Layout->setContentsMargins(20, 18, 20, 18);
    c2Layout->setSpacing(10);

    c2Layout->addWidget(makeCircleIcon(QString::fromUtf8("↺"), card2), 0, Qt::AlignHCenter);

    auto* c2Title = new QLabel(QString::fromUtf8("Настройка Valve Travel"), card2);
    c2Title->setAlignment(Qt::AlignHCenter);
    c2Title->setStyleSheet("font-size: 13pt; font-weight: 700;");
    c2Layout->addWidget(c2Title);

    // Закрыть до [spin] Нм [Старт]
    {
        auto* row = new QHBoxLayout();
        row->setSpacing(8);
        auto* lbl = new QLabel(QString::fromUtf8("Закрыть до"), card2);
        closeTorqueSpin_ = makeSpin(0.0, 999.0, 1.0, 1, card2);
        auto* unit = new QLabel(QString::fromUtf8("Нм"), card2);
        closeVTBtn_ = new QPushButton(QString::fromUtf8("Старт"), card2);
        closeVTBtn_->setMinimumWidth(80);
        row->addWidget(lbl);
        row->addWidget(closeTorqueSpin_);
        row->addWidget(unit);
        row->addWidget(closeVTBtn_);
        row->addStretch();
        c2Layout->addLayout(row);
    }

    // Установить 0% (full width)
    setZeroBtn_ = new QPushButton(QString::fromUtf8("Установить 0%"), card2);
    c2Layout->addWidget(setZeroBtn_);

    // Открыть до [spin] Нм [Старт]
    {
        auto* row = new QHBoxLayout();
        row->setSpacing(8);
        auto* lbl = new QLabel(QString::fromUtf8("Открыть до"), card2);
        openTorqueSpin_ = makeSpin(0.0, 999.0, 1.0, 1, card2);
        auto* unit = new QLabel(QString::fromUtf8("Нм"), card2);
        openVTBtn_ = new QPushButton(QString::fromUtf8("Старт"), card2);
        openVTBtn_->setMinimumWidth(80);
        row->addWidget(lbl);
        row->addWidget(openTorqueSpin_);
        row->addWidget(unit);
        row->addWidget(openVTBtn_);
        row->addStretch();
        c2Layout->addLayout(row);
    }

    // Рассчитать VT [spin] оборотов
    {
        auto* row = new QHBoxLayout();
        row->setSpacing(8);
        calcVTBtn_  = new QPushButton(QString::fromUtf8("Рассчитать VT"), card2);
        calcVTSpin_ = makeSpin(0.0, 9999.0, 1.0, 0, card2);
        calcVTSpin_->setValue(225);
        auto* unit = new QLabel(QString::fromUtf8("оборотов"), card2);
        row->addWidget(calcVTBtn_);
        row->addWidget(calcVTSpin_);
        row->addWidget(unit);
        row->addStretch();
        c2Layout->addLayout(row);
    }

    // Place both cards side by side
    auto* cardsRow = new QHBoxLayout();
    cardsRow->setSpacing(14);
    cardsRow->addWidget(card1, 1);
    cardsRow->addWidget(card2, 1);
    ctrlLayout->addLayout(cardsRow);

    cmdLog_ = new QTextEdit(ctrlPage);
    cmdLog_->setReadOnly(true);
    cmdLog_->setFont(QFont("Courier New", 9));
    ctrlLayout->addWidget(cmdLog_, 1);

    toolTabs_->addTab(ctrlPage, QString::fromUtf8("Управление (SDO)"));

    firmwareTab_ = new FirmwareTab(can, NODE_ID, this);
    toolTabs_->addTab(firmwareTab_, QString::fromUtf8("Прошивка"));

    // Connect signals
    connect(can, &CanDriver::frameReceived, this, &AnviDriveTab::onCanFrame);
    connect(openBtn_,    &QPushButton::clicked, this, &AnviDriveTab::onCmdOpen);
    connect(stopBtn_,    &QPushButton::clicked, this, &AnviDriveTab::onCmdStop);
    connect(closeBtn_,   &QPushButton::clicked, this, &AnviDriveTab::onCmdClose);
    connect(percentBtn_, &QPushButton::clicked, this, &AnviDriveTab::onCmdPercent);
    connect(closeVTBtn_, &QPushButton::clicked, this, &AnviDriveTab::onCmdCloseVT);
    connect(setZeroBtn_, &QPushButton::clicked, this, &AnviDriveTab::onCmdSetZero);
    connect(openVTBtn_,  &QPushButton::clicked, this, &AnviDriveTab::onCmdOpenVT);
    connect(calcVTBtn_,  &QPushButton::clicked, this, &AnviDriveTab::onCmdCalcVT);

    setModuleConnected(false);
}

void AnviDriveTab::setModuleConnected(bool connected)
{
    canStateBadge_->setText(connected
        ? QString::fromUtf8("Модуль подключен")
        : QString::fromUtf8("Модуль не подключен"));
    canStateBadge_->setObjectName(connected ? "onlineBadge" : "offlineBadge");
    canStateBadge_->style()->unpolish(canStateBadge_);
    canStateBadge_->style()->polish(canStateBadge_);

    canStateHint_->setText(connected
        ? QString::fromUtf8("Модуль 0x07 найден в сети. Доступна телеметрия PDO и управление SDO.")
        : QString::fromUtf8("Модуль 0x07 не ответил на NMT-команду сканирования."));
}

void AnviDriveTab::onCanFrame(uint32_t cobId, QByteArray data)
{
    if (cobId == COB_PERCENT && data.size() >= 4) {
        float val = 0.0f;
        memcpy(&val, data.constData(), 4);
        percentLabel_->setText(QString::number(static_cast<double>(val), 'f', 1));
        pdoCount_++;
        pdoCountLabel_->setText(QString::number(pdoCount_));
        lastUpdateLabel_->setText(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));
        return;
    }

    if (cobId == COB_SPD_MOTOR && data.size() >= 2) {
        int16_t val = 0;
        memcpy(&val, data.constData(), 2);
        speedMotorLabel_->setText(QString::number(val));
        pdoCount_++;
        pdoCountLabel_->setText(QString::number(pdoCount_));
        lastUpdateLabel_->setText(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));
        return;
    }

    if (cobId == COB_SPD_DRIVE && data.size() >= 2) {
        int16_t val = 0;
        memcpy(&val, data.constData(), 2);
        speedDriveLabel_->setText(QString::number(val));
        pdoCount_++;
        pdoCountLabel_->setText(QString::number(pdoCount_));
        lastUpdateLabel_->setText(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));
        return;
    }

    if (cobId == COB_TEMPS && data.size() >= 8) {
        int16_t tempElec  = 0;
        int16_t tempMotor = 0;
        int16_t tempTrans = 0;
        int16_t humidity  = 0;
        memcpy(&tempElec,  data.constData() + 0, 2);
        memcpy(&tempMotor, data.constData() + 2, 2);
        memcpy(&tempTrans, data.constData() + 4, 2);
        memcpy(&humidity,  data.constData() + 6, 2);
        tempElecLabel_->setText(QString::number(tempElec));
        tempMotorLabel_->setText(QString::number(tempMotor));
        tempTransLabel_->setText(QString::number(tempTrans));
        humidityLabel_->setText(QString::number(humidity));
        pdoCount_++;
        pdoCountLabel_->setText(QString::number(pdoCount_));
        lastUpdateLabel_->setText(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));
        return;
    }
}

void AnviDriveTab::onCmdOpen()
{
    float val = 100.0f;
    QByteArray data(4, 0);
    memcpy(data.data(), &val, 4);
    setBusy(true);
    QString err;
    if (sdo_->sdoWrite(NODE_ID, 0x600D, 0, data, err))
        log(QString::fromUtf8("WRITE 0x600D (open) = 100% OK"));
    else
        log(QString("[ERR] open: ") + err);
    setBusy(false);
}

void AnviDriveTab::onCmdStop()
{
    setBusy(true);
    QString err;
    if (sdo_->writeUInt32(NODE_ID, 0x6000, 0, 1, err))
        log(QString::fromUtf8("WRITE 0x6000 (stop) = 1 OK"));
    else
        log(QString("[ERR] stop: ") + err);
    setBusy(false);
}

void AnviDriveTab::onCmdClose()
{
    float val = 0.0f;
    QByteArray data(4, 0);
    memcpy(data.data(), &val, 4);
    setBusy(true);
    QString err;
    if (sdo_->sdoWrite(NODE_ID, 0x600D, 0, data, err))
        log(QString::fromUtf8("WRITE 0x600D (close) = 0% OK"));
    else
        log(QString("[ERR] close: ") + err);
    setBusy(false);
}

void AnviDriveTab::onCmdPercent()
{
    float val = static_cast<float>(percentSpin_->value());
    QByteArray data(4, 0);
    memcpy(data.data(), &val, 4);
    setBusy(true);
    QString err;
    if (sdo_->sdoWrite(NODE_ID, 0x600D, 0, data, err))
        log(QString("WRITE 0x600D (percent) = %1% OK").arg(percentSpin_->value(), 0, 'f', 1));
    else
        log(QString("[ERR] percent: ") + err);
    setBusy(false);
}

void AnviDriveTab::onCmdCloseVT()
{
    float val = static_cast<float>(closeTorqueSpin_->value());
    QByteArray data(4, 0);
    memcpy(data.data(), &val, 4);
    setBusy(true);
    QString err;
    if (sdo_->sdoWrite(NODE_ID, 0x600E, 0, data, err))
        log(QString("WRITE 0x600E (closeToTorque) = %1 Нм OK").arg(val, 0, 'f', 1));
    else
        log(QString("[ERR] closeToTorque: ") + err);
    setBusy(false);
}

void AnviDriveTab::onCmdSetZero()
{
    float val = 0.0f;
    QByteArray data(4, 0);
    memcpy(data.data(), &val, 4);
    setBusy(true);
    QString err;
    if (sdo_->sdoWrite(NODE_ID, 0x600C, 0, data, err))
        log(QString::fromUtf8("WRITE 0x600C (setZero) = 0 OK"));
    else
        log(QString("[ERR] setZero: ") + err);
    setBusy(false);
}

void AnviDriveTab::onCmdOpenVT()
{
    float val = static_cast<float>(openTorqueSpin_->value());
    QByteArray data(4, 0);
    memcpy(data.data(), &val, 4);
    setBusy(true);
    QString err;
    if (sdo_->sdoWrite(NODE_ID, 0x600F, 0, data, err))
        log(QString("WRITE 0x600F (openToTorque) = %1 Нм OK").arg(val, 0, 'f', 1));
    else
        log(QString("[ERR] openToTorque: ") + err);
    setBusy(false);
}

void AnviDriveTab::onCmdCalcVT()
{
    float val = static_cast<float>(calcVTSpin_->value());
    QByteArray data(4, 0);
    memcpy(data.data(), &val, 4);
    setBusy(true);
    QString err;
    if (sdo_->sdoWrite(NODE_ID, 0x6010, 0, data, err))
        log(QString("WRITE 0x6010 (calcVT) = %1 оборотов OK").arg(val, 0, 'f', 0));
    else
        log(QString("[ERR] calcVT: ") + err);
    setBusy(false);
}

void AnviDriveTab::log(const QString& msg)
{
    const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    cmdLog_->append(QString("[%1] %2").arg(ts, msg));
}

void AnviDriveTab::setBusy(bool busy)
{
    openBtn_->setEnabled(!busy);
    stopBtn_->setEnabled(!busy);
    closeBtn_->setEnabled(!busy);
    percentBtn_->setEnabled(!busy);
    closeVTBtn_->setEnabled(!busy);
    setZeroBtn_->setEnabled(!busy);
    openVTBtn_->setEnabled(!busy);
    calcVTBtn_->setEnabled(!busy);
}
