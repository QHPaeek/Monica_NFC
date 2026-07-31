/*
 * mode_aimeio.c
 *
 *  Created on: Sep 7, 2025
 *      Author: Qinh
 */
#include "mode_aimeio.h"
#include "flash.h"
#include "Card_Reader.h"

#define READER_TYPE_MNC 2

AimeIO_Up_t AimeIO_Up;
AimeIO_Down_t AimeIO_Down;
uint8_t dfu_flag = 0;
uint8_t aimeio_write_flag = 0;

enum{
	Aime_IO_Start = 0xff,

	Aime_IO_CMD_Find_Card = 0x01,
	Aime_IO_CMD_Write_Card = 0x02,
	Aime_IO_CMD_Flash_Read = 0x70,
	Aime_IO_CMD_Flash_Write = 0x71,
	Aime_IO_CMD_LED = 0x60,
	Aime_IO_CMD_JUMP2DFU = 0x61,
	//other
	Aime_IO_Unknow_CMD = 0x11,
	Aime_IO_OK = 0xfd,
	Aime_IO_NO = 0xfc,
};

uint8_t AimeIO_packet_check(uint8_t* data,uint8_t len){
	if(data[0] != Aime_IO_Start){
		return 0;
	}

	memcpy(AimeIO_Down.raw_data,data,32);
	return data[1];
}

uint8_t AimeIO_process(uint8_t cmd){
	memset(AimeIO_Up.raw_data,0,64);
	switch(cmd){
		case Aime_IO_CMD_Flash_Read:
			AimeIO_Up.Start = Aime_IO_Start;
			AimeIO_Up.cmd = Aime_IO_CMD_Flash_Read;
			AimeIO_Up.Reader_Type = 0xa1;
			AimeIO_Up.Firmware_Vision = FIRMWARE_VISION;
			memcpy(AimeIO_Up.Flash_Data,Flash.raw_flash_byte,16);
			AimeIO_Transmit();
			break;
		case Aime_IO_CMD_Flash_Write:
			AimeIO_Up.Start = Aime_IO_Start;
			AimeIO_Up.cmd = Aime_IO_CMD_Flash_Write;
			AimeIO_Up.Reader_Type = 0xa1;
			memcpy(Flash.raw_flash_byte,AimeIO_Down.Flash_data,7);
			flash_write(Flash.raw_flash_word);
			AimeIO_Transmit();
			break;
		case Aime_IO_CMD_LED:
			LED_show(AimeIO_Down.LED_R,AimeIO_Down.LED_G,AimeIO_Down.LED_B);
			break;
		case Aime_IO_CMD_JUMP2DFU:
			dfu_flag = 1;
			AimeIO_Up.Start = Aime_IO_Start;
			AimeIO_Up.cmd = Aime_IO_CMD_JUMP2DFU;
			AimeIO_Transmit();
			break;
		case Aime_IO_CMD_Write_Card:{
			aimeio_write_flag = 1;
			break;
		}
		default:
			return 0;
			break;
	}
	return 1;
}

void AimeIO_FindCard_Mifare(){
	memset(AimeIO_Up.raw_data,0,64);
	AimeIO_Up.Start = Aime_IO_Start;
	AimeIO_Up.cmd = Aime_IO_CMD_Find_Card;
	if(Card.mifare_auth_status != Auth_ALL_Right)return;
	AimeIO_Up.Card_Type = AimeIO_Card_Type_Unknow_Mifare;
	if(memcmp(Card.mifare_right_key_a,AimeKey,6) == 0){
		AimeIO_Up.Card_Type = AimeIO_Card_Type_Aime;
	}else if(memcmp(Card.mifare_right_key_a,BanaKey_A,6) == 0){
		if(memcmp(Card.mifare_right_key_b,BanaKey_B,6) == 0){
			AimeIO_Up.Card_Type = AimeIO_Card_Type_Bana_Old;
		}else if(memcmp(Card.mifare_right_key_b,AimeKey,6) == 0){
			AimeIO_Up.Card_Type = AimeIO_Card_Type_Bana_New;
		}
	}else if(memcmp(Card.mifare_right_key_a,JubeatKey,6) == 0){
		AimeIO_Up.Card_Type = AimeIO_Card_Type_Jubeat;
	}else if(memcmp(Card.mifare_right_key_a,EmptyKey,6) == 0){
		if(memcmp(Card.mifare_right_key_b,EmptyKey,6) == 0){
			AimeIO_Up.Card_Type = AimeIO_Card_Type_Blank_Mifare;
		}
	}
	if(AimeIO_Up.Card_Type == AimeIO_Card_Type_Unknow_Mifare)return;
	for(uint8_t i = 0;i<3;i++){
		memcpy(AimeIO_Up.Card_RawData[i],Card.mifare_data[i],16);
	}
	AimeIO_Transmit();
}

void AimeIO_FindCard_AIC(){
	memset(AimeIO_Up.raw_data,0,64);
	AimeIO_Up.Start = Aime_IO_Start;
	AimeIO_Up.cmd = Aime_IO_CMD_Find_Card;
	AimeIO_Up.Card_Type = AimeIO_Card_Type_AIC;
	memcpy(AimeIO_Up.Card_RawData[0],Card.felica_IDm,8);
	memcpy(AimeIO_Up.Card_RawData[0]+8,Card.felica_PMm,8);
	memcpy(AimeIO_Up.Card_RawData[1],Card.block_8000,16);
	AimeIO_Transmit();
}

void AimeIO_FindCard_Flica(){
	memset(AimeIO_Up.raw_data,0,64);
	AimeIO_Up.Start = Aime_IO_Start;
	AimeIO_Up.cmd = Aime_IO_CMD_Find_Card;
	AimeIO_Up.Card_Type = AimeIO_Card_Type_Unknow_Felica;
	memcpy(AimeIO_Up.Card_RawData[0],Card.felica_IDm,8);
	memcpy(AimeIO_Up.Card_RawData[0]+8,Card.felica_PMm,8);
	AimeIO_Transmit();
}

void AimeIO_FindCard_Compatible(uint8_t type,uint8_t* CardSerial){
	memset(AimeIO_Up.raw_data,0,64);
	AimeIO_Up.Start = Aime_IO_Start;
	AimeIO_Up.cmd = Aime_IO_CMD_Find_Card;
	switch(type){
		case AimeIO_Card_Type_EPass:{
			AimeIO_Up.Card_Type = AimeIO_Card_Type_EPass;
			memcpy(AimeIO_Up.Card_RawData[0],Card.icode_uid,8);
			break;
		}
		case AimeIO_Card_Type_Nesica:{
			AimeIO_Up.Card_Type = AimeIO_Card_Type_Nesica;
			memcpy(AimeIO_Up.Card_RawData[0],Card.iso14443_uid7,7);
			break;
		}
		case AimeIO_Card_Type_Tunion:{
			AimeIO_Up.Card_Type = AimeIO_Card_Type_Tunion;
			memcpy(AimeIO_Up.Card_RawData[0],Card.iso14443_uid4,4);
			break;
		}
	}
	memcpy(AimeIO_Up.Card_RawData[1],CardSerial,10);
	AimeIO_Transmit();
}

void AimeIO_No_Card(){
	memset(AimeIO_Up.raw_data,0,64);
	AimeIO_Up.Start = Aime_IO_Start;
	AimeIO_Up.cmd = Aime_IO_CMD_Find_Card;
	AimeIO_Up.Card_Type = AimeIO_Card_None;
	AimeIO_Transmit();
}

void AimeIO_Write_Card_Respond(uint8_t status){
	memset(AimeIO_Up.raw_data,0,64);
	AimeIO_Up.Start = Aime_IO_Start;
	AimeIO_Up.cmd = Aime_IO_CMD_Write_Card;
	AimeIO_Up.status = status;
	AimeIO_Transmit();
}

void AimeIO_Transmit(){
    memmove(AimeIO_Up.raw_data + 1, AimeIO_Up.raw_data, 63);
    AimeIO_Up.raw_data[0] = 0x04;
	Reader_HID_SendReport(AimeIO_Up.raw_data,64);
}
