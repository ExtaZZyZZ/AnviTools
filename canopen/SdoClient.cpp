#include "SdoClient.h"
#include <QEventLoop>
#include <QTimer>
#include <cstring>

SdoClient::SdoClient(CanDriver* driver, QObject* parent)
    : QObject(parent), driver_(driver)
{}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool SdoClient::sdoWrite(uint8_t nodeId, uint16_t index, uint8_t subIndex,
                          const QByteArray& data, QString& errorMsg)
{
    if (data.size() < 1 || data.size() > 4) {
        errorMsg = "Expedited SDO: data must be 1-4 bytes. Use sdoWriteString for longer data.";
        return false;
    }
    QByteArray frame = buildExpDownload(index, subIndex, data);
    if (!transmitFrame(nodeId, frame)) {
        errorMsg = "CAN transmit failed";
        return false;
    }
    QByteArray resp;
    if (!waitForResponse(nodeId, index, subIndex, resp, timeoutMs_)) {
        errorMsg = "SDO timeout (no response from node 0x" +
                   QString::number(nodeId, 16).toUpper() + ")";
        return false;
    }
    uint8_t cs = static_cast<uint8_t>(resp[0]);
    if (cs == 0x80) {
        uint32_t abort = 0;
        std::memcpy(&abort, resp.constData() + 4, 4);
        errorMsg = "SDO abort: " + abortCodeToString(abort);
        return false;
    }
    if ((cs & 0xE0) != 0x60) {
        errorMsg = QString("Unexpected SDO response cs=0x%1").arg(cs, 2, 16, QChar('0'));
        return false;
    }
    return true;
}

bool SdoClient::sdoRead(uint8_t nodeId, uint16_t index, uint8_t subIndex,
                         QByteArray& dataOut, QString& errorMsg)
{
    QByteArray frame = buildExpUpload(index, subIndex);
    if (!transmitFrame(nodeId, frame)) {
        errorMsg = "CAN transmit failed";
        return false;
    }
    QByteArray resp;
    if (!waitForResponse(nodeId, index, subIndex, resp, timeoutMs_)) {
        errorMsg = "SDO timeout (no response from node 0x" +
                   QString::number(nodeId, 16).toUpper() + ")";
        return false;
    }
    uint8_t cs = static_cast<uint8_t>(resp[0]);
    if (cs == 0x80) {
        uint32_t abort = 0;
        std::memcpy(&abort, resp.constData() + 4, 4);
        errorMsg = "SDO abort: " + abortCodeToString(abort);
        return false;
    }
    // Upload initiate response: cs = 0x4x
    if ((cs & 0xF0) != 0x40) {
        errorMsg = QString("Unexpected SDO response cs=0x%1").arg(cs, 2, 16, QChar('0'));
        return false;
    }
    bool expedited = (cs & 0x02) != 0;
    if (expedited) {
        // Expedited: data fits in the initiate response (1–4 bytes)
        bool sizeIndicated = (cs & 0x01) != 0;
        int dataLen = sizeIndicated ? (4 - ((cs >> 2) & 0x03)) : 4;
        dataOut = resp.mid(4, dataLen);
        return true;
    }
    // Segmented upload: request segments until last
    QByteArray collected;
    uint8_t toggle = 0;
    while (true) {
        QByteArray req(8, 0x00);
        req[0] = static_cast<char>(0x60 | (toggle << 4));
        if (!transmitFrame(nodeId, req)) {
            errorMsg = "CAN transmit failed (segment request)";
            return false;
        }
        QByteArray sresp;
        if (!waitForResponse(nodeId, index, subIndex, sresp, timeoutMs_)) {
            errorMsg = "SDO segment read timeout";
            return false;
        }
        uint8_t scs = static_cast<uint8_t>(sresp[0]);
        if ((scs & 0x80) != 0) {
            uint32_t abort = 0;
            std::memcpy(&abort, sresp.constData() + 4, 4);
            errorMsg = "SDO abort during segment: " + abortCodeToString(abort);
            return false;
        }
        uint8_t n    = (scs >> 1) & 0x07;  // unused bytes in this segment
        bool    last = (scs & 0x01) != 0;
        collected.append(sresp.mid(1, 7 - n));
        if (last) break;
        toggle ^= 1;
    }
    dataOut = collected;
    return true;
}

bool SdoClient::sdoWriteString(uint8_t nodeId, uint16_t index, uint8_t subIndex,
                                const QString& str, QString& errorMsg)
{
    QByteArray payload = str.toUtf8();
    uint32_t totalSize = static_cast<uint32_t>(payload.size());

    // Initiate segmented download with size indicated
    // cs = 0x21: bits[7:5]=001 (download initiate), bit1=0 (segmented), bit0=1 (size indicated)
    QByteArray initFrame(8, 0x00);
    initFrame[0] = 0x21;
    initFrame[1] = static_cast<char>(index & 0xFF);
    initFrame[2] = static_cast<char>((index >> 8) & 0xFF);
    initFrame[3] = static_cast<char>(subIndex);
    std::memcpy(initFrame.data() + 4, &totalSize, 4);

    if (!transmitFrame(nodeId, initFrame)) { errorMsg = "CAN transmit failed"; return false; }
    QByteArray resp;
    if (!waitForResponse(nodeId, index, subIndex, resp, timeoutMs_)) {
        errorMsg = "SDO segmented init timeout"; return false;
    }
    {
        uint8_t ics = static_cast<uint8_t>(resp[0]);
        if (ics == 0x80) {
            uint32_t abort = 0;
            std::memcpy(&abort, resp.constData() + 4, 4);
            errorMsg = "SDO abort on init: " + abortCodeToString(abort);
            return false;
        }
        if ((ics & 0xE0) != 0x60) {
            errorMsg = QString("SDO segmented init rejected (cs=0x%1)").arg(ics, 2, 16, QChar('0'));
            return false;
        }
    }

    // Send segments
    int offset = 0;
    uint8_t toggle = 0;
    while (offset < payload.size()) {
        int remaining = payload.size() - offset;
        int segLen = qMin(remaining, 7);
        bool last = (segLen == remaining);

        QByteArray seg(8, 0x00);
        uint8_t n = static_cast<uint8_t>(7 - segLen); // unused bytes
        seg[0] = static_cast<char>((toggle << 4) | (n << 1) | (last ? 1 : 0));
        for (int i = 0; i < segLen; ++i)
            seg[1 + i] = payload[offset + i];

        if (!transmitFrame(nodeId, seg)) { errorMsg = "CAN transmit failed"; return false; }
        QByteArray sresp;
        // Segment response: cobId = 0x580+nodeId, cs = 0x20 | (toggle<<4)
        if (!waitForResponse(nodeId, index, subIndex, sresp, timeoutMs_)) {
            errorMsg = "SDO segment timeout"; return false;
        }
        uint8_t scs = static_cast<uint8_t>(sresp[0]);
        if ((scs & 0x80) != 0) {
            uint32_t abort = 0; std::memcpy(&abort, sresp.constData() + 4, 4);
            errorMsg = "SDO abort during segment: " + abortCodeToString(abort);
            return false;
        }

        toggle ^= 1;
        offset += segLen;
    }
    return true;
}

bool SdoClient::sdoReadString(uint8_t nodeId, uint16_t index, uint8_t subIndex,
                               QString& strOut, QString& errorMsg)
{
    // Initiate upload
    QByteArray initFrame = buildExpUpload(index, subIndex);
    if (!transmitFrame(nodeId, initFrame)) { errorMsg = "CAN transmit failed"; return false; }
    QByteArray resp;
    if (!waitForResponse(nodeId, index, subIndex, resp, timeoutMs_)) {
        errorMsg = "SDO upload init timeout"; return false;
    }
    uint8_t cs = static_cast<uint8_t>(resp[0]);
    if (cs == 0x80) {
        uint32_t abort = 0; std::memcpy(&abort, resp.constData() + 4, 4);
        errorMsg = "SDO abort: " + abortCodeToString(abort); return false;
    }

    // If expedited — data fits in the initiate response
    if ((cs & 0x02) != 0) {
        int dataLen = ((cs & 0x01) != 0) ? (4 - ((cs >> 2) & 0x03)) : 4;
        strOut = QString::fromUtf8(resp.mid(4, dataLen));
        return true;
    }

    // Segmented upload
    QByteArray collected;
    uint8_t toggle = 0;
    while (true) {
        QByteArray req(8, 0x00);
        req[0] = static_cast<char>(0x60 | (toggle << 4));
        if (!transmitFrame(nodeId, req)) { errorMsg = "CAN transmit failed"; return false; }
        QByteArray sresp;
        if (!waitForResponse(nodeId, index, subIndex, sresp, timeoutMs_)) {
            errorMsg = "SDO segment read timeout"; return false;
        }
        uint8_t scs = static_cast<uint8_t>(sresp[0]);
        if ((scs & 0x80) != 0) {
            uint32_t abort = 0; std::memcpy(&abort, sresp.constData() + 4, 4);
            errorMsg = "SDO abort: " + abortCodeToString(abort); return false;
        }
        uint8_t n    = (scs >> 1) & 0x07; // unused bytes in segment
        bool    last = (scs & 0x01) != 0;
        collected.append(sresp.mid(1, 7 - n));
        if (last) break;
        toggle ^= 1;
    }
    strOut = QString::fromUtf8(collected);
    return true;
}

// ---------------------------------------------------------------------------
// Typed convenience helpers
// ---------------------------------------------------------------------------

bool SdoClient::writeInt8(uint8_t n, uint16_t i, uint8_t s, int8_t v, QString& e) {
    QByteArray d;
    d.append(static_cast<char>(v));
    return sdoWrite(n, i, s, d, e);
}
bool SdoClient::writeUInt8(uint8_t n, uint16_t i, uint8_t s, uint8_t v, QString& e) {
    QByteArray d;
    d.append(static_cast<char>(v));
    return sdoWrite(n, i, s, d, e);
}
bool SdoClient::writeUInt16(uint8_t n, uint16_t i, uint8_t s, uint16_t v, QString& e) {
    QByteArray d(2, 0); std::memcpy(d.data(), &v, 2);
    return sdoWrite(n, i, s, d, e);
}
bool SdoClient::writeUInt32(uint8_t n, uint16_t i, uint8_t s, uint32_t v, QString& e) {
    QByteArray d(4, 0); std::memcpy(d.data(), &v, 4);
    return sdoWrite(n, i, s, d, e);
}
bool SdoClient::readInt8(uint8_t n, uint16_t i, uint8_t s, int8_t& v, QString& e) {
    QByteArray d; if (!sdoRead(n, i, s, d, e)) return false;
    v = static_cast<int8_t>(d[0]); return true;
}
bool SdoClient::readUInt8(uint8_t n, uint16_t i, uint8_t s, uint8_t& v, QString& e) {
    QByteArray d; if (!sdoRead(n, i, s, d, e)) return false;
    v = static_cast<uint8_t>(d[0]); return true;
}
bool SdoClient::readUInt16(uint8_t n, uint16_t i, uint8_t s, uint16_t& v, QString& e) {
    QByteArray d; if (!sdoRead(n, i, s, d, e)) return false;
    std::memcpy(&v, d.constData(), qMin(2, d.size())); return true;
}
bool SdoClient::readUInt32(uint8_t n, uint16_t i, uint8_t s, uint32_t& v, QString& e) {
    QByteArray d; if (!sdoRead(n, i, s, d, e)) return false;
    std::memcpy(&v, d.constData(), qMin(4, d.size())); return true;
}

// ---------------------------------------------------------------------------
// Slot: called by CanWorker (QueuedConnection → main thread)
// ---------------------------------------------------------------------------

void SdoClient::onFrameReceived(uint32_t cobId, QByteArray data)
{
    if (pendingNode_ == 0xFF) return;
    uint32_t expectedCob = SDO_RX_BASE + pendingNode_;
    if (cobId != expectedCob) return;
    if (data.size() < 8) return;

    // Verify index + subindex match (bytes 1,2,3) for non-abort responses
    uint8_t cs = static_cast<uint8_t>(data[0]);
    bool isAbort   = (cs == 0x80);
    bool isSdoResp = ((cs & 0xE0) == 0x60) || ((cs & 0xF0) == 0x40) ||
                     ((cs & 0xE0) == 0x00) || ((cs & 0xE0) == 0x20) || isAbort;

    if (!isSdoResp) return;

    if (!isAbort) {
        uint16_t rxIdx = static_cast<uint8_t>(data[1]) |
                         (static_cast<uint8_t>(data[2]) << 8);
        uint8_t rxSub  = static_cast<uint8_t>(data[3]);
        // Segment responses don't embed index — accept them if we're in a session
        bool isSegment = ((cs & 0xE0) == 0x00) || ((cs & 0xE0) == 0x20);
        if (!isSegment && (rxIdx != pendingIndex_ || rxSub != pendingSubIdx_)) return;
    }

    pendingResp_   = data;
    responseReady_ = true;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool SdoClient::transmitFrame(uint8_t nodeId, const QByteArray& frame)
{
    uint8_t buf[8] = {};
    int len = qMin(frame.size(), 8);
    std::memcpy(buf, frame.constData(), len);
    return driver_->transmit(SDO_TX_BASE + nodeId, buf);
}

bool SdoClient::waitForResponse(uint8_t nodeId, uint16_t index, uint8_t subIndex,
                                 QByteArray& out, int timeoutMs)
{
    pendingNode_   = nodeId;
    pendingIndex_  = index;
    pendingSubIdx_ = subIndex;
    responseReady_ = false;
    pendingResp_.clear();

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMs);

    // Poll responseReady_ flag inside event loop
    QTimer pollTimer;
    pollTimer.setInterval(1);
    connect(&pollTimer, &QTimer::timeout, [&]() {
        if (responseReady_) loop.quit();
    });
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    pollTimer.start();
    timer.start();
    loop.exec();
    pollTimer.stop();
    timer.stop();

    pendingNode_ = 0xFF;

    if (!responseReady_) return false;
    out = pendingResp_;
    return true;
}

QByteArray SdoClient::buildExpDownload(uint16_t index, uint8_t subIndex, const QByteArray& data)
{
    QByteArray frame(8, 0x00);
    int n = qMin(data.size(), 4);
    // cs = 0x20 | ((4-n)<<2) | 0x03  (expedited, size indicated)
    frame[0] = static_cast<char>(0x20 | ((4 - n) << 2) | 0x03);
    frame[1] = static_cast<char>(index & 0xFF);
    frame[2] = static_cast<char>((index >> 8) & 0xFF);
    frame[3] = static_cast<char>(subIndex);
    for (int i = 0; i < n; ++i)
        frame[4 + i] = data[i];
    return frame;
}

QByteArray SdoClient::buildExpUpload(uint16_t index, uint8_t subIndex)
{
    QByteArray frame(8, 0x00);
    frame[0] = static_cast<char>(0x40);
    frame[1] = static_cast<char>(index & 0xFF);
    frame[2] = static_cast<char>((index >> 8) & 0xFF);
    frame[3] = static_cast<char>(subIndex);
    return frame;
}

QString SdoClient::abortCodeToString(uint32_t code)
{
    switch (code) {
    case 0x05030000: return "Toggle bit error";
    case 0x05040000: return "SDO timeout";
    case 0x05040001: return "Invalid command specifier";
    case 0x06010000: return "Unsupported access";
    case 0x06010001: return "Read access on write-only object";
    case 0x06010002: return "Write access on read-only object";
    case 0x06020000: return "Object does not exist in OD";
    case 0x06040041: return "Object cannot be mapped to PDO";
    case 0x06040042: return "PDO length exceeded";
    case 0x06040043: return "Parameter incompatibility";
    case 0x06040047: return "Internal incompatibility";
    case 0x06060000: return "Hardware error";
    case 0x06070010: return "Data type mismatch";
    case 0x06070012: return "Data type mismatch - length too high";
    case 0x06070013: return "Data type mismatch - length too short";
    case 0x06090011: return "Subindex does not exist";
    case 0x06090030: return "Value range exceeded";
    case 0x06090031: return "Value too high";
    case 0x06090032: return "Value too low";
    case 0x08000000: return "General error";
    case 0x08000020: return "Data cannot be stored";
    case 0x08000021: return "Data cannot be stored — local control";
    case 0x08000022: return "Data cannot be stored — device state";
    default:         return QString("0x%1").arg(code, 8, 16, QChar('0')).toUpper();
    }
}
