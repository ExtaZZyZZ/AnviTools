#pragma once
#include <QWidget>
#include <QByteArray>
#include <cstdint>

class QLabel;
class QLineEdit;
class QPushButton;
class QProgressBar;
class QTextEdit;
class QTimer;
class CanDriver;

class FirmwareTab : public QWidget {
    Q_OBJECT
public:
    explicit FirmwareTab(CanDriver* can, uint8_t nodeId, QWidget* parent = nullptr);

private slots:
    void onBrowse();
    void onFlash();
    void onCanFrame(uint32_t cobId, QByteArray data);
    void onTimeout();

private:
    enum class State { Idle, WaitReady, Flashing, WaitDone };

    void log(const QString& msg);
    void sendEnter();
    void sendStart();
    void sendNextChunk();
    void sendEnd();
    void sendJump();
    void abort(const QString& reason);
    void finishSuccess();
    static uint32_t calcCrc32(const QByteArray& data);

    // Application start address — bootloader sits below, app above
    static constexpr uint32_t APP_ADDR        = 0x08008000;
    // COB-IDs matching bootloader protocol in boot_config.h
    static constexpr uint32_t COB_TX_ENTER    = 0x7E0;  // CMD_ENTER broadcast
    static constexpr uint32_t COB_TX_BASE     = 0x7E2;  // CMD_START/DATA/END/JUMP (fixed)
    static constexpr uint32_t COB_RX_BASE     = 0x7E3;  // RSP_READY/ACK/DONE/ERROR (fixed)
    // Bytes of firmware data per CAN frame (8 total: 1 cmd + 1 seq + 6 data)
    static constexpr int      CHUNK_SIZE      = 6;
    static constexpr int      TIMEOUT_BOOT_MS = 3000;   // waiting for READY after CMD_ENTER
    static constexpr int      TIMEOUT_ACK_MS  = 500;    // waiting for ACK per chunk
    static constexpr int      TIMEOUT_DONE_MS = 3000;   // waiting for DONE after CMD_END
    static constexpr int      MAX_RETRIES     = 3;

    // Bootloader protocol command bytes (TX)
    static constexpr uint8_t CMD_ENTER = 0xB0;  // request node to enter bootloader
    static constexpr uint8_t CMD_START = 0x01;  // start transfer: addr + size
    static constexpr uint8_t CMD_DATA  = 0x02;  // data chunk
    static constexpr uint8_t CMD_END   = 0x03;  // end of transfer + CRC32
    static constexpr uint8_t CMD_JUMP  = 0x04;  // jump to application

    // Bootloader protocol response bytes (RX)
    static constexpr uint8_t RSP_READY = 0xAA;  // bootloader is ready
    static constexpr uint8_t RSP_ACK   = 0xAC;  // chunk accepted
    static constexpr uint8_t RSP_NACK  = 0xBB;  // chunk rejected — retransmit
    static constexpr uint8_t RSP_DONE  = 0xDD;  // CRC OK, write complete
    static constexpr uint8_t RSP_ERROR = 0xEE;  // CRC mismatch or flash error

    CanDriver*    can_;
    uint8_t       nodeId_;
    State         state_       = State::Idle;
    QByteArray    firmware_;
    int           chunkIndex_  = 0;
    int           totalChunks_ = 0;
    int           retries_     = 0;

    QLineEdit*    pathEdit_    = nullptr;
    QPushButton*  browseBtn_   = nullptr;
    QPushButton*  flashBtn_    = nullptr;
    QProgressBar* progressBar_ = nullptr;
    QLabel*       statusLabel_ = nullptr;
    QTextEdit*    logEdit_     = nullptr;
    QTimer*       timer_       = nullptr;
};