#include "SerialMonitor.h"
#include <QSerialPortInfo>

SerialMonitor::SerialMonitor(QObject* parent)
    : QObject(parent)
{}

SerialMonitor::~SerialMonitor()
{
    close();
}

QStringList SerialMonitor::availablePorts()
{
    QStringList ports;
    for (const QSerialPortInfo& info : QSerialPortInfo::availablePorts())
        ports << info.portName();
    return ports;
}

bool SerialMonitor::open(const QString& portName, int baudRate)
{
    close();

    serial_ = new QSerialPort(this);
    serial_->setPortName(portName);
    serial_->setBaudRate(baudRate);
    serial_->setDataBits(QSerialPort::Data8);
    serial_->setParity(QSerialPort::NoParity);
    serial_->setStopBits(QSerialPort::OneStop);
    serial_->setFlowControl(QSerialPort::NoFlowControl);

    connect(serial_, &QSerialPort::readyRead,
            this,    &SerialMonitor::onReadyRead);
    connect(serial_, &QSerialPort::errorOccurred,
            this,    &SerialMonitor::onErrorOccurred);

    if (!serial_->open(QIODevice::ReadOnly)) {
        emit errorOccurred("Cannot open " + portName + ": " + serial_->errorString());
        delete serial_;
        serial_ = nullptr;
        return false;
    }
    return true;
}

void SerialMonitor::close()
{
    if (serial_) {
        serial_->close();
        serial_->deleteLater();
        serial_ = nullptr;
    }
    lineBuffer_.clear();
}

bool SerialMonitor::isOpen() const
{
    return serial_ && serial_->isOpen();
}

void SerialMonitor::onReadyRead()
{
    QByteArray raw = serial_->readAll();
    for (char c : raw) {
        if (c == '\n') {
            // Strip trailing \r if present
            if (!lineBuffer_.isEmpty() && lineBuffer_.back() == '\r')
                lineBuffer_.chop(1);
            if (!lineBuffer_.isEmpty())
                emit lineReceived(lineBuffer_);
            lineBuffer_.clear();
        } else {
            lineBuffer_ += c;
        }
    }
}

void SerialMonitor::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError)
        emit errorOccurred("Serial error: " + serial_->errorString());
}
