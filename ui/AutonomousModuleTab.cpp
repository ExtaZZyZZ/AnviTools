#include "AutonomousModuleTab.h"
#include "ControlTab.h"
#include "ConfigTab.h"
#include "FirmwareTab.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include "../can/CanDriver.h"
#include "../canopen/SdoClient.h"

AutonomousModuleTab::AutonomousModuleTab(CanDriver* can, SdoClient* sdo, QWidget* parent)
    : QWidget(parent)
{
    setObjectName("modulePage");

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 12, 0, 12);
    rootLayout->setSpacing(16);

    auto* heroCard = new QFrame(this);
    heroCard->setObjectName("heroCard");
    auto* heroLayout = new QVBoxLayout(heroCard);
    heroLayout->setContentsMargins(24, 22, 24, 22);
    heroLayout->setSpacing(12);

    auto* titleLabel = new QLabel(QString::fromUtf8("Автономный модуль"), heroCard);
    titleLabel->setObjectName("pageTitle");
    heroLayout->addWidget(titleLabel);

    auto* stateRow = new QHBoxLayout();
    stateRow->setSpacing(10);

    canStateBadge_ = new QLabel(QString::fromUtf8("Модуль не подключен"), heroCard);
    canStateBadge_->setObjectName("offlineBadge");
    stateRow->addWidget(canStateBadge_, 0, Qt::AlignLeft);

    canStateHint_ = new QLabel(
        QString::fromUtf8("Модуль 0x02 не ответил на NMT-команду сканирования."),
        heroCard);
    canStateHint_->setObjectName("hintText");
    canStateHint_->setWordWrap(true);
    stateRow->addWidget(canStateHint_, 1);

    heroLayout->addLayout(stateRow);
    rootLayout->addWidget(heroCard);

    toolTabs_ = new QTabWidget(this);
    toolTabs_->setObjectName("moduleToolTabs");
    toolTabs_->setDocumentMode(true);
    toolTabs_->setUsesScrollButtons(true);
    toolTabs_->tabBar()->setExpanding(false);
    toolTabs_->tabBar()->setElideMode(Qt::ElideRight);
    rootLayout->addWidget(toolTabs_, 1);

    controlTab_  = new ControlTab(can, sdo, this);
    configTab_   = new ConfigTab(sdo, this);
    firmwareTab_ = new FirmwareTab(can, 0x02, this);

    toolTabs_->addTab(controlTab_,  QString::fromUtf8("Управление"));
    toolTabs_->addTab(configTab_,   QString::fromUtf8("Конфигурация"));
    toolTabs_->addTab(firmwareTab_, QString::fromUtf8("Прошивка"));

    setModuleConnected(false);
}

void AutonomousModuleTab::setModuleConnected(bool connected)
{
    canStateBadge_->setText(connected
                                ? QString::fromUtf8("Модуль подключен")
                                : QString::fromUtf8("Модуль не подключен"));
    canStateBadge_->setObjectName(connected ? "onlineBadge" : "offlineBadge");
    canStateBadge_->style()->unpolish(canStateBadge_);
    canStateBadge_->style()->polish(canStateBadge_);

    canStateHint_->setText(
        connected
            ? QString::fromUtf8("Модуль 0x02 найден в сети. Доступны управление, настройка, журнал Flash и SDO-операции.")
            : QString::fromUtf8("Модуль 0x02 не ответил на NMT-команду сканирования."));
}
