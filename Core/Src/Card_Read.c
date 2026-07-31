/*
 * Card_Read.c
 *
 *  Created on: Jul 19, 2025
 *      Author: Qinh
 */
#include "Card_Reader.h"
#include "mode_manager.h"
#include "st_errno.h"
#include "rfal_utils.h"
#include "mode_sega_serial.h"
#include "mode_spice_api.h"
#include "LED.h"
#include "mode_aimeio.h"

#define RFAL_NFCA_SEL_RES_CONF_MIFARE    	0x08 /* SEL_RES (SAK) Mifare configuration */
#define DEMO_NFCV_BLOCK_LEN           		4     /*!< NFCV Block len                      */
#define MIFARE_BLOCK_SIZE            		16
#define MIFARE_CRC_LEN                		2
#define MIFARE_READ_TIMEOUT           		50
#define DEMO_BUF_LEN                  		128

extern USBD_HandleTypeDef hUsbDevice;

rfalIsoDepApduTxRxParam isoDepTxRx;
static union {
    rfalIsoDepDevice  isoDepDev;                                         /* ISO-DEP Device details                          */
    rfalNfcDepDevice  nfcDepDev;                                         /* NFC-DEP Device details                          */
}gDevProto;

const uint8_t ndefSelectApp[] = { 0x00, 0xA4, 0x04, 0x00, 0x07, 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01, 0x00 };
const uint8_t ccSelectFile[] = { 0x00, 0xA4, 0x00, 0x0C, 0x02, 0xE1, 0x03};
const uint8_t readBynary[] = { 0x00, 0xB0, 0x00, 0x00, 0x0F };

CardData Card;

const uint8_t AimeKey[6]   = {0x57, 0x43, 0x43, 0x46, 0x76, 0x32};
const uint8_t BanaKey_A[6] = {0x60, 0x90, 0xD0, 0x06, 0x32, 0xF5};
const uint8_t BanaKey_B[6] = {0x01, 0x97, 0x61, 0xAA, 0x80, 0x82};
const uint8_t JubeatKey[6] = {0xf4, 0x20, 0xd0, 0x09, 0x40, 0xa6};
const uint8_t EmptyKey[6]  = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

uint8_t auth_flag = 0;

uint16_t Get_Card_ATQA(rfalNfcaSensRes atqa){
	return ((atqa.platformInfo << 8) | atqa.anticollisionInfo);
}

void mifare_pre_read(){
	mccInitialize();
	Card.mifare_auth_status = 0;
	if(!auth_flag){
		auth_flag ++;
		if(mifareAuthenticate(MCC_AUTH_KEY_A, 0, Card.iso14443_uid4, 4, (uint8_t *)AimeKey) != RFAL_ERR_NONE){
		}else{
			memcpy(Card.mifare_right_key_a,AimeKey,6);
			Card.mifare_auth_status |= Auth_KeyA_Right;
		}
		if(mifareAuthenticate(MCC_AUTH_KEY_B, 0, Card.iso14443_uid4, 4, (uint8_t *)AimeKey) != RFAL_ERR_NONE){
		}else{
			memcpy(Card.mifare_right_key_b,AimeKey,6);
			Card.mifare_auth_status |= Auth_KeyB_Right;
		}

		goto read2;
	}else if(auth_flag == 1){
		auth_flag ++;
		if(mifareAuthenticate(MCC_AUTH_KEY_A, 0, Card.iso14443_uid4, 4, (uint8_t *)BanaKey_A) != RFAL_ERR_NONE){

		}else{
			memcpy(Card.mifare_right_key_a,BanaKey_A,6);
			Card.mifare_auth_status |= Auth_KeyA_Right;
		}
		if(mifareAuthenticate(MCC_AUTH_KEY_B, 0, Card.iso14443_uid4, 4, (uint8_t *)BanaKey_B) != RFAL_ERR_NONE){

		}else{
			memcpy(Card.mifare_right_key_b,BanaKey_B,6);
			Card.mifare_auth_status |= Auth_KeyB_Right;
		}
		goto read1;
	}else if(auth_flag == 2){
		auth_flag ++;
		if(mifareAuthenticate(MCC_AUTH_KEY_A, 0, Card.iso14443_uid4, 4, (uint8_t *)BanaKey_A) != RFAL_ERR_NONE){

		}else{
			memcpy(Card.mifare_right_key_a,BanaKey_A,6);
			Card.mifare_auth_status |= Auth_KeyA_Right;
		}
		if(mifareAuthenticate(MCC_AUTH_KEY_B, 0, Card.iso14443_uid4, 4, (uint8_t *)AimeKey) != RFAL_ERR_NONE){

		}else{
			memcpy(Card.mifare_right_key_b,AimeKey,6);
			Card.mifare_auth_status |= Auth_KeyB_Right;

		}
		goto read1;
	}else if(auth_flag == 3){
		auth_flag ++;
		if(mifareAuthenticate(MCC_AUTH_KEY_A, 0, Card.iso14443_uid4, 4, (uint8_t *)JubeatKey) != RFAL_ERR_NONE){
//			Card.mifare_auth_status = Auth_ALL_Failed;
//			goto end;
		}else{
			memcpy(Card.mifare_right_key_a,JubeatKey,6);
			memset(Card.mifare_right_key_b,0xff,6);
			Card.mifare_auth_status |= Auth_ALL_Right;
		}
		return;
	}else{
		if(mifareAuthenticate(MCC_AUTH_KEY_A, 0, Card.iso14443_uid4, 4, (uint8_t *)EmptyKey) == RFAL_ERR_NONE){
			memcpy(Card.mifare_right_key_a,EmptyKey,6);
			Card.mifare_auth_status |= Auth_KeyA_Right;
			if(mifareAuthenticate(MCC_AUTH_KEY_B, 0, Card.iso14443_uid4, 4, (uint8_t *)EmptyKey) == RFAL_ERR_NONE){
				memcpy(Card.mifare_right_key_b,EmptyKey,6);
				Card.mifare_auth_status |= Auth_KeyB_Right;
			}
		}
		auth_flag = 0;
		return;
	}
read1:
	if(Card.mifare_auth_status == Auth_ALL_Right){
		uint8_t tmp[18];
		for(uint8_t i = 1;i<3;i++){
			mifareReadBlock(0, i, tmp, 18);
			memcpy(Card.mifare_data[i], tmp, 16);
		}
		uint8_t zero[10] = {0,0,0,0,0,0,0,0,0,0};
		if((auth_flag != 0) && (memcmp(Card.mifare_data[2] + 6,zero,10) == 0)){
			//All Zero Card mean read failed
			Card.mifare_auth_status = Auth_ALL_Failed;
		}
	}
	else if(Card.mifare_auth_status == 0){
		Card.mifare_auth_status = Auth_ALL_Failed;
	}
	mccDeinitialise(true);
	return;
read2:
	if(Card.mifare_auth_status == Auth_ALL_Right){
		uint8_t tmp[18];
		for(uint8_t i = 2;i>0;i--){
			mifareReadBlock(0, i, tmp, 18);
			memcpy(Card.mifare_data[i], tmp, 16);
		}
		uint8_t zero[10] = {0,0,0,0,0,0,0,0,0,0};
		if(memcmp(Card.mifare_data[2] + 6,zero,10) == 0){
			//All Zero Card mean read failed
			Card.mifare_auth_status = Auth_ALL_Failed;
		}
	}
	else if(Card.mifare_auth_status == 0){
		Card.mifare_auth_status = Auth_ALL_Failed;
	}
	mccDeinitialise(true);
	return;
}

void mifare_ul_read(){
	Card.mifare_ul_read_status = 0;
	uint8_t rcv_len = 0;
	if (mifareUlReadNBytes(5, Card.nesica_serial, 16, &rcv_len) != RFAL_ERR_NONE) {
	  return;
	}
	for(uint8_t i = 0;i<16;i++){
		if(!((Card.nesica_serial[i] > 0x2f) && (Card.nesica_serial[i] < 0x3a ))){
		  return;
		}
	}
	Card.mifare_ul_read_status = 1;
}

uint8_t mifare_pre_write(uint8_t retry){
	uint8_t _retry;
	_retry = retry;
	opera:
	ReturnCode           	err;
	rfalNfcaListenDevice 	nfcaDev;
	uint8_t              	devCnt = 0;
	rfalNfcaPollerInitialize();
	rfalFieldOnAndStartGT();
	err = rfalNfcaPollerFullCollisionResolution(RFAL_COMPLIANCE_MODE_NFC,1,&nfcaDev,&devCnt);
	ReturnCode_proc(err);
	if( (err == ERR_NONE) && (devCnt > 0) ){
		if((nfcaDev.nfcId1Len != 4) || (nfcaDev.isSleep) || (nfcaDev.type != RFAL_NFCA_T2T) || ((Get_Card_ATQA(nfcaDev.sensRes) != 0x04) && (Get_Card_ATQA(nfcaDev.sensRes) != 0x02))){
			if(_retry){
				_retry --;
				rfalFieldOff();
				goto opera;
			}else{
				memset(Card.data,0,128);
				rfalFieldOff();
				return 1;
			}
		}
		mccInitialize();
		if(mifareAuthenticate(MCC_AUTH_KEY_A, AimeIO_Down.Write_Sector, nfcaDev.nfcId1, 4, AimeIO_Down.Card_Key_A) != RFAL_ERR_NONE){
			memset(Card.data,0,128);
			rfalFieldOff();
			return 3;
		}
		if(mifareAuthenticate(MCC_AUTH_KEY_B, AimeIO_Down.Write_Sector, nfcaDev.nfcId1, 4, AimeIO_Down.Card_Key_B) != RFAL_ERR_NONE){
			memset(Card.data,0,128);
			rfalFieldOff();
			return 4;
		}
		if(mifareWriteBlock(AimeIO_Down.Write_Sector, AimeIO_Down.Write_Block, AimeIO_Down.Write_Card_data) != RFAL_ERR_NONE){
			memset(Card.data,0,128);
			rfalFieldOff();
			return 5;
		}
		memset(Card.data,0,128);
		rfalFieldOff();
		return 0;
	}else{
		memset(Card.data,0,128);
		rfalFieldOff();
		return 1;
	}
	memset(Card.data,0,128);
	rfalFieldOff();
	return 0;
}

void Card_Poll()
{

    ReturnCode           	err;
    rfalNfcaListenDevice 	nfcaDev;
    rfalNfcfListenDevice  	nfcfDev;
    rfalNfcvListenDevice  	nfcvDev;
    uint8_t              	devCnt = 0;

//    if((Reader.Current_Mode == MODE_SEGA_SERIAL)  && sega_reading_status){
//    	return;
//    }

    if(aimeio_write_flag){
    	aimeio_write_flag = 0;
    	AimeIO_Write_Card_Respond(mifare_pre_write(3));
    	return;
    }

	/*******************************************************************************/
	/* Felica/NFC_F poll first to prevent dual-mode card from being preempted      */
	/*******************************************************************************/
	rfalNfcfPollerInitialize( RFAL_BR_212 );
	rfalFieldOnAndStartGT();
	err = rfalNfcfPollerCollisionResolution( RFAL_COMPLIANCE_MODE_NFC, 1, &nfcfDev, &devCnt );
	ReturnCode_proc(err);
	if( (err == ERR_NONE) && (devCnt > 0) )
	{
		platformLedOn(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
		if((Card.type != Card_Type_Felica_AIC) && (Card.type != Card_Type_Felica_Other)){
			memset(Card.data,0,128);
		}else if(memcmp(Card.felica_IDm,nfcfDev.sensfRes.NFCID2, RFAL_NFCF_NFCID2_LEN) == 0){
			//same card,skip
			rfalFieldOff();
			return;
		}
		Card.operation = Operation_detected;
		memcpy(Card.felica_IDm,nfcfDev.sensfRes.NFCID2, 8);
		memcpy(Card.felica_PMm,nfcfDev.sensfRes.PAD0, 8);
		memcpy(Card.felica_systemcode,nfcfDev.sensfRes.RD,2);
		uint8_t default_pmm[8] = {0x00,0xF1,0x00,0x00,0x00,0x01,0x43,0x00};
		if(memcmp(Card.felica_PMm,default_pmm, 8) == 0){
			Card.type = Card_Type_Felica_AIC;
			err = nfcfReadBlock_default();
        	switch(Reader.Current_Mode){
				case MODE_AIME_IO:
					AimeIO_FindCard_AIC();
					break;
        		case MODE_SPICE_API:
        			spice_felice_process();
        			break;
        		case MODE_IDLE:
        			LED_show(0,0,255);
        		default:
        			LED_show(0,0,255);
					spice_cardio_send(Card.felica_IDm);
					break;
        	}
		}else{
			Card.type = Card_Type_Felica_Other;
        	switch(Reader.Current_Mode){
				case MODE_AIME_IO:
					AimeIO_FindCard_Flica();
					break;
        		case MODE_SPICE_API:
        			if(Flash.spice_setting & SYSTEM_SPICE_OTHER_SUPPORT){
        				spice_felice_process();
        			}
        			break;
        		case MODE_IDLE:
        			LED_show(0,0,255);
        		default:
        			if(Flash.spice_setting & SYSTEM_SPICE_OTHER_SUPPORT){
						LED_show(0,0,255);
						spice_cardio_send(Card.felica_IDm);
					}
					break;
        	}
		}
		rfalFieldOff();
		return;
	}
//	rfalFieldOff();

    /*******************************************************************************/
    /* NFC-A Technology Detection                                                  */
    /*******************************************************************************/

    rfalNfcaPollerInitialize();
//    rfalFieldOnAndStartGT();

    err = rfalNfcaPollerFullCollisionResolution(RFAL_COMPLIANCE_MODE_NFC,1,&nfcaDev,&devCnt);
    ReturnCode_proc(err);
    if( (err == ERR_NONE) && (devCnt > 0) )
    {
    	platformLedOn(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
    	if(nfcaDev.nfcId1Len == 4){
    		if((Card.type == Card_Type_Mifare_Classic) && (memcmp(Card.iso14443_uid4,nfcaDev.nfcId1,4) == 0) && (Card.mifare_auth_status == Auth_ALL_Right)){
    				Card.operation = Operation_detected;
    				rfalFieldOff();
    				return;
    		}else if((Card.type == Card_Type_ISO14443A_T_Union) && (memcmp(Card.t_union_uid,nfcaDev.nfcId1,4) == 0)){
    			Card.operation = Operation_detected;
				rfalFieldOff();
				return;
    		}
    	}else if(nfcaDev.nfcId1Len == 7){
    		if((Card.type == Card_Type_Mifare_UltraLight) && (memcmp(Card.iso14443_uid7,nfcaDev.nfcId1,7) == 0) && (Card.mifare_ul_read_status == 1)){
				Card.operation = Operation_detected;
				rfalFieldOff();
				return;
			}
    	}else{
    		rfalFieldOff();
    		return;
    	}
    	Card.operation = Operation_detected;
        if( nfcaDev.isSleep )
        {
            err = rfalNfcaPollerCheckPresence( RFAL_14443A_SHORTFRAME_CMD_WUPA, &nfcaDev.sensRes );
            if( err == ERR_NONE )
            {
            	err = rfalNfcaPollerSelect( nfcaDev.nfcId1, nfcaDev.nfcId1Len, &nfcaDev.selRes );
            }
        }
		if (nfcaDev.type == RFAL_NFCA_T2T)
		{
			switch(Get_Card_ATQA(nfcaDev.sensRes)){
				case 0x0044:
					if(nfcaDev.nfcId1Len == 7){
						if(Card.type != Card_Type_Mifare_UltraLight){
							memset(Card.data,0,128);
							Card.type = Card_Type_Mifare_UltraLight;
						}
						memcpy(Card.iso14443_uid7,nfcaDev.nfcId1,7);
						mifare_ul_read();
			        	switch(Reader.Current_Mode){
							case MODE_AIME_IO:
								uint8_t tmp[10];
								ascii_to_accesscode(Card.nesica_serial ,tmp);
								AimeIO_FindCard_Compatible(6,tmp);
								break;
			        		case MODE_SPICE_API:
			        			spice_iso14443_process();
								break;
			        		case MODE_IDLE:
			        		default:
			        			spice_cardio_send_14443(nfcaDev.nfcId1);
								break;
			        	}
					}
					break;
		        case 0x0004:
		        case 0x0002:
		        	if(Card.type != Card_Type_Mifare_Classic){
		        		memset(Card.data,0,128);
		        	}
		        	memcpy(Card.iso14443_uid4,nfcaDev.nfcId1,nfcaDev.nfcId1Len);
		        	mifare_pre_read();
		        	Card.type = Card_Type_Mifare_Classic;
		        	switch(Reader.Current_Mode){
						case MODE_AIME_IO:
							AimeIO_FindCard_Mifare();
							break;
		        		case MODE_IDLE:
		        			spice_cardio_send_14443(nfcaDev.nfcId1);
							break;
		        		case MODE_SPICE_API:
		        			spice_iso14443_process();
		        			break;
		        		default:
		        			spice_cardio_send_14443(nfcaDev.nfcId1);
		        			break;
		        	}
		            break;
			}
		}
		else if( nfcaDev.type == RFAL_NFCA_T1T )
		{
			memcpy(Card.iso14443_uid4,nfcaDev.nfcId1,nfcaDev.nfcId1Len);
	    	Card.type = Card_Type_ISO14443A_Unknow;
		}
		else if( (nfcaDev.type == RFAL_NFCA_NFCDEP) || (nfcaDev.type == RFAL_NFCA_T4T_NFCDEP))
		{
			memcpy(Card.iso14443_uid4,nfcaDev.nfcId1,nfcaDev.nfcId1Len);
			Card.type = Card_Type_ISO14443A_Unknow;
	     }
	     else if (nfcaDev.type == RFAL_NFCA_T4T)
	     {
	    	  if(T_Union_Read()){
	    		  Card.type = Card_Type_ISO14443A_T_Union;
	    		  memcpy(Card.t_union_uid,nfcaDev.nfcId1,4);
	    		  switch(Reader.Current_Mode){
					case MODE_AIME_IO:
						AimeIO_FindCard_Compatible(7,Card.t_union_serial);
						break;
					case MODE_IDLE:
						spice_cardio_send_14443(nfcaDev.nfcId1);
						break;
					case MODE_SPICE_API:
						spice_iso14443_process();
						break;
					default:
						spice_cardio_send_14443(nfcaDev.nfcId1);
						break;
	    		  }
	    	  }else{
	    		  memcpy(Card.iso14443_uid4,nfcaDev.nfcId1,nfcaDev.nfcId1Len);
	    		  Card.type = Card_Type_ISO14443A_Unknow;
	    	  }
	     }
		 rfalFieldOff();
		 return;
    }
//	rfalFieldOff();
	/*******************************************************************************/
	/* ISO15693/NFC_V_PASSIVE_POLL_MODE                                            */
	/*******************************************************************************/

	rfalNfcvPollerInitialize();
//	rfalFieldOnAndStartGT();

	err = rfalNfcvPollerCollisionResolution(1,1, &nfcvDev, &devCnt);
	ReturnCode_proc(err);
	if( (err == ERR_NONE) && (devCnt > 0) )
	{
        uint8_t devUID[RFAL_NFCV_UID_LEN];
        platformLedOn(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
        memcpy( devUID, nfcvDev.InvRes.UID, RFAL_NFCV_UID_LEN );
        REVERSE_BYTES( devUID, RFAL_NFCV_UID_LEN );
		if(Card.type != Card_Type_ICODE){
			memset(Card.data,0,128);
		}else if(memcmp(Card.icode_uid,devUID, RFAL_NFCV_UID_LEN) == 0){
			//same card,skip
			rfalFieldOff();
			return;
		}
		Card.type = Card_Type_ICODE;
		Card.operation = Operation_detected;
		memcpy(Card.icode_uid,devUID, RFAL_NFCV_UID_LEN);
    	switch(Reader.Current_Mode){
    		case MODE_SPICE_API:
    			spice_icode_process();
    			break;
    		case MODE_AIME_IO:
    			uint8_t tmp[10];
    			hex_to_accesscode(Card.icode_uid ,tmp);
    			AimeIO_FindCard_Compatible(5,tmp);
    			break;
    		case MODE_IDLE:
    			spice_cardio_send(Card.icode_uid);
    			LED_show(0,128,128);
    			break;
    	}
		rfalFieldOff();
		return;
	}

no_card:
	//No Card Deteced
	//platformLog("no card\r\n");
	platformLedOff(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
	platformLedOff(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
	platformLedOff(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
	platformLedOff(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
	platformLedOff(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
	platformLedOff(PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN);
	switch(Reader.Current_Mode){
		case MODE_IDLE:
			LED_show(0,0,0);
			break;
		case MODE_AIME_IO:
			if(Card.type != Card_None){
				AimeIO_No_Card();
			}
			break;
	}
	Card.type = Card_None;
	Card.operation = Operation_idle;
	memset(Card.data,0,128);
	rfalFieldOff();
}

bool T_Union_Read(){
	ReturnCode           	err;
    rfalIsoDepInitialize();
    err = rfalIsoDepPollAHandleActivation((rfalIsoDepFSxI)RFAL_ISODEP_FSDI_DEFAULT, RFAL_ISODEP_NO_DID, RFAL_BR_424, &gDevProto.isoDepDev);
    if( err == ERR_NONE )
    {
    	uint8_t tmp[128];
    	const uint8_t apdu_select_mf[] = {0x00 ,0xA4 ,0x04 ,0x00 ,0x0E ,0x32 ,0x50 ,0x41 ,0x59 ,0x2E ,0x53 ,0x59 ,0x53 ,0x2E ,0x44 ,0x44 ,0x46 ,0x30 ,0x31};
    	const uint8_t apdu_select_tunion[14] = {0x00 ,0xA4 ,0x04 ,0x00 ,0x08 ,0xA0 ,0x00 ,0x00 ,0x06 ,0x32 ,0x01 ,0x01 ,0x05};
		memset(tmp,0,128);
		uint16_t rxLen = 0;
		err = IsoDepBlockingTxRx(&gDevProto.isoDepDev,apdu_select_mf,sizeof(apdu_select_mf),tmp,128, &rxLen);
		if((err == ERR_NONE) && (APDU_check_response(tmp,rxLen))){
			err = IsoDepBlockingTxRx(&gDevProto.isoDepDev,apdu_select_tunion,sizeof(apdu_select_tunion),tmp,128, &rxLen);
			if((err == ERR_NONE) && (APDU_check_response(tmp,rxLen))){
				memcpy(Card.t_union_end_date,&tmp[rxLen-8],4);
				memcpy(Card.t_union_start_date,&tmp[rxLen-12],4);
				memcpy(Card.t_union_serial,&tmp[rxLen-22],10);
				return true;
			}
		}
    }
    return false;
}

uint8_t APDU_check_response(uint8_t *data,uint16_t len){
	if(data[len-2] == 0x61){
		return 2;
	}
	if((data[len-2] == 0x90) && (data[len-1] == 0x00)){
		return 1;
	}
	return 0;
}

ReturnCode nfcfReadBlock_default()
{
	uint8_t retry = 3;
    ReturnCode                 err;
    uint8_t                    buf[ (RFAL_NFCF_NFCID2_LEN + RFAL_NFCF_CMD_LEN + (4*16)) ];
    uint16_t                   rcvLen;
    rfalNfcfServ               srv = 0x000b;
    rfalNfcfBlockListElem _blockList[1];
    rfalNfcfServBlockListParam servBlock;

    servBlock.numServ   = 1;
    servBlock.servList  = &srv;
    servBlock.numBlock  = 1;
    servBlock.blockList = _blockList;
	_blockList[0].conf = 0x80;
	_blockList[0].blockNum = 0x00;

	try:
	err = rfalNfcfPollerCheck(Card.felica_IDm, &servBlock, buf, sizeof(buf), &rcvLen);
	if(err == ERR_NONE){
		memcpy(Card.block_8000,buf+1,16);
		return err;
	}else if(retry){
		retry --;
		goto try;
	}
	return RFAL_ERR_TIMEOUT;
}

ReturnCode nfcvReadBlock(uint8_t *uid, uint8_t blockNum, uint8_t *rxBuf, uint16_t bufSize)
{
    uint16_t rcvLen;
    ReturnCode err = rfalNfcvPollerReadSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, rxBuf, bufSize, &rcvLen);
    platformLog(" Read Block: %s %s\r\n",
               (err != ERR_NONE) ? "FAIL" : "OK Data:",
               (err != ERR_NONE) ? "" : hex2Str(&rxBuf[1], DEMO_NFCV_BLOCK_LEN));
    return err;
}

ReturnCode nfcvWriteBlock(uint8_t *uid, uint8_t blockNum, uint8_t *wrData, uint16_t dataLen)
{
    ReturnCode err = rfalNfcvPollerWriteSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, wrData, dataLen);
    platformLog(" Write Block: %s Data: %s\r\n",
               (err != ERR_NONE) ? "FAIL" : "OK",
               hex2Str(wrData, DEMO_NFCV_BLOCK_LEN));
    return err;
}

ReturnCode mifareAuthenticate(uint8_t keyType, uint8_t sector, uint8_t* uid, uint32_t uidLen, uint8_t* key)
{
    const uint32_t nonce = 0x94857192;
    ReturnCode err = mccAuthenticate(keyType, sector, uid, uidLen, key, nonce);
    if(err != ERR_NONE) {
    }
    return err;
}

ReturnCode mifareReadBlock(uint8_t sector, uint8_t block, uint8_t* buffer, uint16_t bufSize)
{
    uint8_t request[2] = {MCC_READ_BLOCK, (sector * 4) + block};
    uint16_t numBytesReceived;

    ReturnCode err = mccSendRequest(request, sizeof(request), buffer, bufSize,
                                  &numBytesReceived, MIFARE_READ_TIMEOUT, false);
    if(err == ERR_NONE) {
    }
    return err;
}

ReturnCode mifareWriteBlock(uint8_t sector, uint8_t blockNum, uint8_t *buff)
{
    uint8_t  rx[16];
    uint16_t rxLen = 0, rxActLen;
    ReturnCode ret;

    uint8_t absoluteBlock = sector * 4 + blockNum;

    uint8_t cmd[2] = { MCC_WRITE_BLOCK, absoluteBlock };
    ret = mccSendRequest(cmd, 2, rx, rxLen, &rxActLen, MIFARE_READ_TIMEOUT, false);
    if (ret != ERR_NONE)
        return ret;

    ret = mccSendRequest(buff, 16, rx, rxLen, &rxActLen, MIFARE_READ_TIMEOUT * 10, false);

    return ret;
}

static union{
    rfalIsoDepApduBufFormat  isoDepTxBuf;
    rfalNfcDepBufFormat      nfcDepTxBuf;
    uint8_t                  txBuf[DEMO_BUF_LEN];
}gTxBuf;

static union {
    rfalIsoDepApduBufFormat  isoDepRxBuf;
    rfalNfcDepBufFormat      nfcDepRxBuf;
    uint8_t                  rxBuf[DEMO_BUF_LEN];
}gRxBuf;

static rfalIsoDepBufFormat   tmpBuf;

ReturnCode IsoDepBlockingTxRx( rfalIsoDepDevice *isoDepDev, const uint8_t *txBuf, uint16_t txBufSize, uint8_t *rxBuf, uint16_t rxBufSize, uint16_t *rxActLen )
{
  ReturnCode               err;
  rfalIsoDepApduTxRxParam  isoDepTxRx;

  isoDepTxRx.txBuf        = &gTxBuf.isoDepTxBuf;
  isoDepTxRx.txBufLen     = txBufSize;
  isoDepTxRx.DID          = isoDepDev->info.DID;
  isoDepTxRx.FWT          = isoDepDev->info.FWT;
  isoDepTxRx.dFWT         = isoDepDev->info.dFWT;
  isoDepTxRx.FSx          = isoDepDev->info.FSx;
  isoDepTxRx.ourFSx       = RFAL_ISODEP_FSX_KEEP;
  isoDepTxRx.rxBuf        = &gRxBuf.isoDepRxBuf;
  isoDepTxRx.rxLen        = rxActLen;
  isoDepTxRx.tmpBuf       = &tmpBuf;

  memmove( gTxBuf.isoDepTxBuf.apdu, txBuf, MIN( txBufSize, RFAL_ISODEP_DEFAULT_FSC ) );

  rfalIsoDepStartApduTransceive( isoDepTxRx );
  do {
    rfalWorker();
    err = rfalIsoDepGetApduTransceiveStatus();
  } while(err == ERR_BUSY);

  platformLog(" ISO-DEP TxRx %s: - Tx: %s Rx: %s \r\n", (err != ERR_NONE) ? "FAIL": "OK", hex2Str((uint8_t*)txBuf, txBufSize), (err != ERR_NONE) ? "": hex2Str( isoDepTxRx.rxBuf->apdu, *rxActLen));

  if( err != ERR_NONE )
  {
    return err;
  }

  memmove( rxBuf, isoDepTxRx.rxBuf->apdu, MIN(*rxActLen, rxBufSize) );
  return ERR_NONE;
}

bool ReturnCode_proc(ReturnCode err){
	if(err == ERR_NONE){
		return false;
	}
	return false;
}

void NFC_Recover(void)
{
}

static uint32_t rng_state = 0;

void rng_init(void)
{
    rng_state = 0xA5A5A5A5u ^ (uint32_t)&rng_state;

    if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
    {
        rng_state ^= SysTick->VAL;
    }

    if (rng_state == 0)
    {
        rng_state = 0x1u;
    }
}

static uint32_t rng_get32(void)
{
    uint32_t x = rng_state;

    /* xorshift32 */
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    rng_state = x;
    return x;
}
