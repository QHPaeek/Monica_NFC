/*
 * flash.h
 *
 *  Created on: Jul 18, 2025
 *      Author: Qinh
 */

#ifndef INC_FLASH_H_
#define INC_FLASH_H_

#include <stdint.h>

#define FIRMWARE_VISION 4

enum{
	SYSTEM_MODE_SEETING = 1,
	SYSTEM_AIME_SUPPORT = 2,
	SYSTEM_BANA_SUPPORT = 4,
	SYSTEM_NESICA_SUPPORT = 8,
	SYSTEM_EPASS_SUPPORT = 16,
	SYSTEM_TUNION_SUPPORT = 32,
	SYSTEM_JUBEAT_CARD_SUPPORT = 64,
	SYSTEM_IIDX_2P = 128,
};

typedef union{
	uint32_t raw_flash_word[4];
	uint8_t raw_flash_byte[16];
	struct{
		uint8_t system_setting;
			//bit0~4 : firmversion
			//bit5~8 : UART baudrate. 0x0: 115200; 0x1: 38400 ; 0x2: 9600
		uint8_t gobal_LED_setting;
			//LED Brightness
		uint8_t sega_setting;
			//BIT0:tn32 emulator enable. Default:15693(tn32 disable)
			//bit1:unused
			//bit2:unused
			//bit3:nesica(mifare ultra light)support enable. Default:disable
			//bit4:e-amusement-pass(iso15693)support enable. Default:disable
			//bit5:T-union(交通联合,iso14443-a,APDU)support enable. Default:disable
			//bit6:Jubeat China(mifare)support enable. Default:disable
		uint8_t spice_setting;
			//BIT0:IIDX mode enable. Default:disable
			//bit1:clasic aime(mifare)support enable. Default:disable
			//bit2:banapass(mifare)support enable. Default:disable
			//bit3:nesica(mifare ultra light)support enable. Default:disable
			//bit4:unused
			//bit5:T-union(交通联合,iso14443-a,APDU)support enable. enable. Default:disable
			//bit6:Jubeat China(mifare)support enable. Default:disable
			//bit7:IIDX Player.Default:1P Enable:2P
		uint8_t namco_setting;
			//BIT0:todo
			//bit1:clasic aime(mifare)support enable. Default:disable
			//bit2:banapass(mifare)support enable. Default:disable
			//bit3:nesica(mifare ultra light)support enable. Default:disable
			//bit4:unused
			//bit5:T-union(交通联合,iso14443-b)support enable. enable. Default:disable
			//bit6:Jubeat China(mifare)support enable. Default:disable
	};
}FlashData;
extern FlashData Flash;
extern const uint8_t default_setting[5];

void flash_write(uint32_t data[4]);
void flash_read(uint8_t* data);




#endif /* INC_FLASH_H_ */
