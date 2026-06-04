#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include "../canopen/SdoClient.h"
#include "../model/DeviceConfig.h"

class ConfigTab : public QWidget {
    Q_OBJECT
public:
    explicit ConfigTab(SdoClient* sdo, QWidget* parent = nullptr);

private slots:
    void onReadAll();
    void onWriteAll();
    void onSaveToFlash();
    void onReadApn();
    void onWriteApn();
    void onReadApn2();
    void onWriteApn2();
    void onReadPollInterval();
    void onWritePollInterval();
    void onReadConnectivityInterval();
    void onWriteConnectivityInterval();
    void onReadPhone(int idx);
    void onWritePhone(int idx);

private:
    void applyConfig(const DeviceConfig& cfg);
    DeviceConfig collectConfig() const;
    void setStatus(const QString& msg, bool ok = true);
    void setBusy(bool busy);

    SdoClient*   sdo_;
    QLineEdit*   apnEdit_;
    QLineEdit*   apn2Edit_;
    QSpinBox*    pollIntervalSpin_;
    QSpinBox*    connectivityIntervalSpin_;
    QLineEdit*   whitelistEdits_[DeviceConfig::MAX_WHITELIST_ENTRIES];
    QPushButton* readBtn_;
    QPushButton* writeBtn_;
    QPushButton* saveBtn_;
    QLabel*      statusLabel_;

    // Per-field buttons
    QPushButton* apnReadBtn_;
    QPushButton* apnWriteBtn_;
    QPushButton* apn2ReadBtn_;
    QPushButton* apn2WriteBtn_;
    QPushButton* pollReadBtn_;
    QPushButton* pollWriteBtn_;
    QPushButton* connReadBtn_;
    QPushButton* connWriteBtn_;
    QPushButton* phoneReadBtns_[DeviceConfig::MAX_WHITELIST_ENTRIES];
    QPushButton* phoneWriteBtns_[DeviceConfig::MAX_WHITELIST_ENTRIES];

    static constexpr uint8_t  NODE_ID      = 0x02;
    static constexpr uint16_t OD_APN       = 0x6006;
    static constexpr uint16_t OD_APN2      = 0x600A;
    static constexpr uint16_t OD_POLL_INT  = 0x6007;
    static constexpr uint16_t OD_CONN_INT  = 0x600B;
    static constexpr uint16_t OD_WHITELIST = 0x6003;
    static constexpr uint16_t OD_SAVE      = 0x6008;
};
