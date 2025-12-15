/*
 * Card_Reader.h
 *
 *  Created on: Jul 19, 2025
 *      Author: Qinh
 */

#ifndef INC_CARD_READER_H_
#define INC_CARD_READER_H_

#include "rfal_utils.h"
#include "rfal_nfca.h"
#include "rfal_nfcf.h"
#include "rfal_nfcv.h"
#include "rfal_nfcDep.h"
#include "rfal_isoDep.h"
#include "st_errno.h"
#include "utils.h"
#include "mcc.h"

enum{
	Card_None = 0x00,
	Card_Type_Mifare_Classic = 0x01,
	Card_Type_Mifare_UltraLight = 0x02,
	Card_Type_Felica_AIC = 0x03,
	Card_Type_Felica_Suica = 0x04,
	Card_Type_ISO15693 = 0x05,
	Card_Type_ISO14443A_T_Union = 0x06,
	Card_Type_Felica_Unknow= 0xfe,
	Card_Type_ISO14443A_Unknow = 0xff,
};

enum{
	Operation_idle,
	Operation_detected,
	Operation_busy,
};

enum{
	Auth_KeyA_Right = 0x01,
	Auth_KeyB_Right = 0x02,
	Auth_ALL_Right = 0x03,
	Auth_ALL_Failed = 0b10000000,
};

typedef struct{
	uint8_t type;
	uint8_t operation;
	union{
		uint8_t data[128];
		struct{					//Classic Aime & BanaPass
			uint8_t mifare_auth_status;
			union{
				uint8_t mifare_data[4][16];
				struct{
					uint8_t iso14443_uid4[4];
					uint8_t manufacturer[12];
					uint8_t block1[16];
					uint8_t block2[16];
					uint8_t mifare_right_key_a[6];
					uint8_t access_condition[4];
					uint8_t mifare_right_key_b[6];
				};
			};
		};
		struct{					//Classic Nesica
			uint8_t iso14443_uid7[7];
			uint8_t mifare_ul_read_status;
			union{
				uint8_t mifare_ul_data[4][4];
				uint8_t nesica_serial[16];
			};
		};
		struct{					//Amusement AIC
			uint8_t felica_IDm[8];
			uint8_t felica_PMm[8];
			uint8_t felica_systemcode[2];
		};
		struct{					//Classic e_amusement_pass
			uint8_t iso15693_uid[8];
		};
		struct{					//T-Union
			uint8_t t_union_uid[4];
			uint8_t t_union_balance[2];
			uint8_t t_union_start_date[4];
			uint8_t t_union_end_date[4];
			uint8_t t_union_serial[10];
		};
	};
	uint8_t operation_tmp[128];
}CardData;

extern CardData Card;
extern const uint8_t AimeKey[6];
extern const uint8_t BanaKey_A[6];
extern const uint8_t BanaKey_B[6];

void Card_Poll();
ReturnCode nfcfReadBlock(uint8_t *idm,uint16_t *serviceList ,uint8_t num_block,uint8_t *blockList ,uint8_t blockdata[4][16]);
ReturnCode nfcfWriteSingleBlock(uint8_t *idm, uint8_t num_service,uint16_t *serviceList ,uint16_t *blockList,uint8_t *blockdata);
ReturnCode nfcvReadBlock(rfalNfcvListenDevice *device, uint8_t blockNum, uint8_t *rxBuf, uint16_t bufSize, uint8_t *uid);
ReturnCode nfcvWriteBlock(rfalNfcvListenDevice *device, uint8_t blockNum, uint8_t *wrData, uint16_t dataLen, uint8_t *uid);
ReturnCode mifareAuthenticate(uint8_t keyType, uint8_t sector, uint8_t* uid, uint32_t uidLen, uint8_t* key);
ReturnCode mifareReadBlock(uint8_t sector, uint8_t block, uint8_t* buffer, uint16_t bufSize);
ReturnCode ActivateP2P( uint8_t* nfcid, uint8_t nfidLen, bool isActive, rfalNfcDepDevice *nfcDepDev );
ReturnCode IsoDepBlockingTxRx( rfalIsoDepDevice *isoDepDev, const uint8_t *txBuf, uint16_t txBufSize, uint8_t *rxBuf, uint16_t rxBufSize, uint16_t *rxActLen );
uint8_t APDU_check_response(uint8_t *data,uint16_t len);
bool T_Union_Read();
static uint32_t rng_get32(void);

#endif /* INC_CARD_READER_H_ */
