#pragma once
#include <QObject>
#include <QSerialPort>

/**
 * SerialMonitor — reads UART2 debug output from the firmware (115200 baud).
 * Must be moved to a dedicated QThread before calling open().
 * Accumulates bytes into a line buffer and emits lineReceived on each '\n'.
 */
class SerialMonitor : public QObject {
    Q_OBJECT
public:
    explicit SerialMonitor(QObject* parent = nullptr);
    ~SerialMonitor();

    /** Returns list of available system serial ports. */
    static QStringList availablePorts();

    /** Open serial port. Call from the worker thread. */
    bool open(const QString& portName, int baudRate = 115200);
    void close();
    bool isOpen() const;

signals:
    void lineReceived(QString line);
    void errorOccurred(QString msg);

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    QSerialPort* serial_     = nullptr;
    QString      lineBuffer_;
};
