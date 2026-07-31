/*
 * mode_aimeio.h
 *
 *  Created on: Sep 7, 2025
 *      Author: Qinh
 */

#ifndef INC_MODE_AIMEIO_H_
#define INC_MODE_AIMEIO_H_

#include <stdint.h>

enum{
	AimeIO_Write_Status_OK = 0x00,
	AimeIO_Write_Status_N0_Find_Card = 0x01,
	AimeIO_Write_Status_N0_Same_Card  = 0x02,
	AimeIO_Write_Status_Wrong_KeyA = 0x03,
	AimeIO_Write_Status_Wrong_KeyB  = 0x04,
	AimeIO_Write_Status_Write_Failed  = 0x05,
};

enum{
	AimeIO_Card_None = 0x00,
	AimeIO_Card_Type_Aime = 0x01,
	AimeIO_Card_Type_Bana_New = 0x02,
	AimeIO_Card_Type_Bana_Old = 0x03,
	AimeIO_Card_Type_AIC  = 0x04,
	AimeIO_Card_Type_EPass = 0x05,
	AimeIO_Card_Type_Nesica = 0x06,
	AimeIO_Card_Type_Tunion = 0x07,
	AimeIO_Card_Type_Jubeat = 0x08,
	AimeIO_Card_Type_Unknow_Mifare = 0x09,
	AimeIO_Card_Type_Blank_Mifare = 0x10,
	AimeIO_Card_Type_Unknow_Felica = 0x11,
};

typedef union{
	uint8_t raw_data[64];
	struct{
		uint8_t Start;
		uint8_t cmd;
		union{
			struct{
				uint8_t Card_Type;
				uint8_t Card_RawData[3][16];
			};
			struct{
				uint8_t Reader_Type;
				uint8_t Firmware_Vision;
				uint8_t Flash_Data[16];
			};
			struct{
				uint8_t status;
			};
		};
	};
}AimeIO_Up_t;

typedef union{
	uint8_t raw_data[32];
	struct{
		uint8_t Start;
		uint8_t Cmd;
		union{
			uint8_t Flash_data[16];
			struct{
				uint8_t Write_Sector;
				uint8_t Write_Block;
				uint8_t Card_Key_A[6];
				uint8_t Card_Key_B[6];
				uint8_t Write_Card_data[16];
			};

			struct{
				uint8_t LED_R;
				uint8_t LED_G;
				uint8_t LED_B;
			};
		};
	};
}AimeIO_Down_t;

extern AimeIO_Up_t AimeIO_Up;
extern AimeIO_Down_t AimeIO_Down;
extern uint8_t dfu_flag;
extern uint8_t aimeio_write_flag;

uint8_t AimeIO_process(uint8_t cmd);
uint8_t AimeIO_packet_check(uint8_t* data,uint8_t len);
void AimeIO_Write_Card_Respond(uint8_t status);
void AimeIO_FindCard_Mifare();
void AimeIO_FindCard_AIC();
void AimeIO_FindCard_Flica();
void AimeIO_FindCard_Compatible(uint8_t type,uint8_t* CardSerial);
void AimeIO_No_Card();
void AimeIO_Transmit();

#endif /* INC_MODE_AIMEIO_H_ */
