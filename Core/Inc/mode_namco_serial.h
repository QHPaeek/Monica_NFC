/*
 * mode_namco_serial.h
 *
 *  Created on: Dec 13, 2025
 *      Author: Qinh
 */

#ifndef INC_MODE_NAMCO_SERIAL_H_
#define INC_MODE_NAMCO_SERIAL_H_

#define PN532_PREAMBLE      0x00
#define PN532_STARTCODE1   0x00
#define PN532_STARTCODE2   0xFF
#define PN532_POSTAMBLE    0x00

#define PN532_HOST_TO_PN   0xD4
#define PN532_PN_TO_HOST   0xD5
#define PN532_TFI_RESPONSE  0xD5

#define PN532_MAX_DATA_LEN 64

typedef union
{
    uint8_t raw[128];

    struct{
        uint8_t preamble;
        uint8_t startCode1;
        uint8_t startCode2;
        uint8_t len;
        uint8_t lcs;
        uint8_t tfi;
        uint8_t cmd;
        uint8_t data[PN532_MAX_DATA_LEN];
        uint8_t dcs;
        uint8_t postamble;
    };
} PN532_Down_Frame;

typedef union
{
    uint8_t raw[128];
    struct{
		/* 固定帧头 */
		uint8_t preamble;      // 0x00
		uint8_t startCode1;    // 0x00
		uint8_t startCode2;    // 0xFF
		uint8_t len;           // TFI + CMD + DATA
		uint8_t lcs;           // LEN 校验

		uint8_t tfi;           // 0xD5
		uint8_t cmd;           // CMD + 1
		uint8_t data[PN532_MAX_DATA_LEN];

		uint8_t dcs;           // 数据校验
		uint8_t postamble;     // 0x00
    };

} PN532_Up_Frame;

typedef enum {
    /* ---------- System Commands ---------- */
    PN532_CMD_GET_FIRMWARE_VERSION      = 0x02,
    PN532_CMD_GET_GENERAL_STATUS        = 0x04,
    PN532_CMD_READ_REGISTER             = 0x06,
    PN532_CMD_WRITE_REGISTER            = 0x08,
    PN532_CMD_READ_GPIO                 = 0x0C,
    PN532_CMD_WRITE_GPIO                = 0x0E,
    PN532_CMD_SET_SERIAL_BAUDRATE       = 0x10,
    PN532_CMD_SET_PARAMETERS            = 0x12,
    PN532_CMD_SAM_CONFIGURATION         = 0x14,
    PN532_CMD_POWER_DOWN                = 0x16,

    /* ---------- RF Configuration ---------- */
    PN532_CMD_RF_CONFIGURATION          = 0x32,

    /* ---------- Initiator Commands ---------- */
    PN532_CMD_IN_LIST_PASSIVE_TARGET    = 0x4A,
    PN532_CMD_IN_AUTO_POLL              = 0x60,
    PN532_CMD_IN_SELECT                 = 0x54,
    PN532_CMD_IN_DESELECT               = 0x44,
    PN532_CMD_IN_RELEASE                = 0x52,

    PN532_CMD_IN_DATA_EXCHANGE           = 0x40,
    PN532_CMD_IN_COMMUNICATE_THRU        = 0x42,

    /* ---------- P2P / DEP ---------- */
    PN532_CMD_IN_JUMP_FOR_DEP            = 0x56,
    PN532_CMD_IN_JUMP_FOR_PSL            = 0x46,
    PN532_CMD_IN_PSL                    = 0x4E,
    PN532_CMD_IN_ATR                    = 0x50,
    PN532_CMD_IN_DEP                    = 0x54,

    /* ---------- Target / Card Emulation ---------- */
    PN532_CMD_TG_INIT_AS_TARGET          = 0x8C,
    PN532_CMD_TG_SET_GENERAL_BYTES       = 0x92,
    PN532_CMD_TG_GET_DATA                = 0x86,
    PN532_CMD_TG_SET_DATA                = 0x8E,
    PN532_CMD_TG_RESPONSE_TO_INITIATOR   = 0x90,

	/* */
	PN532_CMD_DIAGNOSE = 0x18,

} pn532_command_t;

typedef enum {
    MIFARE_CMD_AUTH_A     = 0x60,
    MIFARE_CMD_AUTH_B     = 0x61,
    MIFARE_CMD_READ       = 0x30,
    MIFARE_CMD_WRITE      = 0xA0,
    MIFARE_CMD_INCREMENT  = 0xC1,
    MIFARE_CMD_DECREMENT  = 0xC0,
    MIFARE_CMD_TRANSFER   = 0xB0,
    MIFARE_CMD_RESTORE    = 0xC2,
} mifare_command_t;

typedef enum {
    FELICA_CMD_POLLING                  = 0x00,
    FELICA_CMD_READ_WO_ENCRYPTION       = 0x06,
    FELICA_CMD_WRITE_WO_ENCRYPTION      = 0x08,
    FELICA_CMD_REQUEST_SYSTEM_CODE      = 0x0C,
} felica_command_t;

#define PN532_STATUS_OK              0x00
#define PN532_STATUS_AUTH_ERROR      0x14
#define PN532_STATUS_GENERIC_ERROR   0x01

void pn532_in_data_exchange_proc(uint8_t *buf);
bool PN532_SendResponse(const uint8_t *data, uint8_t data_len);
void pn532_rsp_list_passive_target(void);

#endif /* INC_MODE_NAMCO_SERIAL_H_ */
