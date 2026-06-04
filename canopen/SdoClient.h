#pragma once
#include <cstdint>
#include <QObject>
#include <QByteArray>
#include <QString>
#include "../can/CanDriver.h"

/**
 * SdoClient — CANopen SDO client (expedited + segmented).
 *
 * Lives on the main thread. Uses QEventLoop+QTimer pattern to block
 * until an SDO response arrives (or timeout). UI buttons must be
 * disabled before calling sdoRead/sdoWrite to prevent re-entrancy.
 *
 * COB-ID convention:
 *   Request  → node: 0x600 + nodeId
 *   Response ← node: 0x580 + nodeId
 */
class SdoClient : public QObject {
    Q_OBJECT
public:
    explicit SdoClient(CanDriver* driver, QObject* parent = nullptr);

    void setTimeout(int ms) { timeoutMs_ = ms; }

    /**
     * Expedited SDO write (data ≤ 4 bytes).
     * @param data  1–4 bytes, little-endian value
     */
    bool sdoWrite(uint8_t nodeId, uint16_t index, uint8_t subIndex,
                  const QByteArray& data, QString& errorMsg);

    /**
     * Expedited SDO read.
     * @param dataOut  Received payload (1–4 bytes, little-endian)
     */
    bool sdoRead(uint8_t nodeId, uint16_t index, uint8_t subIndex,
                 QByteArray& dataOut, QString& errorMsg);

    /**
     * Segmented SDO write for strings (up to ~256 bytes).
     * Builds initiate + segment frames automatically.
     */
    bool sdoWriteString(uint8_t nodeId, uint16_t index, uint8_t subIndex,
                        const QString& str, QString& errorMsg);

    /**
     * Segmented SDO read for strings.
     */
    bool sdoReadString(uint8_t nodeId, uint16_t index, uint8_t subIndex,
                       QString& strOut, QString& errorMsg);

    // Convenience helpers for typed values
    bool writeInt8 (uint8_t nodeId, uint16_t idx, uint8_t sub, int8_t   v, QString& e);
    bool writeUInt8 (uint8_t nodeId, uint16_t idx, uint8_t sub, uint8_t  v, QString& e);
    bool writeUInt16(uint8_t nodeId, uint16_t idx, uint8_t sub, uint16_t v, QString& e);
    bool writeUInt32(uint8_t nodeId, uint16_t idx, uint8_t sub, uint32_t v, QString& e);
    bool readInt8  (uint8_t nodeId, uint16_t idx, uint8_t sub, int8_t&   v, QString& e);
    bool readUInt8  (uint8_t nodeId, uint16_t idx, uint8_t sub, uint8_t&  v, QString& e);
    bool readUInt16 (uint8_t nodeId, uint16_t idx, uint8_t sub, uint16_t& v, QString& e);
    bool readUInt32 (uint8_t nodeId, uint16_t idx, uint8_t sub, uint32_t& v, QString& e);

public slots:
    /**
     * Connected to CanWorker::frameReceived via Qt::QueuedConnection.
     * Filters SDO response frames (0x580+nodeId) and wakes the pending QEventLoop.
     */
    void onFrameReceived(uint32_t cobId, QByteArray data);

private:
    CanDriver*  driver_;
    int         timeoutMs_     = 1000;

    // State shared between sdoRead/sdoWrite and onFrameReceived
    uint8_t     pendingNode_   = 0xFF;
    uint16_t    pendingIndex_  = 0;
    uint8_t     pendingSubIdx_ = 0;
    QByteArray  pendingResp_;
    bool        responseReady_ = false;

    static constexpr uint16_t SDO_TX_BASE = 0x600; // PC → device
    static constexpr uint16_t SDO_RX_BASE = 0x580; // device → PC

    bool transmitFrame(uint8_t nodeId, const QByteArray& frame);
    bool waitForResponse(uint8_t nodeId, uint16_t index, uint8_t subIndex,
                         QByteArray& out, int timeoutMs);

    QByteArray buildExpDownload(uint16_t index, uint8_t subIndex, const QByteArray& data);
    QByteArray buildExpUpload  (uint16_t index, uint8_t subIndex);

    QString abortCodeToString(uint32_t code);
};
