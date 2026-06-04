#include "ControlTab.h"
#include <cstdint>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDateTime>
#include <QFont>
#include <QFrame>

static QPushButton* makeBtn(const QString& text, QWidget* parent, bool danger = false)
{
    auto* btn = new QPushButton(text, parent);
    btn->setFixedHeight(24);
    btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    if (danger) {
        btn->setStyleSheet(
            "QPushButton { font-size: 11px; padding: 1px 8px; font-weight: bold; color: #cc2200; }"
        );
    } else {
        btn->setStyleSheet(
            "QPushButton { font-size: 11px; padding: 1px 8px; }"
        );
    }
    return btn;
}

static QLabel* makeSection(const QString& text, QWidget* parent)
{
    auto* lbl = new QLabel("<b>" + text + "</b>", parent);
    lbl->setStyleSheet("color: #7a4aaa; font-size: 11px; margin-top: 4px;");
    return lbl;
}

ControlTab::ControlTab(CanDriver* can, SdoClient* sdo, QWidget* parent)
    : QWidget(parent), can_(can), sdo_(sdo)
{
    auto* root = new QVBoxLayout(this);
    root->setSpacing(4);
    root->setContentsMargins(8, 8, 8, 8);

    // ---- Single control panel ----
    auto* panel = new QGroupBox(QString::fromUtf8("Управление узлом 0x02"), this);
    auto* grid  = new QGridLayout(panel);
    grid->setContentsMargins(8, 6, 8, 6);
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(4);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    int row = 0;

    // -- NMT --
    grid->addWidget(makeSection(QString::fromUtf8("Сеть"), panel), row++, 0, 1, 2);
    nmtStartBtn_ = makeBtn(QString::fromUtf8("NMT Старт  (0x000 → [01 02])"), panel);
    nmtStartBtn_->setToolTip(
        QString::fromUtf8("Отправляет NMT Start Remote Node на узел 0x02.\n"
                          "Переводит узел в операционное состояние — используй при таймаутах SDO."));
    grid->addWidget(nmtStartBtn_, row++, 0, 1, 2);

    // -- separator --
    auto* sep1 = new QFrame(panel);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color: #ddd;");
    grid->addWidget(sep1, row++, 0, 1, 2);

    // -- Sleep --
    grid->addWidget(makeSection(QString::fromUtf8("Сон"), panel), row++, 0, 1, 2);
    allowSleepBtn_ = makeBtn(QString::fromUtf8("Разрешить сон  (0x6000 = 1)"), panel);
    grid->addWidget(allowSleepBtn_, row++, 0, 1, 2);

    // -- separator --
    auto* sep2 = new QFrame(panel);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color: #ddd;");
    grid->addWidget(sep2, row++, 0, 1, 2);

    // -- SPI --
    grid->addWidget(makeSection(QString::fromUtf8("SPI шина"), panel), row++, 0, 1, 2);
    spiSwitchBtn_  = makeBtn(QString::fromUtf8("Отдать SOM  (0x6005 = 1)"), panel);
    spiAcquireBtn_ = makeBtn(QString::fromUtf8("Вернуть STM32  (0x6005 = 0)"), panel);
    spiSwitchBtn_->setToolTip(QString::fromUtf8("SPI пины → Hi-Z, шина отдаётся SOM."));
    spiAcquireBtn_->setToolTip(QString::fromUtf8("SPI пины → SPI mode, шина возвращается STM32."));
    grid->addWidget(spiSwitchBtn_,  row, 0);
    grid->addWidget(spiAcquireBtn_, row, 1);
    ++row;

    // -- separator --
    auto* sep3 = new QFrame(panel);
    sep3->setFrameShape(QFrame::HLine);
    sep3->setStyleSheet("color: #ddd;");
    grid->addWidget(sep3, row++, 0, 1, 2);

    // -- Ping --
    grid->addWidget(makeSection(QString::fromUtf8("Пинг сервера"), panel), row++, 0, 1, 2);
    pingEnableBtn_  = makeBtn(QString::fromUtf8("Включить  (0x600C = 1)"), panel);
    pingDisableBtn_ = makeBtn(QString::fromUtf8("Отключить  (0x600C = 0)"), panel);
    pingEnableBtn_->setToolTip(
        QString::fromUtf8("ICMP пинг 8.8.8.8 вместо AT+CSQ при следующей проверке связи."));
    pingDisableBtn_->setToolTip(
        QString::fromUtf8("Возврат к стандартной проверке AT+CSQ."));
    grid->addWidget(pingEnableBtn_,  row, 0);
    grid->addWidget(pingDisableBtn_, row, 1);
    ++row;

    // -- separator --
    auto* sep4 = new QFrame(panel);
    sep4->setFrameShape(QFrame::HLine);
    sep4->setStyleSheet("color: #ddd;");
    grid->addWidget(sep4, row++, 0, 1, 2);

    // -- Flash --
    grid->addWidget(makeSection(QString::fromUtf8("Flash"), panel), row++, 0, 1, 2);
    eraseFlashBtn_ = makeBtn(QString::fromUtf8("Стереть Flash  (0x6009 = 1)"), panel, true);
    eraseFlashBtn_->setToolTip(
        QString::fromUtf8("Полное стирание внешней SPI flash AT25SF041B.\n"
                          "Внимание: все данные будут удалены."));
    grid->addWidget(eraseFlashBtn_, row++, 0, 1, 2);

    root->addWidget(panel);

    logView_ = new QTextEdit(this);
    logView_->setReadOnly(true);
    logView_->setFont(QFont("Courier New", 9));
    logView_->setMaximumHeight(160);
    root->addWidget(logView_);
    root->addStretch();

    connect(nmtStartBtn_,   &QPushButton::clicked, this, &ControlTab::onNmtStart);
    connect(allowSleepBtn_, &QPushButton::clicked, this, &ControlTab::onAllowSleep);
    connect(spiSwitchBtn_,  &QPushButton::clicked, this, &ControlTab::onSpiSwitch);
    connect(spiAcquireBtn_, &QPushButton::clicked, this, &ControlTab::onSpiAcquire);
    connect(eraseFlashBtn_, &QPushButton::clicked, this, &ControlTab::onEraseFlash);
    connect(pingEnableBtn_, &QPushButton::clicked, this, &ControlTab::onPingEnable);
    connect(pingDisableBtn_,&QPushButton::clicked, this, &ControlTab::onPingDisable);
}

void ControlTab::onNmtStart()
{
    uint8_t payload[8] = {0x01, NODE_ID, 0, 0, 0, 0, 0, 0};
    if (can_->transmit(NMT_COB_ID, payload)) {
        log(QString::fromUtf8("[OK] NMT Старт отправлен на узел 0x02"));
    } else {
        log(QString::fromUtf8("[ERR] NMT Старт не отправлен — CAN не подключён?"));
    }
}

void ControlTab::onAllowSleep()
{
    setBusy(true);
    QString err;
    if (sdo_->writeInt8(NODE_ID, OD_ALLOW_SLEEP, 0, 1, err)) {
        log(QString::fromUtf8("[OK] Разрешение сна отправлено (0x6000 = 1)"));
    } else {
        log("[ERR] " + err);
    }
    setBusy(false);
}

void ControlTab::onSpiSwitch()
{
    setBusy(true);
    QString err;
    if (sdo_->writeUInt8(NODE_ID, OD_SPI_SWITCH, 0, 1, err)) {
        log(QString::fromUtf8("[OK] SPI Release: 0x6005 = 1 — шина отдана SOM (Hi-Z)"));
    } else {
        log("[ERR] " + err);
    }
    setBusy(false);
}

void ControlTab::onSpiAcquire()
{
    setBusy(true);
    QString err;
    if (sdo_->writeUInt8(NODE_ID, OD_SPI_SWITCH, 0, 0, err)) {
        log(QString::fromUtf8("[OK] SPI Acquire: 0x6005 = 0 — шина возвращена STM32"));
    } else {
        log("[ERR] " + err);
    }
    setBusy(false);
}

void ControlTab::onEraseFlash()
{
    setBusy(true);
    QString err;
    if (sdo_->writeUInt8(NODE_ID, OD_ERASE_FLASH, 0, 1, err)) {
        log(QString::fromUtf8("[OK] Flash Erase: 0x6009 = 1 — стирание запущено"));
    } else {
        log("[ERR] " + err);
    }
    setBusy(false);
}

void ControlTab::onPingEnable()
{
    setBusy(true);
    QString err;
    if (sdo_->writeUInt8(NODE_ID, OD_PING_MODE, 0, 1, err)) {
        log(QString::fromUtf8("[OK] Ping mode ON: 0x600C = 1 — пинг 8.8.8.8 активирован"));
    } else {
        log("[ERR] " + err);
    }
    setBusy(false);
}

void ControlTab::onPingDisable()
{
    setBusy(true);
    QString err;
    if (sdo_->writeUInt8(NODE_ID, OD_PING_MODE, 0, 0, err)) {
        log(QString::fromUtf8("[OK] Ping mode OFF: 0x600C = 0 — возврат к AT+CSQ"));
    } else {
        log("[ERR] " + err);
    }
    setBusy(false);
}

void ControlTab::log(const QString& msg)
{
    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
    logView_->append(QString("[%1] %2").arg(ts, msg));
}

void ControlTab::setBusy(bool busy)
{
    nmtStartBtn_->setEnabled(!busy);
    allowSleepBtn_->setEnabled(!busy);
    spiSwitchBtn_->setEnabled(!busy);
    spiAcquireBtn_->setEnabled(!busy);
    eraseFlashBtn_->setEnabled(!busy);
    pingEnableBtn_->setEnabled(!busy);
    pingDisableBtn_->setEnabled(!busy);
}
