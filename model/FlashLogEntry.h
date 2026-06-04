#pragma once
#include <cstdint>
#include <QString>
#include <QDateTime>

/**
 * FlashLogEntry — mirrors firmware FlashNS::FlashLogEntry (8 bytes packed).
 * Layout (little-endian, matching STM32):
 *   bytes 0-3: uint32_t timestamp_unix
 *   byte  4:   uint8_t  sensor_id  (0=Sensor1, 1=Sensor2)
 *   bytes 5-6: uint16_t pressure_kPa
 *   byte  7:   uint8_t  flags (bit0=valid)
 */
struct FlashLogEntry {
    uint32_t timestamp_unix = 0;
    uint8_t  sensor_id      = 0;
    uint16_t pressure_kPa   = 0;
    uint8_t  flags          = 0;

    bool isValid() const { return (flags & 0x01) != 0; }

    /** Parse from raw 8-byte SDO payload (little-endian). */
    static FlashLogEntry fromBytes(const QByteArray& raw) {
        FlashLogEntry e;
        if (raw.size() < 8) return e;
        const auto* b = reinterpret_cast<const uint8_t*>(raw.constData());
        e.timestamp_unix = static_cast<uint32_t>(b[0])
                         | static_cast<uint32_t>(b[1]) << 8
                         | static_cast<uint32_t>(b[2]) << 16
                         | static_cast<uint32_t>(b[3]) << 24;
        e.sensor_id      = b[4];
        e.pressure_kPa   = static_cast<uint16_t>(b[5]) | static_cast<uint16_t>(b[6]) << 8;
        e.flags          = b[7];
        return e;
    }

    QString sensorName() const {
        return (sensor_id == 0) ? "Sensor 1" : "Sensor 2";
    }

    QString timestampString() const {
        QDateTime dt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(timestamp_unix),
                                                      Qt::UTC);
        return dt.toString("yyyy-MM-dd HH:mm:ss");
    }
};
