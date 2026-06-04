#include "CanDriver.h"
#include <QMutexLocker>

CanDriver::CanDriver(QObject* parent) : QObject(parent) {}

CanDriver::~CanDriver() { close(); }

QStringList CanDriver::availablePorts()
{
    QStringList list;
    for (const QSerialPortInfo& info : QSerialPortInfo::availablePorts())
        list << info.portName();
    return list;
}

bool CanDriver::open(const QString& portName, int serialBaud, QString* errorMsg)
{
    close();

    serial_ = new QSerialPort(this);
    serial_->setPortName(portName);
    serial_->setBaudRate(serialBaud);
    serial_->setDataBits(QSerialPort::Data8);
    serial_->setParity(QSerialPort::NoParity);
    serial_->setStopBits(QSerialPort::OneStop);
    serial_->setFlowControl(QSerialPort::NoFlowControl);

    connect(serial_, &QSerialPort::readyRead,
            this,    &CanDriver::onReadyRead);
    connect(serial_, &QSerialPort::errorOccurred,
            this,    &CanDriver::onSerialError);

    if (!serial_->open(QIODevice::ReadWrite)) {
        if (errorMsg) *errorMsg = "Cannot open " + portName + ": " + serial_->errorString();
        delete serial_;
        serial_ = nullptr;
        return false;
    }

    return true;
}

void CanDriver::close()
{
    if (serial_) {
        serial_->close();
        serial_->deleteLater();
        serial_ = nullptr;
    }
    rxBuf_.clear();
}

bool CanDriver::isOpen() const
{
    return serial_ && serial_->isOpen();
}

bool CanDriver::transmit(uint32_t cobId, const uint8_t* data)
{
    QMutexLocker lock(&txMutex_);
    if (!isOpen()) return false;

    // 20-byte protocol:
    // [0]    0xAA
    // [1]    0x55
    // [2]    0x01  = data frame (0x02 = remote)
    // [3]    0x01  = standard 11-bit  (0x02 = extended 29-bit)
    // [4]    0x01  = channel
    // [5-8]  CAN ID, little-endian 4 bytes
    // [9]    DLC (data length, 0-8)
    // [10-17] data, 8 bytes (zero-padded)
    // [18]   0x00 reserved
    // [19]   checksum = sum(bytes[2..18]) & 0xFF

    QByteArray frame(20, '\x00');
    frame[0]  = static_cast<char>(0xAA);
    frame[1]  = static_cast<char>(0x55);
    frame[2]  = static_cast<char>(0x01);                          // data frame
    frame[3]  = static_cast<char>(cobId > 0x7FF ? 0x02 : 0x01);  // ext / std
    frame[4]  = static_cast<char>(0x01);                          // channel
    frame[5]  = static_cast<char>(cobId & 0xFF);
    frame[6]  = static_cast<char>((cobId >> 8)  & 0xFF);
    frame[7]  = static_cast<char>((cobId >> 16) & 0xFF);
    frame[8]  = static_cast<char>((cobId >> 24) & 0xFF);
    frame[9]  = static_cast<char>(0x08);                          // DLC = 8
    for (int i = 0; i < 8; ++i)
        frame[10 + i] = static_cast<char>(data[i]);
    // byte 18 already 0x00

    uint8_t chk = 0;
    for (int i = 2; i <= 18; ++i)
        chk += static_cast<uint8_t>(frame[i]);
    frame[19] = static_cast<char>(chk);

    bool ok = (serial_->write(frame) == frame.size());
    if (ok) {
        QByteArray payload(reinterpret_cast<const char*>(data), 8);
        emit frameTransmitted(cobId, payload);
    }
    return ok;
}

void CanDriver::onReadyRead()
{
    QByteArray chunk = serial_->readAll();
    emit rawBytesReceived(chunk);  // diagnostic: raw bytes before parsing
    rxBuf_ += chunk;
    parseBuffer();
}

void CanDriver::onSerialError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError && serial_)
        emit errorOccurred("Serial error: " + serial_->errorString());
}

void CanDriver::parseBuffer()
{
    // Waveshare USB-CAN-A fixed 20-byte protocol:
    // [0-1]  0xAA 0x55  (header)
    // [2]    frame type (0x01=data)
    // [3]    id type    (0x01=std, 0x02=ext)
    // [4]    channel    (0x01)
    // [5-8]  CAN ID, little-endian
    // [9]    DLC (0-8)
    // [10-17] data, 8 bytes (zero-padded)
    // [18]   reserved 0x00
    // [19]   checksum = sum(bytes[2..18]) & 0xFF

    while (rxBuf_.size() >= FRAME_LEN) {
        // Find 0xAA 0x55 header
        int start = -1;
        for (int i = 0; i <= rxBuf_.size() - 2; ++i) {
            if (static_cast<uint8_t>(rxBuf_[i])   == 0xAA &&
                static_cast<uint8_t>(rxBuf_[i+1]) == 0x55) {
                start = i;
                break;
            }
        }
        if (start < 0) { rxBuf_.clear(); return; }
        if (start > 0) rxBuf_.remove(0, start);
        if (rxBuf_.size() < FRAME_LEN) return;

        // Verify checksum
        uint8_t chk = 0;
        for (int i = 2; i <= 18; ++i)
            chk += static_cast<uint8_t>(rxBuf_[i]);
        if (chk != static_cast<uint8_t>(rxBuf_[19])) {
            rxBuf_.remove(0, 1); // bad checksum — skip 1 byte and retry
            continue;
        }

        // Extract CAN ID (little-endian, bytes 5-8)
        uint32_t cobId =
            static_cast<uint32_t>(static_cast<uint8_t>(rxBuf_[5]))        |
            (static_cast<uint32_t>(static_cast<uint8_t>(rxBuf_[6])) << 8) |
            (static_cast<uint32_t>(static_cast<uint8_t>(rxBuf_[7])) << 16)|
            (static_cast<uint32_t>(static_cast<uint8_t>(rxBuf_[8])) << 24);

        // Data starts at byte 10 (byte 9 = DLC)
        QByteArray payload = rxBuf_.mid(10, 8);
        rxBuf_.remove(0, FRAME_LEN);

        emit frameReceived(cobId, payload);
    }
}
