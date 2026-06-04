#include "AnviEnergyTab.h"
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
#include <QSpinBox>
#include <QStyle>
#include <QIcon>
#include <QCursor>
#include <QSize>
#include <QTabBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include "../can/CanDriver.h"
#include "../canopen/SdoClient.h"

AnviEnergyTab::AnviEnergyTab(CanDriver* can, SdoClient* sdo, QWidget* parent)
    : QWidget(parent), sdo_(sdo)
{
    setObjectName("modulePage");

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 12, 0, 12);
    rootLayout->setSpacing(16);

    static const QIcon readIcon(":/resources/icon_read.png");
    static const QIcon writeIcon(":/resources/icon_write.png");
    auto makeIconButton = [&](const QIcon& icon, QWidget* parent) {
        auto* btn = new QPushButton(parent);
        btn->setFixedSize(55, 55);
        btn->setIcon(icon);
        btn->setIconSize(QSize(50, 50));
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setStyleSheet("background: transparent; border: none;");
        return btn;
    };

    // --- Hero card ---
    auto* heroCard = new QFrame(this);
    heroCard->setObjectName("heroCard");
    auto* heroLayout = new QVBoxLayout(heroCard);
    heroLayout->setContentsMargins(24, 22, 24, 22);
    heroLayout->setSpacing(12);

    auto* titleLabel = new QLabel(QString::fromUtf8("AnviEnergy"), heroCard);
    titleLabel->setObjectName("pageTitle");
    heroLayout->addWidget(titleLabel);

    auto* stateRow = new QHBoxLayout();
    stateRow->setSpacing(10);

    canStateBadge_ = new QLabel(QString::fromUtf8("Модуль не подключен"), heroCard);
    canStateBadge_->setObjectName("offlineBadge");
    stateRow->addWidget(canStateBadge_, 0, Qt::AlignLeft);

    canStateHint_ = new QLabel(
        QString::fromUtf8("Модуль 0x0A не ответил на NMT-команду сканирования."),
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

    auto* telGroup = new QGroupBox(QString::fromUtf8("Телеметрия AnviEnergy"), telPage);
    auto* telForm = new QFormLayout(telGroup);
    telForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    tempElecLabel_   = new QLabel(QString::fromUtf8("—"), telGroup);
    humElecLabel_    = new QLabel(QString::fromUtf8("—"), telGroup);
    tempAkkuLabel_   = new QLabel(QString::fromUtf8("—"), telGroup);
    socLabel_        = new QLabel(QString::fromUtf8("—"), telGroup);
    ch1CurrentLabel_  = new QLabel(QString::fromUtf8("—"), telGroup);
    ch2CurrentLabel_  = new QLabel(QString::fromUtf8("—"), telGroup);
    battVoltLabel_    = new QLabel(QString::fromUtf8("—"), telGroup);
    outVoltLabel_     = new QLabel(QString::fromUtf8("—"), telGroup);
    moduleStateLabel_ = new QLabel(QString::fromUtf8("—"), telGroup);

    telForm->addRow(QString::fromUtf8("Состояние модуля:"),        moduleStateLabel_);
    telForm->addRow(QString::fromUtf8("Температура электроники:"), tempElecLabel_);
    telForm->addRow(QString::fromUtf8("Влажность электроники:"),   humElecLabel_);
    telForm->addRow(QString::fromUtf8("Температура АКБ:"),         tempAkkuLabel_);
    telForm->addRow(QString::fromUtf8("Заряд АКБ:"),               socLabel_);
    telForm->addRow(QString::fromUtf8("Ток канала 1:"),            ch1CurrentLabel_);
    telForm->addRow(QString::fromUtf8("Ток канала 2:"),            ch2CurrentLabel_);
    telForm->addRow(QString::fromUtf8("Напряжение АКБ:"),          battVoltLabel_);
    telForm->addRow(QString::fromUtf8("Выходное напряжение:"),     outVoltLabel_);

    telLayout->addWidget(telGroup);

    auto* statGroup = new QGroupBox(QString::fromUtf8("Статус"), telPage);
    auto* statForm = new QFormLayout(statGroup);

    lastUpdateLabel_ = new QLabel(QString::fromUtf8("—"), statGroup);
    pdoCountLabel_   = new QLabel("0", statGroup);

    statForm->addRow(QString::fromUtf8("Последнее обновление:"), lastUpdateLabel_);
    statForm->addRow(QString::fromUtf8("Принято PDO:"),          pdoCountLabel_);

    telLayout->addWidget(statGroup);
    telLayout->addStretch();

    toolTabs_->addTab(telPage, QString::fromUtf8("Телеметрия"));

    // ---- Config tab ----
    auto* cfgPage = new QWidget(this);
    auto* cfgLayout = new QVBoxLayout(cfgPage);
    cfgLayout->setContentsMargins(16, 16, 16, 16);
    cfgLayout->setSpacing(12);

    auto* cfgGroup = new QGroupBox(QString::fromUtf8("Параметры AnviEnergy"), cfgPage);
    auto* cfgForm = new QFormLayout(cfgGroup);
    cfgForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    auto makeDoubleSpin = [&](double min, double max, double step, int decimals) {
        auto* s = new QDoubleSpinBox(cfgGroup);
        s->setRange(min, max);
        s->setSingleStep(step);
        s->setDecimals(decimals);
        return s;
    };

    maxOverVoltDischSpin_    = makeDoubleSpin(0.0, 100.0,  0.1, 3);
    maxUnderVoltDischSpin_   = makeDoubleSpin(0.0, 100.0,  0.1, 3);
    voltMaxChargeSpin_       = makeDoubleSpin(0.0, 100.0,  0.1, 3);
    maxUnderVoltChargeSpin_  = makeDoubleSpin(0.0, 100.0,  0.1, 3);
    diffCurrentSpin_         = makeDoubleSpin(0.0, 1000.0, 0.1, 3);
    maxChargeCurrentSpin_    = makeDoubleSpin(0.0, 1000.0, 0.1, 3);
    maxDischargeCurrentSpin_ = makeDoubleSpin(0.0, 1000.0, 0.1, 3);
    batteryCapacity_         = makeDoubleSpin(0.0, 1000.0, 1.0, 3);
    timeOfTurnOnSpin_ = new QSpinBox(cfgGroup);
    timeOfTurnOnSpin_->setRange(0, 65535);
    timeOfTurnOnSpin_->setSingleStep(1);

    // Wrap spinbox with individual ← (read) and → (write) buttons
    auto makeFloatRow = [&](QDoubleSpinBox* spin, uint16_t idx) -> QWidget* {
        auto* w = new QWidget(cfgGroup);
        auto* h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(4);
        h->addWidget(spin, 1);
        auto* rBtn = makeIconButton(readIcon, w);
        rBtn->setToolTip(QString::fromUtf8("Считать из устройства"));
        auto* wBtn = makeIconButton(writeIcon, w);
        wBtn->setToolTip(QString::fromUtf8("Записать в устройство"));
        h->addWidget(rBtn);
        h->addWidget(wBtn);
        connect(rBtn, &QPushButton::clicked, this, [this, spin, idx]() {
            QByteArray raw; QString err;
            if (sdo_->sdoRead(NODE_ID, idx, 0, raw, err) && raw.size() >= 4) {
                float val = 0.0f;
                memcpy(&val, raw.constData(), 4);
                spin->setValue(static_cast<double>(val));
                log(QString("READ  0x%1 = %2").arg(idx, 4, 16, QChar('0')).arg(val, 0, 'f', 3));
            } else {
                log(QString("[ERR] READ 0x%1: ").arg(idx, 4, 16, QChar('0')) + err);
            }
        });
        connect(wBtn, &QPushButton::clicked, this, [this, spin, idx]() {
            float f = static_cast<float>(spin->value());
            QByteArray data(4, 0);
            memcpy(data.data(), &f, 4);
            QString err;
            if (sdo_->sdoWrite(NODE_ID, idx, 0, data, err))
                log(QString("WRITE 0x%1 = %2 OK").arg(idx, 4, 16, QChar('0')).arg(spin->value(), 0, 'f', 3));
            else
                log(QString("[ERR] WRITE 0x%1: ").arg(idx, 4, 16, QChar('0')) + err);
        });
        return w;
    };

    auto makeUInt16Row = [&](QSpinBox* spin, uint16_t idx) -> QWidget* {
        auto* w = new QWidget(cfgGroup);
        auto* h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(4);
        h->addWidget(spin, 1);
        auto* rBtn = makeIconButton(readIcon, w);
        rBtn->setToolTip(QString::fromUtf8("Считать из устройства"));
        auto* wBtn = makeIconButton(writeIcon, w);
        wBtn->setToolTip(QString::fromUtf8("Записать в устройство"));
        h->addWidget(rBtn);
        h->addWidget(wBtn);
        connect(rBtn, &QPushButton::clicked, this, [this, spin, idx]() {
            uint16_t val = 0; QString err;
            if (sdo_->readUInt16(NODE_ID, idx, 0, val, err)) {
                spin->setValue(static_cast<int>(val));
                log(QString("READ  0x%1 = %2").arg(idx, 4, 16, QChar('0')).arg(val));
            } else {
                log(QString("[ERR] READ 0x%1: ").arg(idx, 4, 16, QChar('0')) + err);
            }
        });
        connect(wBtn, &QPushButton::clicked, this, [this, spin, idx]() {
            uint16_t v = static_cast<uint16_t>(spin->value());
            QString err;
            if (sdo_->writeUInt16(NODE_ID, idx, 0, v, err))
                log(QString("WRITE 0x%1 = %2 OK").arg(idx, 4, 16, QChar('0')).arg(v));
            else
                log(QString("[ERR] WRITE 0x%1: ").arg(idx, 4, 16, QChar('0')) + err);
        });
        return w;
    };

    cfgForm->addRow(QString::fromUtf8("Макс. напряжение разрядки (0x6009, В):"),   makeFloatRow(maxOverVoltDischSpin_,    0x6009));
    cfgForm->addRow(QString::fromUtf8("Мин. напряжение разрядки (0x6008, В):"),    makeFloatRow(maxUnderVoltDischSpin_,   0x6008));
    cfgForm->addRow(QString::fromUtf8("Макс. напряжение зарядки (0x6006, В):"),    makeFloatRow(voltMaxChargeSpin_,       0x6006));
    cfgForm->addRow(QString::fromUtf8("Мин. напряжение зарядки (0x6007, В):"),     makeFloatRow(maxUnderVoltChargeSpin_,  0x6007));
    cfgForm->addRow(QString::fromUtf8("Время работы BMS (0x600A, мс):"),            makeUInt16Row(timeOfTurnOnSpin_,       0x600A));
    cfgForm->addRow(QString::fromUtf8("Макс. разница токов каналов (0x600B, А):"), makeFloatRow(diffCurrentSpin_,         0x600B));
    cfgForm->addRow(QString::fromUtf8("Макс. ток зарядки (0x6015, А):"),            makeFloatRow(maxChargeCurrentSpin_,   0x6015));
    cfgForm->addRow(QString::fromUtf8("Макс. ток разрядки (0x6016, А):"),           makeFloatRow(maxDischargeCurrentSpin_,0x6016));
    cfgForm->addRow(QString::fromUtf8("Ёмкость батареи (0x6024, А·ч):"),            makeFloatRow(batteryCapacity_,         0x6024));
    cfgLayout->addWidget(cfgGroup);

    {
        auto* row = new QHBoxLayout();
        readConfigBtn_  = new QPushButton(QString::fromUtf8("Считать всё"),  cfgPage);
        sendConfigBtn_  = new QPushButton(QString::fromUtf8("Записать всё"), cfgPage);
        saveToFlashBtn_ = new QPushButton(QString::fromUtf8("Сохранить"), cfgPage);
        readConfigBtn_->setMinimumWidth(100);
        sendConfigBtn_->setMinimumWidth(100);
        saveToFlashBtn_->setMinimumWidth(100);
        flashStatusLabel_ = new QLabel(cfgPage);
        flashStatusLabel_->hide();
        row->addWidget(readConfigBtn_);
        row->addWidget(sendConfigBtn_);
        row->addWidget(saveToFlashBtn_);
        row->addWidget(flashStatusLabel_);
        row->addStretch();
        cfgLayout->addLayout(row);

        flashStatusTimer_ = new QTimer(cfgPage);
        flashStatusTimer_->setSingleShot(true);
        flashStatusTimer_->setInterval(3000);
        connect(flashStatusTimer_, &QTimer::timeout,
                flashStatusLabel_, &QLabel::hide);
    }

    cmdLog_ = new QTextEdit(cfgPage);
    cmdLog_->setReadOnly(true);
    cmdLog_->setFont(QFont("Courier New", 9));
    cfgLayout->addWidget(cmdLog_, 1);

    toolTabs_->addTab(cfgPage, QString::fromUtf8("Конфигурация (SDO)"));

    // ---- Calibration tab ----
    auto* calPage = new QWidget(this);
    auto* calLayout = new QVBoxLayout(calPage);
    calLayout->setContentsMargins(16, 16, 16, 16);
    calLayout->setSpacing(12);

    auto* calGroup = new QGroupBox(QString::fromUtf8("Калибровочные эталоны"), calPage);
    auto* calForm  = new QFormLayout(calGroup);
    calForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    packVoltCalSpin_ = new QDoubleSpinBox(calGroup);
    packVoltCalSpin_->setRange(-9999.999, 9999.999);
    packVoltCalSpin_->setSingleStep(0.001);
    packVoltCalSpin_->setDecimals(3);
    packVoltCalSpin_->setSuffix(QString::fromUtf8(" В"));

    outVoltCalSpin_ = new QDoubleSpinBox(calGroup);
    outVoltCalSpin_->setRange(-9999.999, 9999.999);
    outVoltCalSpin_->setSingleStep(0.001);
    outVoltCalSpin_->setDecimals(3);
    outVoltCalSpin_->setSuffix(QString::fromUtf8(" В"));

    // Float row helper that logs to calLog_ (not cmdLog_)
    auto makeFloatRowCal = [&](QDoubleSpinBox* spin, uint16_t idx) -> QWidget* {
        auto* w = new QWidget(calGroup);
        auto* h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(4);
        h->addWidget(spin, 1);
        auto* rBtn = makeIconButton(readIcon, w);
        rBtn->setToolTip(QString::fromUtf8("Считать из устройства"));
        auto* wBtn = makeIconButton(writeIcon, w);
        wBtn->setToolTip(QString::fromUtf8("Записать в устройство"));
        h->addWidget(rBtn);
        h->addWidget(wBtn);
        connect(rBtn, &QPushButton::clicked, this, [this, spin, idx]() {
            QByteArray raw; QString err;
            const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            if (sdo_->sdoRead(NODE_ID, idx, 0, raw, err) && raw.size() >= 4) {
                float val = 0.0f;
                memcpy(&val, raw.constData(), 4);
                spin->setValue(static_cast<double>(val));
                calLog_->append(QString("[%1] READ  0x%2 = %3")
                    .arg(ts).arg(idx, 4, 16, QChar('0')).arg(val, 0, 'f', 3));
            } else {
                calLog_->append(QString("[%1] [ERR] READ 0x%2: ")
                    .arg(ts).arg(idx, 4, 16, QChar('0')) + err);
            }
        });
        connect(wBtn, &QPushButton::clicked, this, [this, spin, idx]() {
            float f = static_cast<float>(spin->value());
            QByteArray data(4, 0);
            memcpy(data.data(), &f, 4);
            QString err;
            const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            if (sdo_->sdoWrite(NODE_ID, idx, 0, data, err))
                calLog_->append(QString("[%1] WRITE 0x%2 = %3 OK")
                    .arg(ts).arg(idx, 4, 16, QChar('0')).arg(spin->value(), 0, 'f', 3));
            else
                calLog_->append(QString("[%1] [ERR] WRITE 0x%2: ")
                    .arg(ts).arg(idx, 4, 16, QChar('0')) + err);
        });
        return w;
    };

    calForm->addRow(QString::fromUtf8("Калибровка напряжения паков (0x6018):"),
                    makeFloatRowCal(packVoltCalSpin_, 0x6018));
    calForm->addRow(QString::fromUtf8("Калибровка выходного напряжения (0x6019):"),
                    makeFloatRowCal(outVoltCalSpin_,  0x6019));

    calLayout->addWidget(calGroup);

    // --- Current calibration ---
    auto* curCalGroup = new QGroupBox(QString::fromUtf8("Калибровка тока по всем каналам"), calPage);
    auto* curCalLayout = new QVBoxLayout(curCalGroup);
    curCalLayout->setContentsMargins(12, 10, 12, 10);
    curCalLayout->setSpacing(8);

    auto* noLoadWarning = new QLabel(
        QString::fromUtf8("Выполнять калибровку тока только без нагрузки!"),
        curCalGroup);
    noLoadWarning->setStyleSheet("color: #000000; font-weight: 600;");
    noLoadWarning->setWordWrap(true);
    curCalLayout->addWidget(noLoadWarning);

    {
        auto* row = new QHBoxLayout();
        calCurrentBtn_ = new QPushButton(QString::fromUtf8("Калибровать ток"), curCalGroup);
        calCurrentBtn_->setMinimumWidth(150);
        calCurrentStatusLabel_ = new QLabel(curCalGroup);
        calCurrentStatusLabel_->hide();
        row->addWidget(calCurrentBtn_);
        row->addWidget(calCurrentStatusLabel_);
        row->addStretch();
        curCalLayout->addLayout(row);

        calCurrentStatusTimer_ = new QTimer(curCalGroup);
        calCurrentStatusTimer_->setSingleShot(true);
        calCurrentStatusTimer_->setInterval(3000);
        connect(calCurrentStatusTimer_, &QTimer::timeout, calCurrentStatusLabel_, &QLabel::hide);
    }

    calLayout->addWidget(curCalGroup);

    connect(calCurrentBtn_, &QPushButton::clicked, this, [this]() {
        calCurrentBtn_->setEnabled(false);
        QString err;
        bool ok = sdo_->writeUInt8(NODE_ID, 0x6014, 0, 2, err);
        const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        if (ok) {
            const QString msg = QString::fromUtf8("WRITE 0x6014 (calibrateCurrent) = 2 OK");
            log(msg);
            calLog_->append(QString("[%1] %2").arg(ts, msg));
            calCurrentStatusLabel_->setText(QString::fromUtf8("Успешно"));
            calCurrentStatusLabel_->setStyleSheet("color: #1E7D52; font-weight: 600;");
        } else {
            const QString msg = QString::fromUtf8("[ERR] 0x6014 (calibrateCurrent): ") + err;
            log(msg);
            calLog_->append(QString("[%1] %2").arg(ts, msg));
            calCurrentStatusLabel_->setText(QString::fromUtf8("Ошибка"));
            calCurrentStatusLabel_->setStyleSheet("color: #B5472C; font-weight: 600;");
        }
        calCurrentStatusLabel_->show();
        calCurrentStatusTimer_->start();
        calCurrentBtn_->setEnabled(true);
    });

    // --- Save calibration to flash ---
    {
        auto* row = new QHBoxLayout();
        calSaveBtn_    = new QPushButton(QString::fromUtf8("Сохранить"), calPage);
        calSaveBtn_->setMinimumWidth(110);
        calStatusLabel_ = new QLabel(calPage);
        calStatusLabel_->hide();
        row->addWidget(calSaveBtn_);
        row->addWidget(calStatusLabel_);
        row->addStretch();
        calLayout->addLayout(row);

        calStatusTimer_ = new QTimer(calPage);
        calStatusTimer_->setSingleShot(true);
        calStatusTimer_->setInterval(3000);
        connect(calStatusTimer_, &QTimer::timeout, calStatusLabel_, &QLabel::hide);
    }

    calLog_ = new QTextEdit(calPage);
    calLog_->setReadOnly(true);
    calLog_->setFont(QFont("Courier New", 9));
    calLayout->addWidget(calLog_, 1);

    connect(calSaveBtn_, &QPushButton::clicked, this, [this]() {
        calSaveBtn_->setEnabled(false);
        QString err;
        bool ok = sdo_->writeUInt8(NODE_ID, 0x6014, 0, 1, err);
        const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        if (ok) {
            const QString msg = QString::fromUtf8("WRITE 0x6014 (saveToFlash) = 1 OK [calibration]");
            log(msg);
            calLog_->append(QString("[%1] %2").arg(ts, msg));
            calStatusLabel_->setText(QString::fromUtf8("Успешно"));
            calStatusLabel_->setStyleSheet("color: #1E7D52; font-weight: 600;");
        } else {
            const QString msg = QString::fromUtf8("[ERR] 0x6014 (calibration save): ") + err;
            log(msg);
            calLog_->append(QString("[%1] %2").arg(ts, msg));
            calStatusLabel_->setText(QString::fromUtf8("Ошибка"));
            calStatusLabel_->setStyleSheet("color: #B5472C; font-weight: 600;");
        }
        calStatusLabel_->show();
        calStatusTimer_->start();
        calSaveBtn_->setEnabled(true);
    });

    toolTabs_->addTab(calPage, QString::fromUtf8("Калибровка"));

    firmwareTab_ = new FirmwareTab(can, NODE_ID, this);
    toolTabs_->addTab(firmwareTab_, QString::fromUtf8("Прошивка"));

    // Connect signals
    connect(can, &CanDriver::frameReceived, this, &AnviEnergyTab::onCanFrame);
    connect(readConfigBtn_,  &QPushButton::clicked, this, &AnviEnergyTab::onReadConfig);
    connect(sendConfigBtn_,  &QPushButton::clicked, this, &AnviEnergyTab::onSendConfig);
    connect(saveToFlashBtn_, &QPushButton::clicked, this, &AnviEnergyTab::onSaveToFlash);

    // SDO auto-poll timer (500 ms)
    pollTimer_ = new QTimer(this);
    pollTimer_->setInterval(300);
    connect(pollTimer_, &QTimer::timeout, this, &AnviEnergyTab::onSdoPoll);

    setModuleConnected(false);
}

void AnviEnergyTab::setModuleConnected(bool connected)
{
    const bool wasConnected = connected_;
    connected_ = connected;

    canStateBadge_->setText(connected
        ? QString::fromUtf8("Модуль подключен")
        : QString::fromUtf8("Модуль не подключен"));
    canStateBadge_->setObjectName(connected ? "onlineBadge" : "offlineBadge");
    canStateBadge_->style()->unpolish(canStateBadge_);
    canStateBadge_->style()->polish(canStateBadge_);

    canStateHint_->setText(connected
        ? QString::fromUtf8("Модуль 0x0A найден в сети. Данные обновляются каждые 300 мс.")
        : QString::fromUtf8("Модуль 0x0A не ответил на NMT-команду сканирования."));

    if (connected && !wasConnected) {
        pollTimer_->start();
        QMetaObject::invokeMethod(this, &AnviEnergyTab::onReadConfig, Qt::QueuedConnection);
    } else if (!connected && wasConnected) {
        pollTimer_->stop();
    }
}

void AnviEnergyTab::onCanFrame(uint32_t cobId, QByteArray data)
{
    if (cobId == COB_TEMPS && data.size() >= 6) {
        int16_t tempElec = 0;
        int16_t humElec  = 0;
        int16_t tempAkku = 0;
        memcpy(&tempElec, data.constData() + 0, 2);
        memcpy(&humElec,  data.constData() + 2, 2);
        memcpy(&tempAkku, data.constData() + 4, 2);
        tempElecLabel_->setText(QString::number(tempElec / 10.0, 'f', 1) + QString::fromUtf8(" °C"));
        humElecLabel_->setText(QString::number(humElec / 10.0, 'f', 1)   + QString::fromUtf8(" %"));
        tempAkkuLabel_->setText(QString::number(tempAkku / 10.0, 'f', 1) + QString::fromUtf8(" °C"));
        pdoCount_++;
        pdoCountLabel_->setText(QString::number(pdoCount_));
        lastUpdateLabel_->setText(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));
        return;
    }

    if (cobId == COB_SOC && data.size() >= 4) {
        int32_t soc = 0;
        memcpy(&soc, data.constData(), 4);
        socLabel_->setText(QString::number(soc) + QString::fromUtf8(" %"));
        pdoCount_++;
        pdoCountLabel_->setText(QString::number(pdoCount_));
        lastUpdateLabel_->setText(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));
        return;
    }
}

void AnviEnergyTab::onSendConfig()
{
    setBusy(true);
    QString err;

    auto writeF = [&](uint16_t idx, double val, const char* name) {
        float f = static_cast<float>(val);
        QByteArray data(4, 0);
        memcpy(data.data(), &f, 4);
        if (sdo_->sdoWrite(NODE_ID, idx, 0, data, err))
            log(QString("WRITE 0x%1 (%2) = %3 OK")
                    .arg(idx, 4, 16, QChar('0'))
                    .arg(name)
                    .arg(val, 0, 'f', 3));
        else
            log(QString("[ERR] 0x%1 (%2): ")
                    .arg(idx, 4, 16, QChar('0'))
                    .arg(name) + err);
    };

    writeF(0x6009, maxOverVoltDischSpin_->value(),   "maxOverVoltageDischarge");
    writeF(0x6008, maxUnderVoltDischSpin_->value(),  "maxUnderVoltageDischage");
    writeF(0x6006, voltMaxChargeSpin_->value(),      "VOLTAGE_MAX_CHARGE");
    writeF(0x6007, maxUnderVoltChargeSpin_->value(), "maxUnderVoltageCharge");

    uint16_t t = static_cast<uint16_t>(timeOfTurnOnSpin_->value());
    if (sdo_->writeUInt16(NODE_ID, 0x600A, 0, t, err))
        log(QString("WRITE 0x600A (timeofturnonPower) = %1 OK").arg(t));
    else
        log(QString("[ERR] 0x600A (timeofturnonPower): ") + err);

    writeF(0x600B, diffCurrentSpin_->value(),         "differenceCurrent");
    writeF(0x6015, maxChargeCurrentSpin_->value(),    "maxChargeCurrent");
    writeF(0x6016, maxDischargeCurrentSpin_->value(), "maxDischargeCurrent");
    writeF(0x6024, batteryCapacity_->value(),         "batteryCapacity");

    setBusy(false);
}

void AnviEnergyTab::log(const QString& msg)
{
    const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    cmdLog_->append(QString("[%1] %2").arg(ts, msg));
}

void AnviEnergyTab::onReadConfig()
{
    setBusy(true);

    auto readF = [&](QDoubleSpinBox* spin, uint16_t idx) {
        QByteArray raw; QString err;
        if (sdo_->sdoRead(NODE_ID, idx, 0, raw, err) && raw.size() >= 4) {
            float val = 0.0f;
            memcpy(&val, raw.constData(), 4);
            spin->setValue(static_cast<double>(val));
            log(QString("READ  0x%1 = %2").arg(idx, 4, 16, QChar('0')).arg(val, 0, 'f', 3));
        } else {
            log(QString("[ERR] READ 0x%1: ").arg(idx, 4, 16, QChar('0')) + err);
        }
    };

    readF(maxOverVoltDischSpin_,    0x6009);
    readF(maxUnderVoltDischSpin_,   0x6008);
    readF(voltMaxChargeSpin_,       0x6006);
    readF(maxUnderVoltChargeSpin_,  0x6007);

    {
        uint16_t val = 0; QString err;
        if (sdo_->readUInt16(NODE_ID, 0x600A, 0, val, err)) {
            timeOfTurnOnSpin_->setValue(static_cast<int>(val));
            log(QString("READ  0x600A = %1").arg(val));
        } else {
            log(QString("[ERR] READ 0x600A: ") + err);
        }
    }

    readF(diffCurrentSpin_,         0x600B);
    readF(maxChargeCurrentSpin_,    0x6015);
    readF(maxDischargeCurrentSpin_, 0x6016);
    readF(batteryCapacity_,         0x6024);
    setBusy(false);
}

void AnviEnergyTab::onSaveToFlash()
{
    QString err;
    saveToFlashBtn_->setEnabled(false);
    bool ok = sdo_->writeUInt8(NODE_ID, 0x6014, 0, 1, err);
    if (ok) {
        log(QString::fromUtf8("WRITE 0x6014 (saveToFlash) = 1 OK"));
        flashStatusLabel_->setText(QString::fromUtf8("Успешно"));
        flashStatusLabel_->setStyleSheet("color: #1E7D52; font-weight: 600;");
    } else {
        log(QString::fromUtf8("[ERR] 0x6014 (saveToFlash): ") + err);
        flashStatusLabel_->setText(QString::fromUtf8("Ошибка"));
        flashStatusLabel_->setStyleSheet("color: #B5472C; font-weight: 600;");
    }
    flashStatusLabel_->show();
    flashStatusTimer_->start();
    saveToFlashBtn_->setEnabled(true);
}

void AnviEnergyTab::setBusy(bool busy)
{
    readConfigBtn_->setEnabled(!busy);
    sendConfigBtn_->setEnabled(!busy);
    saveToFlashBtn_->setEnabled(!busy);
}

static QString moduleStateText(uint16_t state)
{
    switch (state) {
        case  0: return QString::fromUtf8("0 — Инициализация");
        case  1: return QString::fromUtf8("1 — Зарядка");
        case  2: return QString::fromUtf8("2 — Предварительная зарядка");
        case  3: return QString::fromUtf8("3 — Нагрузка включена");
        case  5: return QString::fromUtf8("5 — Выключение");
        case  7: return QString::fromUtf8("7 — Ошибка");
        case  8: return QString::fromUtf8("8 — Ошибка предварительной зарядки");
        case 10: return QString::fromUtf8("10 — Заряжен");
        default: return QString::fromUtf8("Неизвестно (%1)").arg(state);
    }
}

void AnviEnergyTab::onSdoPoll()
{
    if (sdoBusy_) return;
    sdoBusy_ = true;

    QString err;

    // Poll float values: 0x600F, 0x6010, 0x6011, 0x6012
    static const struct { uint16_t idx; const char* unit; } floatEntries[] = {
        { 0x600F, " А" },
        { 0x6010, " А" },
        { 0x6011, " В" },
        { 0x6012, " В" },
    };
    QLabel* floatLabels[] = { ch1CurrentLabel_, ch2CurrentLabel_, battVoltLabel_, outVoltLabel_ };

    for (int i = 0; i < 4; ++i) {
        QByteArray raw;
        if (sdo_->sdoRead(NODE_ID, floatEntries[i].idx, 0, raw, err) && raw.size() >= 4) {
            float val = 0.0f;
            memcpy(&val, raw.constData(), 4);
            floatLabels[i]->setText(
                QString::number(static_cast<double>(val), 'f', 3) +
                QString::fromUtf8(floatEntries[i].unit));
        } else {
            floatLabels[i]->setText(QString::fromUtf8("Err"));
        }
    }

    // Poll module state: 0x6013 (uint16)
    {
        uint16_t state = 0;
        if (sdo_->readUInt16(NODE_ID, 0x6013, 0, state, err))
            moduleStateLabel_->setText(moduleStateText(state));
        else
            moduleStateLabel_->setText(QString::fromUtf8("Err"));
    }

    sdoBusy_ = false;
}
