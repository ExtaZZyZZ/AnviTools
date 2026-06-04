#include "SensorTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>

SensorTab::SensorTab(SdoClient* sdo, QWidget* parent)
    : QWidget(parent), sdo_(sdo)
{
    pollTimer_ = new QTimer(this);
    connect(pollTimer_, &QTimer::timeout, this, &SensorTab::onPollTimer);

    auto* layout = new QVBoxLayout(this);

    // Poll controls
    auto* ctrlRow = new QHBoxLayout();
    pollToggleBtn_ = new QPushButton("Начать опрос", this);
    intervalSpin_  = new QSpinBox(this);
    intervalSpin_->setRange(1, 60);
    intervalSpin_->setValue(2);
    intervalSpin_->setSuffix(" s");
    ctrlRow->addWidget(new QLabel("Интервал:", this));
    ctrlRow->addWidget(intervalSpin_);
    ctrlRow->addWidget(pollToggleBtn_);
    ctrlRow->addStretch();
    layout->addLayout(ctrlRow);

    // Sensor 1
    auto* s1Group = new QGroupBox("Датчик 1 — Давление (Узел 0x0A, OD 0x6001)", this);
    auto* s1Layout = new QVBoxLayout(s1Group);
    s1ValueLabel_  = new QLabel("— kPa", this);
    s1ValueLabel_->setFont(QFont("Arial", 28, QFont::Bold));
    s1Bar_ = new QProgressBar(this);
    s1Bar_->setRange(0, MAX_KPA);
    s1Bar_->setValue(0);
    s1Bar_->setTextVisible(true);
    s1Bar_->setFormat("%v kPa");
    s1StatusLabel_ = new QLabel("Ожидание", this);
    s1StatusLabel_->setStyleSheet("color: gray;");
    s1Layout->addWidget(s1ValueLabel_);
    s1Layout->addWidget(s1Bar_);
    s1Layout->addWidget(s1StatusLabel_);
    layout->addWidget(s1Group);

    // Sensor 2
    auto* s2Group = new QGroupBox("Датчик 2 — Давление (Узел 0x0A, OD 0x6002)", this);
    auto* s2Layout = new QVBoxLayout(s2Group);
    s2ValueLabel_  = new QLabel("— kPa", this);
    s2ValueLabel_->setFont(QFont("Arial", 28, QFont::Bold));
    s2Bar_ = new QProgressBar(this);
    s2Bar_->setRange(0, MAX_KPA);
    s2Bar_->setValue(0);
    s2Bar_->setTextVisible(true);
    s2Bar_->setFormat("%v kPa");
    s2StatusLabel_ = new QLabel("Ожидание", this);
    s2StatusLabel_->setStyleSheet("color: gray;");
    s2Layout->addWidget(s2ValueLabel_);
    s2Layout->addWidget(s2Bar_);
    s2Layout->addWidget(s2StatusLabel_);
    layout->addWidget(s2Group);

    layout->addStretch();

    connect(pollToggleBtn_, &QPushButton::clicked, this, &SensorTab::onPollToggle);
}

void SensorTab::onPollToggle()
{
    if (pollTimer_->isActive()) {
        pollTimer_->stop();
        pollToggleBtn_->setText("Начать опрос");
    } else {
        pollTimer_->start(intervalSpin_->value() * 1000);
        pollToggleBtn_->setText("Остановить опрос");
        onPollTimer(); // immediate first read
    }
}

void SensorTab::onPollTimer()
{
    setControlsEnabled(false);
    updateSensor(S1_NODE, S1_INDEX, s1ValueLabel_, s1Bar_, s1StatusLabel_);
    updateSensor(S2_NODE, S2_INDEX, s2ValueLabel_, s2Bar_, s2StatusLabel_);
    setControlsEnabled(true);
}

void SensorTab::updateSensor(uint8_t nodeId, uint16_t odIndex,
                               QLabel* valueLabel, QProgressBar* bar, QLabel* statusLabel)
{
    uint16_t pressure = 0;
    QString err;
    if (sdo_->readUInt16(nodeId, odIndex, 0, pressure, err)) {
        valueLabel->setText(QString("%1 kPa").arg(pressure));
        bar->setValue(pressure);
        statusLabel->setText("ОК");
        statusLabel->setStyleSheet("color: green;");
    } else {
        statusLabel->setText("Ошибка: " + err);
        statusLabel->setStyleSheet("color: red;");
    }
}

void SensorTab::setControlsEnabled(bool en)
{
    pollToggleBtn_->setEnabled(en);
    intervalSpin_->setEnabled(en);
}
