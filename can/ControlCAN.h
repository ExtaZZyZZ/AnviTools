#pragma once
/**
 * ControlCAN.h — Waveshare USB-CAN-A API definitions
 * Structures and function pointer typedefs matching ControlCAN.dll ABI.
 * The DLL is loaded at runtime via QLibrary — no static import lib needed.
 */

#include <cstdint>

// Device type for Waveshare USB-CAN-A
#define VCI_USBCAN2   4u

#define STATUS_OK     1u
#define STATUS_ERR    0u

#pragma pack(push, 1)

/**
 * CAN bus initialization config.
 * Full 14-byte struct — all fields required (missing Reserved causes wrong timing).
 * For 1000 kbps (1 Mbps) on SJA1000: Timing0=0x00, Timing1=0x14
 */
struct VCI_INIT_CONFIG {
    uint32_t AccCode;   // Acceptance code (0x00000000 = accept all)
    uint32_t AccMask;   // Acceptance mask  (0xFFFFFFFF = accept all)
    uint32_t Reserved;  // Must be 0
    uint8_t  Filter;    // 0 = dual filter, 1 = single filter
    uint8_t  Timing0;   // SJA1000 BTR0
    uint8_t  Timing1;   // SJA1000 BTR1
    uint8_t  Mode;      // 0 = Normal, 1 = Listen-only, 2 = Loopback
};

/** CAN frame structure for transmit and receive */
struct VCI_CAN_OBJ {
    uint32_t ID;           // CAN message identifier
    uint32_t TimeStamp;    // Receive timestamp (0.1ms units, RX only)
    uint8_t  TimeFlag;     // 1 = timestamp valid (RX only)
    uint8_t  SendType;     // TX: 0 = normal, 1 = single shot
    uint8_t  RemoteFlag;   // 1 = remote frame
    uint8_t  ExternFlag;   // 0 = 11-bit, 1 = 29-bit extended
    uint8_t  DataLen;      // Payload length (0-8)
    uint8_t  Data[8];      // Payload bytes
    uint8_t  Reserved[3];
};

#pragma pack(pop)

// Function pointer types for QLibrary::resolve()
using PFN_VCI_OpenDevice    = uint32_t (*)(uint32_t DevType, uint32_t DevIndex, uint32_t Reserved);
using PFN_VCI_CloseDevice   = uint32_t (*)(uint32_t DevType, uint32_t DevIndex);
using PFN_VCI_InitCAN       = uint32_t (*)(uint32_t DevType, uint32_t DevIndex, uint32_t CANIndex,
                                            VCI_INIT_CONFIG* pInitConfig);
using PFN_VCI_StartCAN      = uint32_t (*)(uint32_t DevType, uint32_t DevIndex, uint32_t CANIndex);
using PFN_VCI_Transmit      = uint32_t (*)(uint32_t DevType, uint32_t DevIndex, uint32_t CANIndex,
                                            VCI_CAN_OBJ* pSend, uint32_t Len);
using PFN_VCI_Receive       = uint32_t (*)(uint32_t DevType, uint32_t DevIndex, uint32_t CANIndex,
                                            VCI_CAN_OBJ* pReceive, uint32_t Len, uint32_t WaitTime);
using PFN_VCI_GetReceiveNum = uint32_t (*)(uint32_t DevType, uint32_t DevIndex, uint32_t CANIndex);
using PFN_VCI_ClearBuffer   = uint32_t (*)(uint32_t DevType, uint32_t DevIndex, uint32_t CANIndex);
