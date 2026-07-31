/*
 * flash.h
 *
 *  Created on: Jul 18, 2025
 *      Author: Qinh
 */

#ifndef INC_FLASH_H_
#define INC_FLASH_H_

#include <stdint.h>

#define FIRMWARE_VISION 0x5

enum{
	SYSTEM_MODE_SEETING = 1,
	SYSTEM_AIME_SUPPORT = 2,
	SYSTEM_BANA_SUPPORT = 4,
	SYSTEM_NESICA_SUPPORT = 8,
	SYSTEM_EPASS_SUPPORT = 16,
	SYSTEM_TUNION_SUPPORT = 32,
	SYSTEM_JUBEAT_CARD_SUPPORT = 64,
	SYSTEM_IIDX_2P = 128,
	SYSTEM_SEGA_REAL_AIC_SUPPORT =  2,
	SYSTEM_SEGA_OTHER_SUPPORT =  4,
	SYSTEM_SPICE_OTHER_SUPPORT =  16,
};

typedef union{
	uint32_t raw_flash_word[4];
	uint8_t raw_flash_byte[16];
	struct{
		uint8_t firmversion_1;
			//firmversion low byte
		uint8_t firmversion_2;
			//firmversion high byte
		uint8_t system_setting;
			//UART baudrate. 0x0: 115200; 0x1: 38400 ; 0x2: 9600
		uint8_t gobal_LED_setting;
			//LED Brightness
		uint8_t sega_setting;
			//BIT0:tn32 emulator enable. 0:15693(tn32 disable)
			//bit1:felica amusemunt card (AIC) transform to real serial in sega mode. 0:disable 1:enable
			//bit2:Other felica card and 15693 card enable. 0:disable
			//bit3:nesica(mifare ultra light)support enable. 0:disable
			//bit4:e-amusement-pass(iso15693)support enable. 0:disable
			//bit5:T-union(交通联合,iso14443-a,APDU)support enable. 0:disable
			//bit6:Jubeat China(mifare)support enable. 0:disable
		uint8_t spice_setting;
			//BIT0:2P mode enable. 0:1P
			//bit1:clasic aime(mifare)support enable. 0:disable
			//bit2:banapass(mifare)support enable. 0:disable
			//bit3:nesica(mifare ultra light)support enable. 0:disable
			//bit4:Other felica card and 15693 card enable. 0:disable
			//bit5:T-union(交通联合,iso14443-a,APDU)support enable. 0:disable
			//bit6:Jubeat China(mifare)support enable. 0:disable
			//bit7:IIDX Player.0:1P Enable:2P
		uint8_t namco_setting;
			//BIT0:todo
			//bit1:clasic aime(mifare)support enable. 0:disable
			//bit2:banapass(mifare)support enable. 0:disable
			//bit3:nesica(mifare ultra light)support enable. 0:disable
			//bit4:unused
			//bit5:T-union(交通联合,iso14443-b)support enable. 0:disable
			//bit6:Jubeat China(mifare)support enable. 0:disable
	};
}FlashData;
extern FlashData Flash;
extern const uint8_t default_setting[7];

void flash_write(uint32_t data[4]);
void flash_read(uint8_t* data);



#endif /* INC_FLASH_H_ */
