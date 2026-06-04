#include "ModulePlaceholderTab.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QVBoxLayout>

ModulePlaceholderTab::ModulePlaceholderTab(const QString& title,
                                           const QString& description,
                                           QWidget* parent)
    : QWidget(parent)
    , description_(description)
{
    setObjectName("modulePage");

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(28, 28, 28, 28);
    rootLayout->setSpacing(0);

    auto* card = new QFrame(this);
    card->setObjectName("placeholderCard");
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(28, 28, 28, 28);
    cardLayout->setSpacing(16);

    auto* badgeRow = new QHBoxLayout();
    statusBadge_ = new QLabel(QString::fromUtf8("Модуль не подключен"), card);
    statusBadge_->setObjectName("offlineBadge");
    badgeRow->addWidget(statusBadge_, 0, Qt::AlignLeft);

    auto* comingSoonBadge = new QLabel(QString::fromUtf8("Скоро"), card);
    comingSoonBadge->setObjectName("comingSoonBadge");
    badgeRow->addWidget(comingSoonBadge, 0, Qt::AlignLeft);
    badgeRow->addStretch();
    cardLayout->addLayout(badgeRow);

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setObjectName("pageTitle");
    cardLayout->addWidget(titleLabel);

    auto* descLabel = new QLabel(description_, card);
    descLabel->setObjectName("pageSubtitle");
    descLabel->setWordWrap(true);
    cardLayout->addWidget(descLabel);

    statusHint_ = new QLabel(QString::fromUtf8("Модуль не ответил на NMT-команду сканирования."), card);
    statusHint_->setObjectName("hintText");
    statusHint_->setWordWrap(true);
    cardLayout->addWidget(statusHint_);

    noteLabel_ = new QLabel(
        QString::fromUtf8("Экран модуля подготовлен как каркас. После появления устройства в сети здесь можно будет развивать отдельный набор экранов и команд."),
        card);
    noteLabel_->setObjectName("placeholderNote");
    noteLabel_->setWordWrap(true);
    cardLayout->addWidget(noteLabel_);

    cardLayout->addStretch();
    rootLayout->addWidget(card, 1);
}

void ModulePlaceholderTab::setModuleConnected(bool connected, uint8_t nodeId)
{
    const QString nodeText = QString("0x%1").arg(nodeId, 2, 16, QChar('0')).toUpper();

    statusBadge_->setText(connected
                              ? QString::fromUtf8("Модуль подключен")
                              : QString::fromUtf8("Модуль не подключен"));
    statusBadge_->setObjectName(connected ? "onlineBadge" : "offlineBadge");
    statusBadge_->style()->unpolish(statusBadge_);
    statusBadge_->style()->polish(statusBadge_);

    statusHint_->setText(
        connected
            ? QString::fromUtf8("Узел %1 найден в CAN-сети. Основной экран модуля пока в разработке.")
                  .arg(nodeText)
            : QString::fromUtf8("Узел %1 не ответил на NMT-команду сканирования.")
                  .arg(nodeText));

    noteLabel_->setText(
        connected
            ? QString::fromUtf8("Модуль присутствует в сети. Сейчас доступен только информационный каркас страницы.")
            : QString::fromUtf8("После подключения модуля страница останется той же, но статус сменится на подключенный."));
}
