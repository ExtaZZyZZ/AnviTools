#pragma once
#include <QString>
#include <QStringList>

/**
 * DeviceConfig — mirrors the runtime-configurable fields stored in firmware
 * internal flash (ConfigNS::InternalFlashConfigManager).
 */
struct DeviceConfig {
    QString     apn;                        // APN основного канала (max 64 chars)
    QString     apn2;                       // APN резервного канала (max 64 chars)
    uint32_t    sensorPollIntervalSec = 5;   // Sensor polling period
    uint32_t    connectivityCheckSec  = 60; // Connectivity check interval (AT+CSQ)
    QStringList whitelist;                   // Phone whitelist (up to 10 numbers)

    static constexpr int MAX_WHITELIST_ENTRIES = 10;
    static constexpr int MAX_APN_LEN           = 64;
    static constexpr int MAX_NUMBER_LEN        = 15;
};
