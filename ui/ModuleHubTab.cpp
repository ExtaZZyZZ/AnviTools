#include "ModuleHubTab.h"
#include "AutonomousModuleTab.h"
#include "AnviDriveTab.h"
#include "AnviEnergyTab.h"
#include "CanMonitorPanel.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include "../can/CanDriver.h"
#include "../canopen/SdoClient.h"

namespace {

QWidget* makeSummaryRow(const QString& title,
                        QLabel** badgeOut,
                        QLabel** infoOut,
                        QWidget* parent)
{
    auto* rowWidget = new QWidget(parent);
    auto* rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(12);

    auto* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);

    auto* titleLabel = new QLabel(title, rowWidget);
    titleLabel->setStyleSheet("font-weight: 600;");
    textLayout->addWidget(titleLabel);

    auto* infoLabel = new QLabel(rowWidget);
    infoLabel->setObjectName("hintText");
    infoLabel->setWordWrap(true);
    textLayout->addWidget(infoLabel);

    auto* badge = new QLabel(rowWidget);
    badge->setObjectName("offlineBadge");

    rowLayout->addLayout(textLayout, 1);
    rowLayout->addWidget(badge, 0, Qt::AlignTop);

    *badgeOut = badge;
    *infoOut = infoLabel;
    return rowWidget;
}

} // namespace

ModuleHubTab::ModuleHubTab(CanDriver* can, SdoClient* sdo, QWidget* parent)
    : QWidget(parent)
    , can_(can)
{
    setObjectName("modulePage");

    // ── Root: horizontal splitter ─────────────────────────────────────────────
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(6);
    rootLayout->addWidget(splitter);

    // ── Left panel (scrollable) ───────────────────────────────────────────────
    auto* leftScroll = new QScrollArea();
    leftScroll->setFrameShape(QFrame::NoFrame);
    leftScroll->setWidgetResizable(true);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    leftScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto* leftWidget = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(18, 18, 10, 18);
    leftLayout->setSpacing(14);

    auto* introCard = new QFrame();
    introCard->setObjectName("heroCard");
    auto* introLayout = new QVBoxLayout(introCard);
    introLayout->setContentsMargins(24, 22, 24, 22);
    introLayout->setSpacing(12);

    auto* titleLabel = new QLabel(QString::fromUtf8("Модули"), introCard);
    titleLabel->setObjectName("pageTitle");
    introLayout->addWidget(titleLabel);

    auto* actionRow = new QHBoxLayout();
    actionRow->setSpacing(12);

    scanButton_ = new QPushButton(QString::fromUtf8("Сканировать сеть"), introCard);
    actionRow->addWidget(scanButton_, 0, Qt::AlignLeft);

    scanStatusLabel_ = new QLabel(
        QString::fromUtf8("Подключите CAN на вкладке \"Подключение\", чтобы выполнить сканирование."),
        introCard);
    scanStatusLabel_->setObjectName("hintText");
    scanStatusLabel_->setWordWrap(true);
    actionRow->addWidget(scanStatusLabel_, 1);

    introLayout->addLayout(actionRow);
    introLayout->addWidget(makeSummaryRow(QString::fromUtf8("Автономный модуль"),
                                          &autonomousBadge_,
                                          &autonomousInfo_,
                                          introCard));
    introLayout->addWidget(makeSummaryRow(QString::fromUtf8("AnviDrive"),
                                          &driveBadge_,
                                          &driveInfo_,
                                          introCard));
    introLayout->addWidget(makeSummaryRow(QString::fromUtf8("AnviEnergy"),
                                          &energyBadge_,
                                          &energyInfo_,
                                          introCard));

    leftLayout->addWidget(introCard);

    moduleTabs_ = new QTabWidget(leftWidget);
    moduleTabs_->setObjectName("moduleTabs");
    moduleTabs_->setDocumentMode(true);
    moduleTabs_->setUsesScrollButtons(true);
    moduleTabs_->tabBar()->setExpanding(false);
    moduleTabs_->tabBar()->setElideMode(Qt::ElideNone);
    leftLayout->addWidget(moduleTabs_, 1);

    autonomousTab_ = new AutonomousModuleTab(can, sdo);
    anviDriveTab_  = new AnviDriveTab(can, sdo);
    anviEnergyTab_ = new AnviEnergyTab(can, sdo);

    moduleTabs_->addTab(autonomousTab_, QString::fromUtf8("Автономный модуль"));
    moduleTabs_->addTab(anviDriveTab_,  QString::fromUtf8("AnviDrive"));
    moduleTabs_->addTab(anviEnergyTab_, QString::fromUtf8("AnviEnergy"));

    leftScroll->setWidget(leftWidget);

    // ── Right panel: CAN monitor ──────────────────────────────────────────────
    auto* rightWidget = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(10, 18, 18, 18);
    rightLayout->setSpacing(0);

    canMonitorPanel_ = new CanMonitorPanel(rightWidget);
    rightLayout->addWidget(canMonitorPanel_);

    // ── Splitter proportions: 60% left / 40% right ───────────────────────────
    splitter->addWidget(leftScroll);
    splitter->addWidget(rightWidget);
    splitter->setSizes({600, 400});
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    // ── Timers & connections ──────────────────────────────────────────────────
    scanTimer_ = new QTimer(this);
    scanTimer_->setSingleShot(true);
    scanTimer_->setInterval(1600);

    connect(scanButton_, &QPushButton::clicked, this, &ModuleHubTab::onScanRequested);
    connect(scanTimer_,  &QTimer::timeout,      this, &ModuleHubTab::onScanFinished);
    connect(can_, &CanDriver::frameReceived,    this, &ModuleHubTab::onCanFrame);

    connect(can_, &CanDriver::frameReceived,
            canMonitorPanel_, &CanMonitorPanel::appendFrame);
    connect(can_, &CanDriver::frameTransmitted,
            canMonitorPanel_, &CanMonitorPanel::appendFrameTx);

    setCanConnected(false);
}

void ModuleHubTab::setCanConnected(bool connected)
{
    canConnected_ = connected;
    scanButton_->setEnabled(connected);

    if (!connected) {
        scanTimer_->stop();
        scanInProgress_ = false;
        resetModuleStates();
        scanStatusLabel_->setText(
            QString::fromUtf8("Подключите CAN на вкладке \"Подключение\", чтобы выполнить сканирование."));
        return;
    }

    onScanRequested();
}

void ModuleHubTab::onScanRequested()
{
    if (!canConnected_) {
        scanStatusLabel_->setText(
            QString::fromUtf8("CAN не подключен. Сканирование сети недоступно."));
        return;
    }

    resetModuleStates();
    scanInProgress_ = true;
    updateModuleViews();
    scanStatusLabel_->setText(
        QString::fromUtf8("Сканирование сети: отправлены NMT Start узлам 0x02, 0x07 и 0x10. Ожидание heartbeat..."));

    bool allSent = sendNmtStart(AUTONOMOUS_NODE_ID);
    allSent = sendNmtStart(DRIVE_NODE_ID) && allSent;
    allSent = sendNmtStart(ENERGY_NODE_ID) && allSent;

    if (!allSent) {
        scanInProgress_ = false;
        updateModuleViews();
        scanStatusLabel_->setText(
            QString::fromUtf8("Не удалось отправить NMT-команды сканирования. Проверьте подключение CAN."));
        return;
    }

    scanTimer_->start();
}

void ModuleHubTab::onScanFinished()
{
    scanInProgress_ = false;
    const int foundCount = (autonomousSeen_ ? 1 : 0)
                         + (driveSeen_ ? 1 : 0)
                         + (energySeen_ ? 1 : 0);

    scanStatusLabel_->setText(
        foundCount > 0
            ? QString::fromUtf8("Сканирование завершено: обнаружено %1 из 3 модулей.").arg(foundCount)
            : QString::fromUtf8("Сканирование завершено: ни один модуль не ответил на NMT-команду."));
    updateModuleViews();
}

void ModuleHubTab::onCanFrame(uint32_t cobId, QByteArray data)
{
    if (cobId < 0x700 || cobId > 0x77F || data.isEmpty())
        return;

    const uint8_t nodeId   = static_cast<uint8_t>(cobId - 0x700);
    const uint8_t nmtState = static_cast<uint8_t>(data[0]);

    if (nodeId == AUTONOMOUS_NODE_ID) {
        setModuleState(nodeId, true, nmtState);
    } else if (nodeId == DRIVE_NODE_ID) {
        setModuleState(nodeId, true, nmtState);
    } else if (nodeId == ENERGY_NODE_ID) {
        setModuleState(nodeId, true, nmtState);
    } else {
        return;
    }

    if (scanInProgress_ && allKnownModulesDetected()) {
        scanTimer_->stop();
        onScanFinished();
    }
}

void ModuleHubTab::resetModuleStates()
{
    autonomousSeen_  = false;
    driveSeen_       = false;
    energySeen_      = false;
    autonomousState_ = 0xFF;
    driveState_      = 0xFF;
    energyState_     = 0xFF;
    updateModuleViews();
}

void ModuleHubTab::updateModuleViews()
{
    autonomousTab_->setModuleConnected(autonomousSeen_);
    anviDriveTab_->setModuleConnected(driveSeen_);
    anviEnergyTab_->setModuleConnected(energySeen_);

    updateSummaryRow(autonomousBadge_, autonomousInfo_,
                     AUTONOMOUS_NODE_ID, autonomousSeen_, autonomousState_);
    updateSummaryRow(driveBadge_,  driveInfo_,
                     DRIVE_NODE_ID,      driveSeen_,       driveState_);
    updateSummaryRow(energyBadge_, energyInfo_,
                     ENERGY_NODE_ID,     energySeen_,      energyState_);
}

void ModuleHubTab::updateSummaryRow(QLabel* badge, QLabel* details,
                                    uint8_t nodeId, bool connected, uint8_t nmtState)
{
    const QString nodeText = formatNodeId(nodeId);
    QString badgeText, objectName, detailsText;

    if (connected) {
        badgeText   = QString::fromUtf8("Подключен");
        objectName  = "onlineBadge";
        detailsText = QString::fromUtf8("ID %1 — heartbeat: %2.")
                          .arg(nodeText, nmtStateText(nmtState));
    } else if (!canConnected_) {
        badgeText   = QString::fromUtf8("Нет CAN");
        objectName  = "offlineBadge";
        detailsText = QString::fromUtf8("ID %1 — CAN не подключен.").arg(nodeText);
    } else if (scanInProgress_) {
        badgeText   = QString::fromUtf8("Ожидается");
        objectName  = "comingSoonBadge";
        detailsText = QString::fromUtf8("ID %1 — ожидается heartbeat после NMT-команды.").arg(nodeText);
    } else {
        badgeText   = QString::fromUtf8("Не найден");
        objectName  = "offlineBadge";
        detailsText = QString::fromUtf8("ID %1 — модуль не ответил на NMT-команду.").arg(nodeText);
    }

    badge->setText(badgeText);
    badge->setObjectName(objectName);
    badge->style()->unpolish(badge);
    badge->style()->polish(badge);
    details->setText(detailsText);
}

void ModuleHubTab::setModuleState(uint8_t nodeId, bool connected, uint8_t nmtState)
{
    switch (nodeId) {
    case AUTONOMOUS_NODE_ID: autonomousSeen_ = connected; autonomousState_ = nmtState; break;
    case DRIVE_NODE_ID:      driveSeen_      = connected; driveState_      = nmtState; break;
    case ENERGY_NODE_ID:     energySeen_     = connected; energyState_     = nmtState; break;
    default: return;
    }
    updateModuleViews();
}

bool ModuleHubTab::sendNmtStart(uint8_t nodeId)
{
    uint8_t payload[8] = {0x01, nodeId, 0, 0, 0, 0, 0, 0};
    return can_ && can_->transmit(0x000, payload);
}

bool ModuleHubTab::allKnownModulesDetected() const
{
    return autonomousSeen_ && driveSeen_ && energySeen_;
}

QString ModuleHubTab::nmtStateText(uint8_t nmtState) const
{
    switch (nmtState) {
    case 0x00: return QString::fromUtf8("Boot-up");
    case 0x04: return QString::fromUtf8("Stopped");
    case 0x05: return QString::fromUtf8("Operational");
    case 0x7F: return QString::fromUtf8("Pre-operational");
    default:   return QString("0x%1").arg(nmtState, 2, 16, QChar('0')).toUpper();
    }
}

QString ModuleHubTab::formatNodeId(uint8_t nodeId) const
{
    return QString("0x%1").arg(nodeId, 2, 16, QChar('0')).toUpper();
}
