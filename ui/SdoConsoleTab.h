#pragma once
#include <QWidget>
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include "../canopen/SdoClient.h"

class SdoConsoleTab : public QWidget {
    Q_OBJECT
public:
    explicit SdoConsoleTab(SdoClient* sdo, QWidget* parent = nullptr);

private slots:
    void onRead();
    void onWrite();

private:
    void log(const QString& msg);
    void setBusy(bool busy);
    void applyKnownTypePreset();
    QByteArray valueToBytes(const QString& text, const QString& type, QString& err);
    QString bytesToString(const QByteArray& data, const QString& type);

    SdoClient*   sdo_;
    QLineEdit*   nodeIdEdit_;     // hex input "0x02"
    QLineEdit*   indexEdit_;      // hex input "0x6000"
    QSpinBox*    subIndexSpin_;
    QComboBox*   dataTypeBox_;    // int8, uint8, uint16, uint32, string
    QLineEdit*   valueEdit_;      // read result shown here / write value
    QPushButton* readBtn_;
    QPushButton* writeBtn_;
    QTextEdit*   historyLog_;
};
