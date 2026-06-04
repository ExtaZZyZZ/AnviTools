#include "FirmwareTab.h"
#include "../can/CanDriver.h"
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

FirmwareTab::FirmwareTab(CanDriver* can, uint8_t nodeId, QWidget* parent)
    : QWidget(parent), can_(can), nodeId_(nodeId)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // ── File selection ────────────────────────────────────────────────────
    auto* fileGroup = new QGroupBox(QString::fromUtf8("Файл прошивки (.bin)"), this);
    auto* fileRow   = new QHBoxLayout(fileGroup);
    fileRow->setSpacing(8);

    pathEdit_ = new QLineEdit(fileGroup);
    pathEdit_->setReadOnly(true);
    pathEdit_->setPlaceholderText(QString::fromUtf8("Выберите .bin файл..."));

    browseBtn_ = new QPushButton(QString::fromUtf8("Обзор"), fileGroup);
    browseBtn_->setFixedWidth(80);

    fileRow->addWidget(pathEdit_);
    fileRow->addWidget(browseBtn_);
    root->addWidget(fileGroup);

    // ── Flash control ─────────────────────────────────────────────────────
    auto* flashGroup  = new QGroupBox(QString::fromUtf8("Прошивка"), this);
    auto* flashLayout = new QVBoxLayout(flashGroup);
    flashLayout->setSpacing(8);

    progressBar_ = new QProgressBar(flashGroup);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(true);
    progressBar_->setFormat("%p%  (%v / %m чанков)");

    statusLabel_ = new QLabel(QString::fromUtf8("Выберите .bin файл для начала"), flashGroup);
    statusLabel_->setObjectName("hintText");
    statusLabel_->setWordWrap(true);

    flashBtn_ = new QPushButton(QString::fromUtf8("Прошить"), flashGroup);
    flashBtn_->setEnabled(false);

    auto* infoRow  = new QHBoxLayout();
    auto* nodeLabel = new QLabel(
        QString::fromUtf8("Целевой узел: 0x%1").arg(nodeId_, 2, 16, QChar('0')).toUpper(),
        flashGroup);
    nodeLabel->setObjectName("hintText");
    infoRow->addWidget(nodeLabel);
    infoRow->addStretch();
    infoRow->addWidget(flashBtn_);

    flashLayout->addWidget(progressBar_);
    flashLayout->addWidget(statusLabel_);
    flashLayout->addLayout(infoRow);
    root->addWidget(flashGroup);

    // ── Log ──────────────────────────────────────────────────────────────
    auto* logGroup  = new QGroupBox(QString::fromUtf8("Журнал"), this);
    auto* logLayout = new QVBoxLayout(logGroup);

    logEdit_ = new QTextEdit(logGroup);
    logEdit_->setReadOnly(true);
    logEdit_->setFont(QFont("Courier New", 9));
    logEdit_->setMaximumHeight(160);
    logLayout->addWidget(logEdit_);
    root->addWidget(logGroup);

    root->addStretch();

    // ── Timer ─────────────────────────────────────────────────────────────
    timer_ = new QTimer(this);
    timer_->setSingleShot(true);

    connect(browseBtn_, &QPushButton::clicked,    this, &FirmwareTab::onBrowse);
    connect(flashBtn_,  &QPushButton::clicked,    this, &FirmwareTab::onFlash);
    connect(timer_,     &QTimer::timeout,         this, &FirmwareTab::onTimeout);
    connect(can_,       &CanDriver::frameReceived, this, &FirmwareTab::onCanFrame);
}

// ── Slots ────────────────────────────────────────────────────────────────

void FirmwareTab::onBrowse()
{
    QString path = QFileDialog::getOpenFileName(
        this, QString::fromUtf8("Выбрать файл прошивки"), "", "Binary files (*.bin)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        log(QString::fromUtf8("[ERR] Не удалось открыть файл: ") + path);
        return;
    }
    firmware_ = f.readAll();
    f.close();

    totalChunks_ = (firmware_.size() + CHUNK_SIZE - 1) / CHUNK_SIZE;
    progressBar_->setMaximum(totalChunks_);
    progressBar_->setValue(0);
    progressBar_->setFormat("%v / %m чанков  (%p%)");

    pathEdit_->setText(path);
    statusLabel_->setText(
        QString::fromUtf8("Файл загружен: %1 байт, %2 чанков")
            .arg(firmware_.size()).arg(totalChunks_));
    flashBtn_->setEnabled(true);

    log(QString::fromUtf8("[INFO] Загружен: %1 (%2 байт, CRC32=0x%3)")
            .arg(path.section('/', -1))
            .arg(firmware_.size())
            .arg(calcCrc32(firmware_), 8, 16, QChar('0')).toUpper());
}

void FirmwareTab::onFlash()
{
    if (firmware_.isEmpty() || state_ != State::Idle) return;

    flashBtn_->setEnabled(false);
    browseBtn_->setEnabled(false);
    chunkIndex_ = 0;
    retries_    = 0;
    state_      = State::WaitReady;

    progressBar_->setValue(0);
    statusLabel_->setText(QString::fromUtf8("Ожидание бутлоадера..."));

    log(QString::fromUtf8("[INFO] Запрос перехода в бутлоадер — узел 0x%1")
            .arg(nodeId_, 2, 16, QChar('0')).toUpper());
    sendEnter();
    timer_->start(TIMEOUT_BOOT_MS);
}

void FirmwareTab::onCanFrame(uint32_t cobId, QByteArray data)
{
    // Only handle bootloader responses (fixed COB-ID 0x7E3)
    if (cobId != COB_RX_BASE) return;
    if (data.isEmpty()) return;

    uint8_t rsp = static_cast<uint8_t>(data[0]);

    switch (state_) {

    case State::WaitReady:
        if (rsp == RSP_READY) {
            timer_->stop();
            log(QString::fromUtf8("[OK] Бутлоадер готов. Начинаем передачу %1 чанков...")
                    .arg(totalChunks_));
            state_ = State::Flashing;
            sendStart();
            sendNextChunk();
        }
        break;

    case State::Flashing:
        if (rsp == RSP_ACK) {
            timer_->stop();
            retries_ = 0;
            chunkIndex_++;
            progressBar_->setValue(chunkIndex_);
            statusLabel_->setText(
                QString::fromUtf8("Передача: %1 / %2 чанков")
                    .arg(chunkIndex_).arg(totalChunks_));

            if (chunkIndex_ >= totalChunks_) {
                log(QString::fromUtf8("[INFO] Все чанки переданы. Отправка CRC32..."));
                state_ = State::WaitDone;
                sendEnd();
            } else {
                sendNextChunk();
            }
        } else if (rsp == RSP_NACK) {
            timer_->stop();
            if (++retries_ <= MAX_RETRIES) {
                log(QString::fromUtf8("[WARN] NACK на чанк %1, повтор %2/%3")
                        .arg(chunkIndex_).arg(retries_).arg(MAX_RETRIES));
                sendNextChunk();
            } else {
                abort(QString::fromUtf8("Превышено количество повторов на чанке %1")
                          .arg(chunkIndex_));
            }
        }
        break;

    case State::WaitDone:
        if (rsp == RSP_DONE) {
            timer_->stop();
            finishSuccess();
        } else if (rsp == RSP_ERROR) {
            abort(QString::fromUtf8("Ошибка CRC32 на стороне устройства"));
        }
        break;

    default:
        break;
    }
}

void FirmwareTab::onTimeout()
{
    switch (state_) {
    case State::WaitReady:
        abort(QString::fromUtf8("Таймаут: бутлоадер не ответил (%1 мс)").arg(TIMEOUT_BOOT_MS));
        break;
    case State::Flashing:
        if (++retries_ <= MAX_RETRIES) {
            log(QString::fromUtf8("[WARN] Таймаут ACK на чанке %1, повтор %2/%3")
                    .arg(chunkIndex_).arg(retries_).arg(MAX_RETRIES));
            sendNextChunk();
        } else {
            abort(QString::fromUtf8("Таймаут: нет ACK на чанке %1").arg(chunkIndex_));
        }
        break;
    case State::WaitDone:
        abort(QString::fromUtf8("Таймаут: нет ответа на CRC (%1 мс)").arg(TIMEOUT_DONE_MS));
        break;
    default:
        break;
    }
}

// ── Protocol helpers ──────────────────────────────────────────────────────

void FirmwareTab::sendEnter()
{
    // Broadcast CMD_ENTER with target node ID
    uint8_t d[8] = { CMD_ENTER, nodeId_, 0, 0, 0, 0, 0, 0 };
    can_->transmit(COB_TX_ENTER, d);
}

void FirmwareTab::sendStart()
{
    uint32_t addr = APP_ADDR;
    uint32_t size = static_cast<uint32_t>(firmware_.size());
    uint8_t d[8] = {
        CMD_START,
        uint8_t(addr >> 24), uint8_t(addr >> 16),
        uint8_t(addr >>  8), uint8_t(addr),
        uint8_t(size >> 16), uint8_t(size >> 8), uint8_t(size)
    };
    can_->transmit(COB_TX_BASE, d);
}

void FirmwareTab::sendNextChunk()
{
    int offset = chunkIndex_ * CHUNK_SIZE;
    uint8_t d[8] = { CMD_DATA, uint8_t(chunkIndex_ & 0xFF), 0, 0, 0, 0, 0, 0 };
    int n = qMin(CHUNK_SIZE, firmware_.size() - offset);
    for (int i = 0; i < n; i++)
        d[2 + i] = static_cast<uint8_t>(firmware_[offset + i]);
    can_->transmit(COB_TX_BASE, d);
    timer_->start(TIMEOUT_ACK_MS);
}

void FirmwareTab::sendEnd()
{
    uint32_t crc = calcCrc32(firmware_);
    uint8_t d[8] = {
        CMD_END,
        uint8_t(crc >> 24), uint8_t(crc >> 16),
        uint8_t(crc >>  8), uint8_t(crc),
        0, 0, 0
    };
    can_->transmit(COB_TX_BASE, d);
    timer_->start(TIMEOUT_DONE_MS);
}

void FirmwareTab::sendJump()
{
    uint8_t d[8] = { CMD_JUMP, 0, 0, 0, 0, 0, 0, 0 };
    can_->transmit(COB_TX_BASE, d);
}

// ── State helpers ─────────────────────────────────────────────────────────

void FirmwareTab::finishSuccess()
{
    state_ = State::Idle;
    progressBar_->setValue(totalChunks_);
    statusLabel_->setText(QString::fromUtf8("Готово — прошивка записана успешно"));
    log(QString::fromUtf8("[OK] Прошивка записана. Отправка команды запуска приложения..."));
    sendJump();
    log(QString::fromUtf8("[OK] Модуль перезапускается."));
    flashBtn_->setEnabled(true);
    browseBtn_->setEnabled(true);
}

void FirmwareTab::abort(const QString& reason)
{
    timer_->stop();
    state_ = State::Idle;
    log(QString::fromUtf8("[ERR] Прервано: ") + reason);
    statusLabel_->setText(QString::fromUtf8("Ошибка: ") + reason);
    progressBar_->setValue(0);
    flashBtn_->setEnabled(true);
    browseBtn_->setEnabled(true);
}

void FirmwareTab::log(const QString& msg)
{
    const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    logEdit_->append(QString("[%1] %2").arg(ts, msg));
}

// ── CRC32 (IEEE 802.3 polynomial) ─────────────────────────────────────────

uint32_t FirmwareTab::calcCrc32(const QByteArray& data)
{
    uint32_t crc = 0xFFFFFFFF;
    for (int i = 0; i < data.size(); i++) {
        crc ^= static_cast<uint8_t>(data[i]);
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ (0xEDB88320u & (-(crc & 1u)));
    }
    return ~crc;
}
