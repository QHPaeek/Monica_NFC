/*
 * mode_aimeio.c
 *
 *  Created on: Sep 7, 2025
 *      Author: Qinh
 */
#include "mode_aimeio.h"
#include "flash.h"

#define READER_TYPE_MNC 2

AimeIO_Up_t AimeIO_Up;
AimeIO_Down_t AimeIO_Down;
uint8_t dfu_flag = 0;

enum{
	Aime_IO_Start = 0xff,

	//up cmd
	Aime_IO_Type_None = 0x00,
	Aime_IO_Type_Aime = 0x01,
	Aime_IO_Type_Bana = 0x02,
	Aime_IO_Type_Epass = 0x03,
	Aime_IO_Type_Nesica = 0x04,
	Aime_IO_Type_Felica_AIC = 0x05,
	Aime_IO_Type_Felica = 0x06,
	Aime_IO_Type_T_Union = 0x07,
	Aime_IO_Type_Konami_C = 0x08,
	Aime_IO_Type_Flash = 0xfe,

	//down cmd
	Aime_IO_CMD_Flash_Read = 0x70,
	Aime_IO_CMD_Flash_Write = 0x71,
	Aime_IO_CMD_LED = 0x60,
	Aime_IO_CMD_JUMP2DFU = 0x61,
	//other
	Aime_IO_CheckSum_Error = 0x10,
	Aime_IO_Unknow_CMD = 0x11,
	Aime_IO_OK = 0xfd,
	Aime_IO_NO = 0xfc,
};

uint8_t AimeIO_packet_check(uint8_t* data,uint8_t len){
	if(len != 11){
		return 0;
	}else if(data[0] != Aime_IO_Start){
		return 0;
	}
	uint8_t checksum = 0;
	for(uint8_t i = 0;i<10;i++){
		checksum += data[i];
	}
	if(checksum != data[10]){
		return Aime_IO_CheckSum_Error;
	}
	memcpy(AimeIO_Down.raw_data,data,11);
	return data[1];
}

void AimeIO_process(uint8_t cmd){
	switch(cmd){
		case Aime_IO_CMD_Flash_Read:
			AimeIO_Up.Start = Aime_IO_Start;
			AimeIO_Up.Flash_symbol = Aime_IO_Type_Flash;
			memcpy(AimeIO_Up.Flash_Data,Flash.raw_flash_byte,8);
			AimeIO_Up.checksum = 0;
			for(uint8_t i = 0;i<20;i++){
				AimeIO_Up.checksum += AimeIO_Up.raw_data[i];
			}
			CDC_Transmit(0,AimeIO_Up.raw_data,21);
			break;
		case Aime_IO_CMD_Flash_Write:
			AimeIO_Up.Start = Aime_IO_Start;
			AimeIO_Up.Flash_symbol = Aime_IO_Type_Flash;
			AimeIO_Up.Reader_Type = 2;
			memcpy(Flash.raw_flash_byte,AimeIO_Up.Flash_Data,8);
			AimeIO_Up.checksum = 0;
			for(uint8_t i = 0;i<20;i++){
				AimeIO_Up.checksum += AimeIO_Up.raw_data[i];
			}
			CDC_Transmit(0,AimeIO_Up.raw_data,21);
		case Aime_IO_CMD_LED:
			LED_show(AimeIO_Down.LED_R,AimeIO_Down.LED_G,AimeIO_Down.LED_B);
			break;
		case Aime_IO_CMD_JUMP2DFU:
			dfu_flag = 1;
			uint8_t tmp[21] = {0xff,0xfd,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xfc};
			CDC_Transmit(0,tmp,21);
		default:
			return Aime_IO_Unknow_CMD;
			break;
	}
}
