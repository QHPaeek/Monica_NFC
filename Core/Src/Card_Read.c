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

#define RFAL_NFCA_SEL_RES_CONF_MIFARE    	0x08 /* SEL_RES (SAK) Mifare configuration */
#define DEMO_NFCV_BLOCK_LEN           		4     /*!< NFCV Block len                      */
#define MIFARE_BLOCK_SIZE            		16
#define MIFARE_CRC_LEN                		2
#define MIFARE_READ_TIMEOUT           		50
//#define RFAL_CRC_LEN 						2
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

const uint8_t AimeKey[6] = {0x57, 0x43, 0x43, 0x46, 0x76, 0x32};
const uint8_t BanaKey_A[6] = {0x60, 0x90, 0xD0, 0x06, 0x32, 0xF5};
const uint8_t BanaKey_B[6] = {0x01, 0x97, 0x61, 0xAA, 0x80, 0x82};
const uint8_t JubeatKey[6] = {0xf4, 0x20, 0xd0, 0x09, 0x40, 0xa6};

uint8_t auth_flag = 0;

uint16_t Get_Card_ATQA(rfalNfcaSensRes atqa){
	//return ((atqa.anticollisionInfo << 8) | atqa.platformInfo);
	return ((atqa.platformInfo << 8) | atqa.anticollisionInfo);
}

void mifare_pre_read(){
	mccInitialize();
	Card.mifare_auth_status = 0;
	if(!auth_flag){
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
		auth_flag ++;
		goto read2;
	}else if(auth_flag == 1){
		if(mifareAuthenticate(MCC_AUTH_KEY_A, 0, Card.iso14443_uid4, 4, (uint8_t *)BanaKey_A) != RFAL_ERR_NONE){
			//platformLog("banakey a fail\r\n");
		}else{
			//platformLog("banakey a success\r\n");
			memcpy(Card.mifare_right_key_a,BanaKey_A,6);
			Card.mifare_auth_status |= Auth_KeyA_Right;
		}
		if(mifareAuthenticate(MCC_AUTH_KEY_B, 0, Card.iso14443_uid4, 4, (uint8_t *)BanaKey_B) != RFAL_ERR_NONE){
		}else{
			memcpy(Card.mifare_right_key_b,BanaKey_B,6);
			Card.mifare_auth_status |= Auth_KeyB_Right;
		}
		auth_flag ++;
		goto read1;
	}else{
		if(mifareAuthenticate(MCC_AUTH_KEY_A, 0, Card.iso14443_uid4, 4, (uint8_t *)JubeatKey) != RFAL_ERR_NONE){

		}else{
			memcpy(Card.mifare_right_key_a,JubeatKey,6);
			memset(Card.mifare_right_key_b,0xff,6);
			Card.mifare_auth_status = Auth_ALL_Right;
		}
		auth_flag = 0;
		goto read1;
	}
read1:
	if(Card.mifare_auth_status == Auth_ALL_Right){
		uint8_t tmp[18];
		for(uint8_t i = 1;i<3;i++){
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
read2:
	if(Card.mifare_auth_status == Auth_ALL_Right){
		uint8_t tmp[18];
		for(uint8_t i = 2;i>0;i--){
			mifareReadBlock(0, i, tmp, 18);
			memcpy(Card.mifare_data[i], tmp, 16);
//			platformLog(" Read block %d:");
//			for (int i = 0; i < 16; i++) {
//				platformLog("%02X ", Card.mifare_data[i]); // 两位大写16进制，不足补零
//			}
//			platformLog("\r\n");
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
//	if(rfalT2TPollerSectorSelect(0) != RFAL_ERR_NONE){
//		uint8_t tmp = 0xcc;
//		CDC_Transmit(0, &tmp, 1);
//		return;
//	}
	uint16_t rcv_len = 0;
	if (mifareUlReadNBytes(5, Card.nesica_serial, 16, &rcv_len) != RFAL_ERR_NONE) {
	  return;
	}
	for(uint8_t i = 0;i<16;i++){
		if(!((Card.nesica_serial[i] > 0x2f) && (Card.nesica_serial[i] < 0x3a ))){
		  return;
		}
	}
//	static uint8_t tmp[16];
//	ascii_to_accesscode(Card.nesica_serial ,tmp + 6);
//	CDC_Transmit(0, tmp, 16);
	Card.mifare_ul_read_status = 1;
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
    rfalFieldOnAndStartGT();
    /*******************************************************************************/
    /* NFC-A Technology Detection                                                  */
    /*******************************************************************************/

    rfalNfcaPollerInitialize();                                                       /* Initialize RFAL for NFC-A */


    //err = rfalNfcaPollerTechnologyDetection( RFAL_COMPLIANCE_MODE_NFC, &sensRes ); /* Poll for NFC-A devices */
    err = rfalNfcaPollerFullCollisionResolution(RFAL_COMPLIANCE_MODE_NFC,1,&nfcaDev,&devCnt);
    if( (err == ERR_NONE) && (devCnt > 0) )
    {
    	platformLedOn(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
    	//platformLog("ISO14443A/NFC-A card found. UID: %s\r\n", hex2Str(nfcaDev.nfcId1,nfcaDev.nfcId1Len));
    	if(nfcaDev.nfcId1Len == 4){
    		if((memcmp(Card.iso14443_uid4,nfcaDev.nfcId1,nfcaDev.nfcId1Len) == 0) && (Card.mifare_auth_status == Auth_ALL_Right)){
    			//platformLog("same,skip\n");
    				Card.operation = Operation_detected;
    				rfalFieldOff();
    				return;
    		}else if((Card.type == Card_Type_ISO14443A_T_Union) && (memcmp(Card.t_union_uid,nfcaDev.nfcId1,4) == 0)){
    			Card.operation = Operation_detected;
				rfalFieldOff();
				return;
    		}
    	}else if(nfcaDev.nfcId1Len == 7){
    		if(memcmp(Card.iso14443_uid7,nfcaDev.nfcId1,7) == 0){
				//platformLog("same,skip\n");
				if(Card.mifare_ul_read_status == 1){
					Card.operation = Operation_detected;
					rfalFieldOff();
					return;
				}
			}
    	}else{
    		rfalFieldOff();
    		return;
    	}
    	Card.operation = Operation_detected;
        /*******************************************************************************/
        /* Check if desired device is in Sleep                                         */
        if( nfcaDev.isSleep )
        {
            err = rfalNfcaPollerCheckPresence( RFAL_14443A_SHORTFRAME_CMD_WUPA, &nfcaDev.sensRes ); /* Wake up all cards  */
            if( err == ERR_NONE )
            {
            	err = rfalNfcaPollerSelect( nfcaDev.nfcId1, nfcaDev.nfcId1Len, &nfcaDev.selRes ); /* Select specific device  */
            }
        }
		/* Check if device supports Mifare */
		if (nfcaDev.type == RFAL_NFCA_T2T)
		{
			switch(Get_Card_ATQA(nfcaDev.sensRes)){
				case 0x0044:
					if(nfcaDev.nfcId1Len == 7){
						//Detected: Mifare Ultralight or NFC Type 2 Tag
						if(Card.type != Card_Type_Mifare_UltraLight){
							memset(Card.data,0,128);
							Card.type = Card_Type_Mifare_UltraLight;
						}
						memcpy(Card.iso14443_uid7,nfcaDev.nfcId1,7);
						mifare_ul_read();
			        	switch(Reader.Current_Mode){
							case MODE_IDLE:{
								uint8_t data[9] = {0x01,0xE0,0x04,0x01,0xAF};
								memcpy(data+5,Card.iso14443_uid7,4);
								USBD_CUSTOM_HID_SendReport(&hUsbDevice,data, 9);
								break;
							}
			        		case MODE_SPICE_API:
			        			spice_iso14443_process();
								break;
			        	}
					}
					break;
		        case 0x0004:{
		        	memcpy(Card.iso14443_uid4,nfcaDev.nfcId1,nfcaDev.nfcId1Len);
		        	//platformLog("Detected: Mifare Classic 1K\n");
		        	if(Card.type != Card_Type_Mifare_Classic){
		        		memset(Card.data,0,128);
		        	}
		        	mifare_pre_read();
//		        	if(Card.mifare_auth_status == Auth_ALL_Failed){
//		        		goto no_card;
//		        	}
		        	Card.type = Card_Type_Mifare_Classic;
		        	switch(Reader.Current_Mode){
		        		case MODE_IDLE:{
							uint8_t data[9] = {0x01,0xE0,0x04,0x01,0xAF};
							memcpy(data+5,Card.iso14443_uid4,4);
							USBD_CUSTOM_HID_SendReport(&hUsbDevice,data, 9);
							break;
						}
		        		case MODE_SPICE_API:
		        			spice_iso14443_process();
		        			break;
		        	}
		        	break;
		        }
		        case 0x0002:
		        	memcpy(Card.iso14443_uid4,nfcaDev.nfcId1,nfcaDev.nfcId1Len);
		        	//platformLog("Detected: Mifare Classic 4K\n");
		        	if(Card.type != Card_Type_Mifare_Classic){
		        		memset(Card.data,0,128);
		        	}
		        	mifare_pre_read();
//		        	if(Card.mifare_auth_status == Auth_ALL_Failed){
//						goto no_card;
//					}
		        	Card.type = Card_Type_Mifare_Classic;
		        	switch(Reader.Current_Mode){
		        		case MODE_IDLE:{
							uint8_t data[9] = {0x01,0xE0,0x04,0x01,0xAF};
							memcpy(data+5,Card.iso14443_uid4,4);
							USBD_CUSTOM_HID_SendReport(&hUsbDevice,data, 9);
							break;
						}
		        		case MODE_SPICE_API:
		        			spice_iso14443_process();
		        			break;
		        	}
		            break;
			}
		}
		/* Check if it is Topaz aka T1T */
		else if( nfcaDev.type == RFAL_NFCA_T1T )
		{
			/********************************************/
			/* NFC-A T1T card found                     */
			/* NFCID/UID is contained in: t1tRidRes.uid */
//			uint8_t tmp[128];
//			sprintf(tmp,"ISO14443A/Topaz (NFC-A T1T) TAG found. UID: %s\r\n", hex2Str(nfcaDev.ridRes.uid, RFAL_T1T_UID_LEN));
//			CDC_Transmit(0, tmp, strlen(tmp));
			memcpy(Card.iso14443_uid4,nfcaDev.nfcId1,nfcaDev.nfcId1Len);
	    	Card.type = Card_Type_ISO14443A_Unknow;
		}
		/* Check if device supports P2P/NFC-DEP */
		else if( (nfcaDev.type == RFAL_NFCA_NFCDEP) || (nfcaDev.type == RFAL_NFCA_T4T_NFCDEP))
		{
	        /* Continue with P2P Activation .... */

//	        err = ActivateP2P( NFCID3, RFAL_NFCDEP_NFCID3_LEN, false, &gDevProto.nfcDepDev );
//	        if (err == ERR_NONE)
//	        {
//	          /*********************************************/
//	          /* Passive P2P device activated              */
//	          //platformLog("NFCA Passive P2P device found. NFCID: %s\r\n", hex2Str(gDevProto.nfcDepDev.activation.Target.ATR_RES.NFCID3, RFAL_NFCDEP_NFCID3_LEN));
//				uint8_t tmp[128];
//				sprintf(tmp,"NFCA Passive P2P device found. NFCID: %s\r\n", hex2Str(gDevProto.nfcDepDev.activation.Target.ATR_RES.NFCID3, RFAL_NFCDEP_NFCID3_LEN));
//				CDC_Transmit(0, tmp, strlen(tmp));
//	          /* Send an URI record */
//	          //demoSendNdefUri();
//	        }
			memcpy(Card.iso14443_uid4,nfcaDev.nfcId1,nfcaDev.nfcId1Len);
			Card.type = Card_Type_ISO14443A_Unknow;
	     }
	     /* Check if device supports ISO14443-4/ISO-DEP */
	     else if (nfcaDev.type == RFAL_NFCA_T4T)
	     {
	        /* Activate the ISO14443-4 / ISO-DEP layer */

	    	  if(T_Union_Read()){
	    		  Card.type = Card_Type_ISO14443A_T_Union;
	    		  memcpy(Card.t_union_uid,nfcaDev.nfcId1,4);
	    		  switch(Reader.Current_Mode){
	    		  	  case MODE_IDLE:{
						uint8_t data[9] = {0x01,0xE0,0x04,0x01,0xAF};
						memcpy(data+5,Card.t_union_uid,4);
						USBD_CUSTOM_HID_SendReport(&hUsbDevice,data, 9);
						break;
					}
	    		  	case MODE_SPICE_API:
						spice_iso14443_process();
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
	/*******************************************************************************/
	/* Felica/NFC_F_PASSIVE_POLL_MODE                                              */
	/*******************************************************************************/

	rfalNfcfPollerInitialize( RFAL_BR_212 ); /* Initialize for NFC-F */

	err = rfalNfcfPollerCheckPresence();
	if( err == ERR_NONE )
	{
		err = rfalNfcfPollerCollisionResolution( RFAL_COMPLIANCE_MODE_NFC, 1, &nfcfDev, &devCnt );
		if( (err == ERR_NONE) && (devCnt > 0) )
		{
			/******************************************************/
			/* NFC-F card found                                   */
			/* NFCID/UID is contained in: nfcfDev.sensfRes.NFCID2 */
			//platformLog("Felica/NFC-F card found. UID: %s\r\n", hex2Str(nfcfDev.sensfRes.NFCID2, RFAL_NFCF_NFCID2_LEN));
			platformLedOn(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
			if(Card.type != Card_Type_Felica_AIC){
				memset(Card.data,0,128);
			}
			Card.type = Card_Type_Felica_AIC;
			Card.operation = Operation_detected;
			memcpy(Card.felica_IDm,nfcfDev.sensfRes.NFCID2, RFAL_NFCF_NFCID2_LEN);
			memcpy(Card.felica_PMm,&nfcfDev.sensfRes.PAD0[0], 8);
			memcpy(Card.felica_systemcode,nfcfDev.sensfRes.RD,2);
//			CDC_Transmit(0, Card.felica_IDm,  16);
//			CDC_Transmit(0, Card.felica_PMm,  16);
//			rfalFieldOff();
//			demoNfcf();

        	switch(Reader.Current_Mode){
        		case MODE_IDLE:
        			LED_show(0,0,255);
        			uint8_t data[9];
					data[0] = 1;
					memcpy(data+1,Card.felica_IDm,8);
					USBD_CUSTOM_HID_SendReport(&hUsbDevice,data, 9);
        			break;
        		case MODE_SPICE_API:
        			spice_felice_process();
        			break;
        	}
			rfalFieldOff();
			return;
		}
		rfalFieldOff();
		return;
	}
	/*******************************************************************************/
	/* ISO15693/NFC_V_PASSIVE_POLL_MODE                                            */
	/*******************************************************************************/

	rfalNfcvPollerInitialize();           /* Initialize for NFC-V */

	err = rfalNfcvPollerCollisionResolution(1,1, &nfcvDev, &devCnt);
	if( (err == ERR_NONE) && (devCnt > 0) )
	{
		/******************************************************/
		/* NFC-V card found                                   */
		/* NFCID/UID is contained in: invRes.UID */
		REVERSE_BYTES(nfcvDev.InvRes.UID, RFAL_NFCV_UID_LEN);
		//platformLog("ISO15693/NFC-V card found. UID: %s\r\n", hex2Str(nfcvDev.InvRes.UID, RFAL_NFCV_UID_LEN));
		platformLedOn(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
		if(Card.type != Card_Type_ISO15693){
			memset(Card.data,0,128);
		}
		Card.type = Card_Type_ISO15693;
		Card.operation = Operation_detected;
		memcpy(Card.iso15693_uid,nfcvDev.InvRes.UID, RFAL_NFCV_UID_LEN);
    	switch(Reader.Current_Mode){
    		case MODE_SPICE_API:
    			spice_iso15693_process();
    			break;
    		case MODE_IDLE:{
    			uint8_t data[9];
    			data[0] = 1;
    			memcpy(data+1,Card.iso15693_uid,8);
    			LED_show(0,128,128);
    			USBD_CUSTOM_HID_SendReport(&hUsbDevice,data, 9);
    			break;
    		}
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
    Card.type = Card_None;
    Card.operation = Operation_idle;
    if(Reader.Current_Mode == MODE_IDLE){
    	LED_show(0,0,0);
    }
    memset(Card.data,0,128);
    rfalFieldOff();
}

bool T_Union_Read(){
	ReturnCode           	err;
    rfalIsoDepInitialize();
    err = rfalIsoDepPollAHandleActivation((rfalIsoDepFSxI)RFAL_ISODEP_FSDI_DEFAULT, RFAL_ISODEP_NO_DID, RFAL_BR_424, &gDevProto.isoDepDev);
    if( err == ERR_NONE )
    {
//	          //platformLog("ISO14443-4/ISO-DEP layer activated. \r\n");
    	uint8_t tmp[128];
    	//const uint8_t apdu_read[5] = {0x80,0x5C ,0x00 ,0x02 ,0x04};
    	//const uint8_t apdu_select_mf[] = {0x00,0xA4,0x00,0x00,0x02, 0x3F,0x00};
    	const uint8_t apdu_select_mf[] = {0x00 ,0xA4 ,0x04 ,0x00 ,0x0E ,0x32 ,0x50 ,0x41 ,0x59 ,0x2E ,0x53 ,0x59 ,0x53 ,0x2E ,0x44 ,0x44 ,0x46 ,0x30 ,0x31};
    	//const uint8_t apdu_read[5] = {0x00,0xB2,0x01 ,0x0C ,0x00};
    	//const uint8_t apdu_read_2[5] = {0x00 ,0xB0 ,0x95 ,0x00 ,0x1e};
    	//const uint8_t apdu_read_serial[5] = {0x00 ,0xB2 ,0x01 ,0x04 ,0x00};

    	//const uint8_t apdu_select_pboc[14] = {0x00 ,0xA4 ,0x04 ,0x00 ,0x07 ,0xA0 ,0x00 ,0x00 ,0x00 ,0x03 ,0x86 ,0x98 ,0x07 ,0x01};
    	const uint8_t apdu_select_tunion[14] = {0x00 ,0xA4 ,0x04 ,0x00 ,0x08 ,0xA0 ,0x00 ,0x00 ,0x06 ,0x32 ,0x01 ,0x01 ,0x05};
    	const uint8_t apdu_read_balance[] = {0x80,0x5C ,0x00 ,0x02 ,0x04};
////				sprintf(tmp,"ISO14443-4/ISO-DEP layer activated.UID:%s\r\n", hex2Str(nfcaDev.nfcId1,nfcaDev.nfcId1Len));
////				CDC_Transmit(0, tmp, strlen(tmp));
		memset(tmp,0,128);
		uint16_t rxLen = 0;
		err = IsoDepBlockingTxRx(&gDevProto.isoDepDev,apdu_select_mf,sizeof(apdu_select_mf),tmp,128, &rxLen);
		if((err == ERR_NONE) && (APDU_check_response(tmp,rxLen))){
			err = IsoDepBlockingTxRx(&gDevProto.isoDepDev,apdu_select_tunion,sizeof(apdu_select_tunion),tmp,128, &rxLen);
			if((err == ERR_NONE) && (APDU_check_response(tmp,rxLen))){
				memcpy(Card.t_union_end_date,&tmp[rxLen-8],4);
				memcpy(Card.t_union_start_date,&tmp[rxLen-12],4);
				memcpy(Card.t_union_serial,&tmp[rxLen-22],10);
				//CDC_Transmit(0, Card.t_union_serial, 10);
				return true;
			}
		}
    }
    return false;
}

uint8_t APDU_check_response(uint8_t *data,uint16_t len){
	if(data[len-2] == 0x61){
		//SUCCESS,NEED GET RESPONSE
		return 2;
	}
	if((data[len-2] == 0x90) && (data[len-1] == 0x00)){
		return 1;
	}
	return 0;
}

//void demo_suica()
//{
//    ReturnCode                 err;
//    rfalNfcfListenDevice  		nfcfDev;
//    uint8_t                    buf[ (RFAL_NFCF_NFCID2_LEN + RFAL_NFCF_CMD_LEN + (4*RFAL_NFCF_BLOCK_LEN)) ];
//    uint16_t                   rcvLen;
//    rfalNfcfServ               srv = 0x008b;
//    //rfalNfcfServ               srv = 0x090f;
//    rfalNfcfBlockListElem _blockList[4];
//    rfalNfcfServBlockListParam servBlock;
//    uint8_t              	devCnt = 0;
//
//    servBlock.numServ   = 1;                            /* Only one Service to be used           */
//    servBlock.servList  = &srv;                         /* Service Code: NDEF is Read/Writeable  */
//    servBlock.numBlock  = num_block;                            /* Only one block to be used             */
//    servBlock.blockList = _blockList;
//	for(uint8_t i = 0;i<num_block;i++){
//		_blockList[i].conf = RFAL_NFCF_BLOCKLISTELEM_LEN_BIT;
//		_blockList[i].blockNum = blockList[i];
//	}
//
//    rfalFieldOff();
//    while(1){
//    	rfalNfcfPollerInitialize( RFAL_BR_212 ); /* Initialize for NFC-F */
//		rfalFieldOnAndStartGT();                 /* Turns the Field On if not already and start GT timer */
//
//		err = rfalNfcfPollerCheckPresence();
//		if( err == ERR_NONE)
//		{
//			err = rfalNfcfPollerCollisionResolution( RFAL_COMPLIANCE_MODE_NFC, 1, &nfcfDev, &devCnt );
//			if( (err == ERR_NONE) && (devCnt > 0))
//			{
//				if(memcmp(idm,nfcfDev.sensfRes.NFCID2,8)){
//					return RFAL_ERR_TIMEOUT;
//				}
//				err = rfalNfcfPollerCheck(idm, &servBlock, buf, sizeof(buf), &rcvLen);
//				if(err == ERR_NONE){
//					//DECRYPT_ACCESSCODE(buf+1);
//					//CDC_Transmit(0, buf+1,  16);
//				    for(uint8_t i = 0;i<num_block;i++){
//				    	memcpy(blockdata[i],&buf[1 + (16*i)],16);
//				    }
//				    rfalFieldOff();
//				    return err;
//				}
//			}
//
//		}
//		rfalFieldOff();
//		return RFAL_ERR_TIMEOUT;
//    }
//}


//ReturnCode nfcfReadBlock(uint8_t *idm,uint16_t *serviceList ,uint8_t num_block,uint8_t *blockList ,uint8_t blockdata[4][16])
//{
//    ReturnCode                 err;
//    rfalNfcfListenDevice  		nfcfDev;
//    uint8_t                    buf[ (RFAL_NFCF_NFCID2_LEN + RFAL_NFCF_CMD_LEN + (4*16)) ];
//    uint16_t                   rcvLen;
//    rfalNfcfServ               srv = *serviceList;
//    //rfalNfcfServ               srv = 0x090f;
//    rfalNfcfBlockListElem _blockList[4];
//    rfalNfcfServBlockListParam servBlock;
//    uint8_t              	devCnt = 0;
//
//    servBlock.numServ   = 1;                            /* Only one Service to be used           */
//    servBlock.servList  = &srv;                         /* Service Code: NDEF is Read/Writeable  */
//    servBlock.numBlock  = num_block;                            /* Only one block to be used             */
//    servBlock.blockList = _blockList;
//	for(uint8_t i = 0;i<num_block;i++){
//		_blockList[i].conf = RFAL_NFCF_BLOCKLISTELEM_LEN_BIT;
//		_blockList[i].blockNum = blockList[i];
//	}
//
//    rfalFieldOff();
//	rfalNfcfPollerInitialize( RFAL_BR_212 ); /* Initialize for NFC-F */
//	rfalFieldOnAndStartGT();                 /* Turns the Field On if not already and start GT timer */
//
//	err = rfalNfcfPollerCheckPresence();
//	if( err == ERR_NONE)
//	{
//		err = rfalNfcfPollerCollisionResolution( RFAL_COMPLIANCE_MODE_NFC, 1, &nfcfDev, &devCnt );
//		if( (err == ERR_NONE) && (devCnt > 0))
//		{
//			if(memcmp(idm,nfcfDev.sensfRes.NFCID2,8)){
//				return RFAL_ERR_TIMEOUT;
//			}
//			err = rfalNfcfPollerCheck(idm, &servBlock, buf, sizeof(buf), &rcvLen);
//			if(err == ERR_NONE){
//				//DECRYPT_ACCESSCODE(buf+1);
//				//CDC_Transmit(0, buf+1,  16);
//				for(uint8_t i = 0;i<num_block;i++){
//					memcpy(blockdata[i],&buf[1 + (16*i)],16);
//				}
//				rfalFieldOff();
//				return err;
//			}
//		}
//
//	}
//	rfalFieldOff();
//	return RFAL_ERR_TIMEOUT;
//}

//ReturnCode nfcfReadBlock(uint8_t *idm,uint16_t *serviceList ,uint8_t num_block,uint8_t *blockList ,uint8_t blockdata[4][16])
//{
//	rfalNfcfServ               srv = 0x000b;
//    rfalNfcfServBlockListParam servBlock;
//    rfalNfcfBlockListElem _blockList[4];
//    uint16_t rcvLen = 0;
//    uint8_t buf[ (RFAL_NFCF_NFCID2_LEN + RFAL_NFCF_CMD_LEN + (4*RFAL_NFCF_BLOCK_LEN)) ];
//
//    servBlock.numServ   = 1;
//    //servBlock.servList  = serviceList;
//    servBlock.servList  = &srv;
//    servBlock.numBlock  = num_block;
//    servBlock.blockList = _blockList;
//    for(uint8_t i = 0;i<num_block;i++){
//    	_blockList[i].conf = RFAL_NFCF_BLOCKLISTELEM_LEN_BIT;
//    	_blockList[i].blockNum = blockList[i];
//    }
//
//    ReturnCode err = rfalNfcfPollerCheck(idm, &servBlock, buf, sizeof(buf), &rcvLen);
//    if(err != RFAL_ERR_NONE){
//    	return err;
//    }
////    platformLog(" Read Block: %s Data: %s \r\n",
////               (err != ERR_NONE) ? "FAIL" : "OK",
////               (err != ERR_NONE) ? "" : hex2Str(&buf[1], RFAL_NFCF_BLOCK_LEN));
//
//    for(uint8_t i = 0;i<num_block;i++){
//    	memcpy(blockdata[i],&buf[1 + (16*i)],16);
//    }
//    return err;
//}

//ReturnCode nfcfWriteSingleBlock(uint8_t *idm, uint8_t num_service,uint16_t *serviceList ,uint16_t *blockList,uint8_t *blockdata)
//{
//    rfalNfcfServBlockListParam servBlock;
//    rfalNfcfBlockListElem _blockList[4];
//    uint8_t buf[ (RFAL_NFCF_NFCID2_LEN + RFAL_NFCF_CMD_LEN + (3*RFAL_NFCF_BLOCK_LEN)) ];
//
//    servBlock.numServ   = num_service;
//    servBlock.servList  = serviceList;
//    servBlock.numBlock  = 1;
//    servBlock.blockList = _blockList;
//	_blockList[0].conf = RFAL_NFCF_BLOCKLISTELEM_LEN_BIT;
//	_blockList[0].blockNum = blockList[0];
//
//    ReturnCode err = rfalNfcfPollerUpdate(idm, &servBlock, buf, sizeof(buf), blockdata, buf, sizeof(buf));
//
////    platformLog(" Write Block: %s Data: %s \r\n",
////               (err != ERR_NONE) ? "FAIL" : "OK",
////               (err != ERR_NONE) ? "" : hex2Str(wrData, RFAL_NFCF_BLOCK_LEN));
//
//    return err;
//}

//void demoNfcv(void)
//{
//    ReturnCode            err;
//    rfalNfcvListenDevice  nfcvDev  = {0};
//    uint16_t              rcvLen;
//    uint8_t               blockNum = 1;
//    uint8_t               rxBuf[ 1 + DEMO_NFCV_BLOCK_LEN + RFAL_CRC_LEN ];                        /* Flags + Block Data + CRC */
//    uint8_t *             uid;
//    uint8_t               wrData[DEMO_NFCV_BLOCK_LEN] = { 0x11, 0x22, 0x33, 0x99 };             /* Write block example */
//
//
//    uid = nfcvDev.InvRes.UID;
//
//    #if DEMO_NFCV_USE_SELECT_MODE
//        /*
//        * Activate selected state
//        */
//        err = rfalNfcvPollerSelect(RFAL_NFCV_REQ_FLAG_DEFAULT, nfcvDev->InvRes.UID );
//        platformLog(" Select %s \r\n", (err != ERR_NONE) ? "FAIL (revert to addressed mode)": "OK" );
//        if( err == ERR_NONE )
//        {
//            uid = NULL;
//        }
//    #endif
//
//    /*
//    * Read block using Read Single Block command
//    * with addressed mode (uid != NULL) or selected mode (uid == NULL)
//    */
//    err = rfalNfcvPollerReadSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, rxBuf, sizeof(rxBuf), &rcvLen);
//    platformLog(" Read Block: %s %s\r\n", (err != ERR_NONE) ? "FAIL": "OK Data:", (err != ERR_NONE) ? "" : hex2Str( &rxBuf[1], DEMO_NFCV_BLOCK_LEN));
//
//        err = rfalNfcvPollerWriteSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, wrData, sizeof(wrData));
//        platformLog(" Write Block: %s Data: %s\r\n", (err != ERR_NONE) ? "FAIL": "OK", hex2Str( wrData, DEMO_NFCV_BLOCK_LEN) );
//        err = rfalNfcvPollerReadSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, rxBuf, sizeof(rxBuf), &rcvLen);
//        platformLog(" Read Block: %s %s\r\n", (err != ERR_NONE) ? "FAIL": "OK Data:", (err != ERR_NONE) ? "" : hex2Str( &rxBuf[1], DEMO_NFCV_BLOCK_LEN));
//}

ReturnCode nfcvReadBlock(rfalNfcvListenDevice *device, uint8_t blockNum, uint8_t *rxBuf, uint16_t bufSize, uint8_t *uid)
{
    uint16_t rcvLen;
    ReturnCode err = rfalNfcvPollerReadSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, rxBuf, bufSize, &rcvLen);
    platformLog(" Read Block: %s %s\r\n",
               (err != ERR_NONE) ? "FAIL" : "OK Data:",
               (err != ERR_NONE) ? "" : hex2Str(&rxBuf[1], DEMO_NFCV_BLOCK_LEN));
    return err;
}

ReturnCode nfcvWriteBlock(rfalNfcvListenDevice *device, uint8_t blockNum, uint8_t *wrData, uint16_t dataLen, uint8_t *uid)
{
    ReturnCode err = rfalNfcvPollerWriteSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, wrData, dataLen);
    platformLog(" Write Block: %s Data: %s\r\n",
               (err != ERR_NONE) ? "FAIL" : "OK",
               hex2Str(wrData, DEMO_NFCV_BLOCK_LEN));
    return err;
}

ReturnCode mifareAuthenticate(uint8_t keyType, uint8_t sector, uint8_t* uid, uint32_t uidLen, uint8_t* key)
{
//    const uint32_t nonce = 0x94857192;
//    ReturnCode err = mccAuthenticate(keyType, sector, uid, uidLen, key, nonce);
    ReturnCode err = mccAuthenticate(keyType, sector, uid, uidLen, key, rng_get32());
    if(err != ERR_NONE) {
        //platformLog("Authentication failed:%04X\r\n",err);
//        mccDeinitialise(true);
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
//        platformLog(" Read block %d: %s\r\n", block,
//                   hex2Str(buffer, bufSize - MIFARE_CRC_LEN));
    }
//    mccDeinitialise(true);
    return err;
}

static union{
    rfalIsoDepApduBufFormat  isoDepTxBuf;                                /* ISO-DEP Tx buffer format (with header/prologue) */
    rfalNfcDepBufFormat      nfcDepTxBuf;                                /* NFC-DEP Rx buffer format (with header/prologue) */
    uint8_t                  txBuf[DEMO_BUF_LEN];                        /* Generic buffer abstraction                      */
}gTxBuf;


/*! Receive buffers union, only one interface is used at a time                                                             */
static union {
    rfalIsoDepApduBufFormat  isoDepRxBuf;                                /* ISO-DEP Rx buffer format (with header/prologue) */
    rfalNfcDepBufFormat      nfcDepRxBuf;                                /* NFC-DEP Rx buffer format (with header/prologue) */
    uint8_t                  rxBuf[DEMO_BUF_LEN];                        /* Generic buffer abstraction                      */
}gRxBuf;

static rfalIsoDepBufFormat   tmpBuf;

ReturnCode IsoDepBlockingTxRx( rfalIsoDepDevice *isoDepDev, const uint8_t *txBuf, uint16_t txBufSize, uint8_t *rxBuf, uint16_t rxBufSize, uint16_t *rxActLen )
{
  ReturnCode               err;
  rfalIsoDepApduTxRxParam  isoDepTxRx;

  /* Initialize the ISO-DEP protocol transceive context */
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


  /* Copy data to send */
  memmove( gTxBuf.isoDepTxBuf.apdu, txBuf, MIN( txBufSize, RFAL_ISODEP_DEFAULT_FSC ) );

  /* Perform the ISO-DEP Transceive in a blocking way */
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

  /* Copy received data */
  memmove( rxBuf, isoDepTxRx.rxBuf->apdu, MIN(*rxActLen, rxBufSize) );
  return ERR_NONE;
}

static uint32_t rng_state = 0;

void rng_init(void)
{
    /* 利用 SRAM 上电态 + SysTick 作为扰动 */
    rng_state = 0xA5A5A5A5u ^ (uint32_t)&rng_state;

    /* 如果 SysTick 已启用，用当前计数器再搅一次 */
    if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk)
    {
        rng_state ^= SysTick->VAL;
    }

    /* 防止为 0（xorshift 不能是 0） */
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

