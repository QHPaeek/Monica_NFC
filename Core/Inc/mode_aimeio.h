/*
 * mode_aimeio.h
 *
 *  Created on: Sep 7, 2025
 *      Author: Qinh
 */

#ifndef INC_MODE_AIMEIO_H_
#define INC_MODE_AIMEIO_H_

#include <stdint.h>

typedef union{
	uint8_t raw_data[21];
	struct{
		uint8_t Start;
		union{
			struct{
				uint8_t Card_Type;
				uint8_t UID[8];
				uint8_t Card_Serial[10];
			};
			struct{
				uint8_t Flash_symbol;
				uint8_t Reader_Type;
				uint8_t Firmware_Vision;
				uint8_t Flash_Data[16];
			};
			struct{
				uint8_t status;
				uint8_t unused[18];
			};
		};
		uint8_t checksum;
	};
}AimeIO_Up_t;

typedef union{
	uint8_t raw_data[11];
	struct{
		uint8_t Start;
		uint8_t Cmd;
		union{
			uint8_t data[8];
			struct{
				uint8_t LED_R;
				uint8_t LED_G;
				uint8_t LED_B;
				uint8_t unused[5];
			};
		};
		uint8_t checksum;
	};
}AimeIO_Down_t;


#endif /* INC_MODE_AIMEIO_H_ */
