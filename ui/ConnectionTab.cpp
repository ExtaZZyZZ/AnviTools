#include "ConnectionTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QSettings>
#include <QLabel>
#include <QFrame>
#include <QFont>

namespace {

QLabel* makeSectionHint(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName("hintText");
    label->setWordWrap(true);
    return label;
}

void configureFormLayout(QFormLayout* form)
{
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

} // namespace

ConnectionTab::ConnectionTab(CanDriver* can, SerialMonitor* serial, QWidget* parent)
    : QWidget(parent), canDriver_(can), serialMon_(serial)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(14);
    mainLayout->setAlignment(Qt::AlignTop);

    // ---- CAN Group ----
    auto* canGroup = new QGroupBox(QString::fromUtf8("CAN интерфейс"), this);
    auto* canGroupLayout = new QVBoxLayout(canGroup);
    canGroupLayout->setSpacing(10);
    canGroupLayout->addWidget(
        makeSectionHint(QString::fromUtf8("Waveshare USB-CAN-A (CH341 virtual COM)."), canGroup));

    auto* canForm = new QFormLayout();
    configureFormLayout(canForm);

    auto* canPortRow = new QHBoxLayout();
    canPortBox_ = new QComboBox(this);
    canRefreshBtn_ = new QPushButton("Refresh", this);
    canPortRow->addWidget(canPortBox_, 1);
    canPortRow->addWidget(canRefreshBtn_);
    canForm->addRow(QString::fromUtf8("COM порт:"), canPortRow);

    canBaudBox_ = new QComboBox(this);
    canBaudBox_->addItem(QString::fromUtf8("2000000 baud (по умолчанию)"), 2000000);
    canBaudBox_->addItem("1228800 baud", 1228800);
    canBaudBox_->addItem("115200 baud", 115200);
    canForm->addRow(QString::fromUtf8("Скорость (baud):"), canBaudBox_);

    auto* canBtnRow = new QHBoxLayout();
    canBtnRow->setSpacing(10);
    canConnectBtn_ = new QPushButton(QString::fromUtf8("Подключить CAN"), this);
    canStatusLabel_ = new QLabel(QString::fromUtf8("● Отключено"), this);
    canStatusLabel_->setStyleSheet("background: rgba(220,80,60,0.18); color: #8B2010; padding: 4px 10px; border-radius: 8px;");
    canBtnRow->addWidget(canConnectBtn_);
    canBtnRow->addWidget(canStatusLabel_);
    canBtnRow->addStretch();
    canForm->addRow(QString(), canBtnRow);

    canGroupLayout->addLayout(canForm);
    mainLayout->addWidget(canGroup);

    // ---- Serial Group ----
    auto* serialGroup = new QGroupBox(QString::fromUtf8("Монитор UART отладки"), this);
    auto* serialGroupLayout = new QVBoxLayout(serialGroup);
    serialGroupLayout->setSpacing(10);
    serialGroupLayout->addWidget(
        makeSectionHint(QString::fromUtf8("UART2 @ 115200 (USB-UART адаптер)."), serialGroup));

    auto* serialForm = new QFormLayout();
    configureFormLayout(serialForm);

    auto* serPortRow = new QHBoxLayout();
    serialPortBox_ = new QComboBox(this);
    serialRefreshBtn_ = new QPushButton("Refresh", this);
    serPortRow->addWidget(serialPortBox_, 1);
    serPortRow->addWidget(serialRefreshBtn_);
    serialForm->addRow(QString::fromUtf8("COM порт:"), serPortRow);

    auto* serBtnRow = new QHBoxLayout();
    serBtnRow->setSpacing(10);
    serialConnectBtn_ = new QPushButton(QString::fromUtf8("Подключить Serial"), this);
    serialStatusLabel_ = new QLabel(QString::fromUtf8("● Отключено"), this);
    serialStatusLabel_->setStyleSheet("background: rgba(220,80,60,0.18); color: #8B2010; padding: 4px 10px; border-radius: 8px;");
    serBtnRow->addWidget(serialConnectBtn_);
    serBtnRow->addWidget(serialStatusLabel_);
    serBtnRow->addStretch();
    serialForm->addRow(QString(), serBtnRow);

    serialGroupLayout->addLayout(serialForm);
    mainLayout->addWidget(serialGroup);

    // ---- Raw bytes diagnostic panel ----
    auto* diagGroup = new QGroupBox(QString::fromUtf8("Диагностика CAN адаптера"), this);
    diagGroup->setToolTip(
        QString::fromUtf8("Показывает сырые байты от адаптера Waveshare USB-CAN-A.\n\n"
                          "Корректные фреймы Waveshare начинаются с 0xAA и заканчиваются 0x55.\n"
                          "Если байты не начинаются с 0xAA — неверная скорость (baud rate).\n"
                          "Попробуйте 115200 baud вместо 2000000.\n"
                          "Если ничего нет — QSerialPort не получает данные (неверный порт?)."));
    auto* diagLayout = new QVBoxLayout(diagGroup);
    diagLayout->setSpacing(10);
    diagLayout->addWidget(makeSectionHint(
        QString::fromUtf8("Первые 128 байт от CAN-адаптера. Подсказка по скорости связи."),
        diagGroup));

    rawHexView_ = new QTextEdit(this);
    rawHexView_->setReadOnly(true);
    rawHexView_->setFont(QFont("Courier New", 9));
    rawHexView_->setFixedHeight(80);
    rawHexView_->setStyleSheet("background-color: #1e1e1e; color: #9cdcfe;");
    rawHexView_->setPlaceholderText(
        QString::fromUtf8("Байты появятся здесь, когда CAN-адаптер начнёт передавать данные..."));
    diagLayout->addWidget(rawHexView_);
    mainLayout->addWidget(diagGroup);

    mainLayout->addStretch();

    connect(canRefreshBtn_,    &QPushButton::clicked, this, &ConnectionTab::onCanRefresh);
    connect(canConnectBtn_,    &QPushButton::clicked, this, &ConnectionTab::onCanConnect);
    connect(serialRefreshBtn_, &QPushButton::clicked, this, &ConnectionTab::onSerialRefresh);
    connect(serialConnectBtn_, &QPushButton::clicked, this, &ConnectionTab::onSerialConnect);
    connect(canDriver_, &CanDriver::rawBytesReceived, this, &ConnectionTab::onRawBytes);

    loadSettings();
    onCanRefresh();
    onSerialRefresh();
}

void ConnectionTab::onCanRefresh()
{
    canPortBox_->clear();
    for (const QString& p : CanDriver::availablePorts())
        canPortBox_->addItem(p);
}

void ConnectionTab::onCanConnect()
{
    if (canOpen_) {
        canDriver_->close();
        setCanConnected(false);
        emit canDisconnected();
        return;
    }
    if (canPortBox_->currentText().isEmpty()) {
        QMessageBox::warning(this,
                             QString::fromUtf8("Нет порта"),
                             QString::fromUtf8("Выберите COM порт для CAN адаптера."));
        return;
    }
    saveSettings();
    int baud = canBaudBox_->currentData().toInt();
    QString err;
    if (!canDriver_->open(canPortBox_->currentText(), baud, &err)) {
        QMessageBox::critical(this, QString::fromUtf8("Ошибка CAN"), err);
        return;
    }
    rawHexView_->clear();
    rawByteCount_ = 0;

    setCanConnected(true);
    emit canConnected();
}

void ConnectionTab::onSerialRefresh()
{
    serialPortBox_->clear();
    for (const QString& p : SerialMonitor::availablePorts())
        serialPortBox_->addItem(p);
}

void ConnectionTab::onSerialConnect()
{
    if (serialOpen_) {
        serialMon_->close();
        setSerialConnected(false);
        emit serialDisconnected();
        return;
    }
    if (serialPortBox_->currentText().isEmpty()) {
        QMessageBox::warning(this,
                             QString::fromUtf8("Нет порта"),
                             QString::fromUtf8("Выберите COM порт для UART монитора."));
        return;
    }
    bool ok = false;
    QMetaObject::invokeMethod(serialMon_, [&]() {
        ok = serialMon_->open(serialPortBox_->currentText());
    }, Qt::BlockingQueuedConnection);

    if (!ok) {
        QMessageBox::critical(this,
                              QString::fromUtf8("Ошибка Serial"),
                              QString::fromUtf8("Невозможно открыть ")
                                  + serialPortBox_->currentText());
        return;
    }
    setSerialConnected(true);
    emit serialConnected();
}

void ConnectionTab::onRawBytes(QByteArray data)
{
    if (rawByteCount_ >= MAX_RAW_BYTES) return;

    bool hasAA = false;
    QString hex;
    for (int i = 0; i < data.size() && rawByteCount_ < MAX_RAW_BYTES; ++i, ++rawByteCount_) {
        uint8_t b = static_cast<uint8_t>(data[i]);
        hex += QString("%1 ").arg(b, 2, 16, QChar('0')).toUpper();
        if (b == 0xAA) hasAA = true;
    }
    rawHexView_->insertPlainText(hex);

    if (rawByteCount_ >= MAX_RAW_BYTES) {
        if (!hasAA) {
            rawHexView_->insertPlainText(
                QString::fromUtf8("\n[WARN] Байтов 0xAA не найдено — несоответствие скорости.\n"
                                  "Отключитесь и попробуйте 115200 baud вместо 2000000."));
        } else {
            rawHexView_->insertPlainText(
                QString::fromUtf8("\n[OK] Найден 0xAA — скорость выбрана корректно."));
        }
    }
}

void ConnectionTab::setCanConnected(bool connected)
{
    canOpen_ = connected;
    canConnectBtn_->setText(connected
                                ? QString::fromUtf8("Отключить CAN")
                                : QString::fromUtf8("Подключить CAN"));
    canPortBox_->setEnabled(!connected);
    canRefreshBtn_->setEnabled(!connected);
    canBaudBox_->setEnabled(!connected);
    canStatusLabel_->setText(connected
                                 ? QString::fromUtf8("● Подключено")
                                 : QString::fromUtf8("● Отключено"));
    canStatusLabel_->setStyleSheet(connected
        ? "background: rgba(80,200,140,0.25); color: #1A6640; padding: 4px 10px; border-radius: 8px; font-weight: bold;"
        : "background: rgba(220,80,60,0.18); color: #8B2010; padding: 4px 10px; border-radius: 8px;");
}

void ConnectionTab::setSerialConnected(bool connected)
{
    serialOpen_ = connected;
    serialConnectBtn_->setText(connected
                                   ? QString::fromUtf8("Отключить Serial")
                                   : QString::fromUtf8("Подключить Serial"));
    serialPortBox_->setEnabled(!connected);
    serialRefreshBtn_->setEnabled(!connected);
    serialStatusLabel_->setText(connected
                                    ? QString::fromUtf8("● Подключено")
                                    : QString::fromUtf8("● Отключено"));
    serialStatusLabel_->setStyleSheet(connected
        ? "background: rgba(80,200,140,0.25); color: #1A6640; padding: 4px 10px; border-radius: 8px; font-weight: bold;"
        : "background: rgba(220,80,60,0.18); color: #8B2010; padding: 4px 10px; border-radius: 8px;");
}

void ConnectionTab::loadSettings()
{
    QSettings s("AnviTools", "AnviTools");
    QString canPort = s.value("can/port").toString();
    int baudIdx = s.value("can/baudIdx", -1).toInt();

    if (canPort.isEmpty() && baudIdx < 0) {
        QSettings legacy("ModemDebugger", "ModemDebugger");
        canPort = legacy.value("can/port").toString();
        baudIdx = legacy.value("can/baudIdx", 0).toInt();
    }

    if (!canPort.isEmpty()) {
        int idx = canPortBox_->findText(canPort);
        if (idx >= 0) canPortBox_->setCurrentIndex(idx);
    }
    if (baudIdx < 0) baudIdx = 0;
    if (baudIdx < canBaudBox_->count()) canBaudBox_->setCurrentIndex(baudIdx);
}

void ConnectionTab::saveSettings()
{
    QSettings s("AnviTools", "AnviTools");
    s.setValue("can/port", canPortBox_->currentText());
    s.setValue("can/baudIdx", canBaudBox_->currentIndex());
}
