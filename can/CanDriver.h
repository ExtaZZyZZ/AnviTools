#pragma once
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMutex>

/**
 * CanDriver — Waveshare USB-CAN-A driver over QSerialPort (CH341 virtual COM).
 *
 * Proprietary binary protocol (NOT ControlCAN.dll, NOT SLCAN):
 *   Frame: 0xAA [type] [IDL] [IDH] [data 0..8 bytes] 0x55
 *
 * Type byte:
 *   bit5 = 0 → standard 11-bit ID   bit5 = 1 → extended 29-bit ID
 *   bit4 = 0 → data frame            bit4 = 1 → remote frame
 *   bit3..0  = data length (0-8)
 *
 * Serial baud: 2,000,000 bps (adapter LED = 1 blink = default)
 */
class CanDriver : public QObject {
    Q_OBJECT
public:
    explicit CanDriver(QObject* parent = nullptr);
    ~CanDriver();

    static QStringList availablePorts();

    /**
     * Open serial port and configure CAN at 1000 kbps.
     * @param portName     e.g. "COM3"
     * @param serialBaud   Serial baud rate (default 2000000)
     */
    bool open(const QString& portName, int serialBaud = 2000000, QString* errorMsg = nullptr);
    void close();
    bool isOpen() const;

    /**
     * Transmit a standard 11-bit CAN data frame. Thread-safe.
     * @param cobId  11-bit CAN identifier
     * @param data   Pointer to exactly 8 bytes
     */
    bool transmit(uint32_t cobId, const uint8_t* data);

signals:
    /** Emitted from the main thread (QSerialPort::readyRead slot). */
    void frameReceived(uint32_t cobId, QByteArray data);
    /** Emitted after each successful transmit — for monitor logging. */
    void frameTransmitted(uint32_t cobId, QByteArray data);
    /** Emitted with raw bytes BEFORE parsing — use for baud rate diagnostics. */
    void rawBytesReceived(QByteArray data);
    void errorOccurred(QString msg);

private slots:
    void onReadyRead();
    void onSerialError(QSerialPort::SerialPortError error);

private:
    void parseBuffer();

    QSerialPort* serial_ = nullptr;
    QByteArray   rxBuf_;
    QMutex       txMutex_;

    // Fixed 20-byte frame length for Waveshare USB-CAN-A
    static constexpr int FRAME_LEN = 20;
};
