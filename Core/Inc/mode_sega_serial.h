/*
 * mode_sega_serial.h
 *
 *  Created on: Jun 27, 2025
 *      Author: Qinh
 */

#ifndef INC_MODE_SEGA_SERIAL_H_
#define INC_MODE_SEGA_SERIAL_H_

#include "stdint.h"
#include "stdbool.h"
#include "mode_manager.h"
#include "flash.h"

#define SERIAL_CMD_START 0xE0
#define SERIAL_CMD_ESCAPE 0xD0
enum {
  CMD_GET_FW_VERSION = 0x30,
  CMD_GET_HW_VERSION = 0x32,
  // Card read
  CMD_START_POLLING = 0x40,
  CMD_STOP_POLLING = 0x41,
  CMD_CARD_DETECT = 0x42,
  CMD_CARD_SELECT = 0x43,
  CMD_CARD_HALT = 0x44,
  // MIFARE
  CMD_MIFARE_KEY_SET_A = 0x50,
  CMD_MIFARE_AUTHORIZE_A = 0x51,
  CMD_MIFARE_READ = 0x52,
  CMD_MIFARE_WRITE = 0x53,
  CMD_MIFARE_KEY_SET_B = 0x54,
  CMD_MIFARE_AUTHORIZE_B = 0x55,
  // Boot,update
  CMD_TO_UPDATER_MODE = 0x60,
  CMD_SEND_HEX_DATA = 0x61,
  CMD_TO_NORMAL_MODE = 0x62,
  CMD_SEND_BINDATA_INIT = 0x63,
  CMD_SEND_BINDATA_EXEC = 0x64,
  // FeliCa
  CMD_FELICA_THROUGH = 0x71,
  // LED board
  CMD_EXT_BOARD_LED = 0x80,
  CMD_EXT_BOARD_LED_RGB = 0x81,
  CMD_EXT_BOARD_LED_RGB_UNKNOWN = 0x82,  // 未知
  CMD_EXT_BOARD_INFO = 0xf0,
  CMD_EXT_FIRM_SUM = 0xf2,
  CMD_EXT_SEND_HEX_DATA = 0xf3,
  CMD_EXT_TO_BOOT_MODE = 0xf4,
  CMD_EXT_TO_NORMAL_MODE = 0xf5,
  //读卡器上位机功能
  CMD_READ_EEPROM = 0xf6,
  CMD_WRITE_EEPROM = 0xf7,
  CMD_SW_MODE = 0xf8,
  CMD_READ_MODE = 0xf9,
};

enum {
  FelicaPolling = 0x00,
  FelicaReqResponce = 0x04,
  FelicaReadWithoutEncryptData = 0x06,
  FelicaWriteWithoutEncryptData = 0x08,
  FelicaReqSysCode = 0x0C,
  FelicaActive2 = 0xA4,
};

enum {
  STATUS_OK = 0x00,
  STATUS_CARD_ERROR = 0x01,
  STATUS_NOT_ACCEPT = 0x02,
  STATUS_INVALID_COMMAND = 0x03,
  STATUS_INVALID_DATA = 0x04,
  STATUS_SUM_ERROR = 0x05,
  STATUS_INTERNAL_ERROR = 0x06,
  STATUS_INVALID_FIRM_DATA = 0x07,
  STATUS_FIRM_UPDATE_SUCCESS = 0x08,
  STATUS_COMP_DUMMY_2ND = 0x10,
  STATUS_COMP_DUMMY_3RD = 0x20,
};

typedef union{
	uint8_t bytes[128];
  struct {
	uint8_t frame_len;
	uint8_t addr;
	uint8_t seq_no;
	uint8_t cmd;
	uint8_t payload_len;
    union {
		uint8_t mode;
		uint8_t key[6];            // CMD_MIFARE_KEY_SET
		uint8_t color_payload[3];  // CMD_EXT_BOARD_LED_RGB
		struct {
		uint8_t eeprom_data[2];     //系统内部设置
		uint8_t mapped_IDm[8];
		uint8_t target_accesscode[10];
		};
      struct {                   // CMD_CARD_SELECT,AUTHORIZE,READ
        uint8_t uid[4];
        uint8_t block_no;
      };
      struct {  // CMD_FELICA_THROUGH
        uint8_t encap_IDm[8];
        uint8_t encap_len;
        uint8_t encap_code;
        union {
          struct {  // CMD_FELICA_THROUGH_POLL
            uint8_t poll_systemCode[2];
            uint8_t poll_requestCode;
            uint8_t poll_timeout;
          };
          struct {  // CMD_FELICA_THROUGH_READ,WRITE,NDA_A4
            uint8_t RW_IDm[8];
            uint8_t numService;
            uint8_t serviceCodeList[2];
            uint8_t numBlock;
            union{
              uint8_t blockList[4][2];  // CMD_FELICA_THROUGH_READ
              struct{
                uint8_t blockList_write[1][2];
                uint8_t blockData[16];    // CMD_FELICA_THROUGH_WRITE
              };

            };
          };
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_request_t;

typedef union{
  uint8_t bytes[128];
  struct {
    uint8_t frame_len;
    uint8_t addr;
    uint8_t seq_no;
    uint8_t cmd;
    uint8_t status;
    uint8_t payload_len;
    union {
      uint8_t mode;
      uint8_t version[1];  // CMD_GET_FW_VERSION,CMD_GET_HW_VERSION,CMD_EXT_BOARD_INFO
      uint8_t block[16];   // CMD_MIFARE_READ
      struct{
        uint8_t eeprom_data[3];
        uint8_t board_vision;
      };
      struct {             // CMD_CARD_DETECT
        uint8_t count;
        uint8_t type;
        uint8_t id_len;
        union {
          uint8_t mifare_uid[4];
          struct {
            uint8_t IDm[8];
            uint8_t PMm[8];
          };
        };
      };
      struct {  // CMD_FELICA_THROUGH
        uint8_t encap_len;
        uint8_t encap_code;
        uint8_t encap_IDm[8];
        union {
          struct {  // FELICA_CMD_POLL
            uint8_t poll_PMm[8];
            uint8_t poll_systemCode[2];
          };
          struct {
            uint8_t RW_status[2];
            uint8_t numBlock;
            uint8_t blockData[4][16];
          };
          //uint8_t RW_status_write[2];
          uint8_t felica_payload[1];
        };
      };
    };
  };
} packet_response_t;

extern packet_request_t req;
extern packet_response_t res;
extern uint8_t sega_reading_status;

uint8_t sega_packet_check(uint8_t* data,uint8_t len);
void Sega_Mode_Loop(uint8_t cmd);
void sega_mifare_pre_read();

#endif /* INC_MODE_SEGA_SERIAL_H_ */
