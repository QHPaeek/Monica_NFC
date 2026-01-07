/*
 * mode_namco_serial.c
 *
 *  Created on: Aug 25, 2025
 *      Author: Qinh
 */
#include <stdbool.h>
#include <stdint.h>
#include "Card_Reader.h"
#include "mode_namco_serial.h"

uint8_t *namco_cmd_data = Card.operation_tmp;
uint8_t namco_led_mode = 0;
static LedCtrl_t led = {
    .mode = LED_MODE_OFF
};


PN532_Down_Frame down_frame;

static bool pn532_check_len(uint8_t len, uint8_t lcs)
{
    return (uint8_t)(len + lcs) == 0x00;
}

static bool pn532_check_dcs(uint8_t *data, uint8_t len, uint8_t dcs)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++)
        sum += data[i];

    sum += dcs;
    return sum == 0x00;
}

bool PN532_ParseFrame(uint8_t *buf, uint16_t buf_len, PN532_Down_Frame *out)
{
	memset(out, 0, sizeof(PN532_Down_Frame));
    if (buf_len < 8)
        return false;

    // 1. 固定头
    if (buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0xFF){
//    	test_printf(0x17);
    	return false;
    }

    uint8_t len = buf[3];
    uint8_t lcs = buf[4];

    if (!pn532_check_len(len, lcs)){
//    	test_printf(0x27);
        return false;
    }
    // 最小帧长度校验
    // 00 00 FF LEN LCS TFI CMD DCS 00
    uint16_t frame_len = 3 + 2 + len + 2;
    if (buf_len < frame_len){
//    	test_printf(0x37);
        return false;
    }
    // 2. TFI
    uint8_t tfi = buf[5];
    if (tfi != PN532_HOST_TO_PN){
//    	test_printf(0x47);
    	return false;
    }
    // 3. DCS 校验
    uint8_t *dcs_ptr = &buf[5 + len];
    if (!pn532_check_dcs(&buf[5], len, *dcs_ptr))
        //return false;

    // 4. Postamble
    if (buf[5 + len + 1] != 0x00){
//    	test_printf(0x57);
    	return false;
    }
    // 5. 填充结构体
    out->preamble   = buf[0];
    out->startCode1 = buf[1];
    out->startCode2 = buf[2];
    out->len        = len;
    out->lcs        = lcs;
    out->tfi        = tfi;
    out->cmd        = buf[6];

    uint8_t data_len = len - 2;
    if (data_len > 0)
    {
        memcpy(out->data, &buf[7], data_len);
    }

    out->dcs       = *dcs_ptr;
    out->postamble = 0x00;

    return true;
}

uint8_t namco_packet_check(uint8_t* data,uint8_t len){
	if(PN532_ParseFrame(data, len, &down_frame)){
		return down_frame.cmd;
	}else{
		return 0;
	}
}
void namco_packet_process(uint8_t cmd){
	uint8_t status = PN532_STATUS_OK;
	switch(cmd){
		case PN532_CMD_IN_SELECT:{
			PN532_SendResponse(&status, 1,1);
			break;
		}
		case PN532_CMD_IN_DESELECT:{
			uint8_t rsp[2] = {1,0};
			PN532_SendResponse(rsp, 2,1);
			break;
		}
		case PN532_CMD_IN_RELEASE:{
			uint8_t rsp[2] = {1,0};
			PN532_SendResponse(rsp, 2,1);
			break;
		}
		case PN532_CMD_GET_FIRMWARE_VERSION:{
			uint8_t rsp[4];
		    rsp[0] = 0x32;   // IC = PN532
		    rsp[1] = 0x01;   // Ver
		    rsp[2] = 0x06;   // Rev
		    rsp[3] = 0x0F;   // ISO14443A/B + FELICA + 14443-4
		    PN532_SendResponse(rsp, 4,1);
	        break;
		}
		case PN532_CMD_GET_GENERAL_STATUS:{
			uint8_t rsp[3] = {0x00, 0x00, 0x00};
			PN532_SendResponse(rsp, 3,1);
			break;
		}
		case PN532_CMD_RF_CONFIGURATION:
	        if (down_frame.data[1] == 0x81) {
	            uint8_t err = 0xF8; // Syntax Error
	            PN532_SendResponse(&err, 1,1);
	        } else {
//	            PN532_SendResponse(&status, 1,1);
	        	PN532_SendResponse(NULL, 0,1);
	        }
	        break;
		case PN532_CMD_READ_REGISTER: {
			if (down_frame.len == 0x12) {
				uint8_t rsp[] = {
					0xFF, 0x3F, 0x0E, 0xF1,
					0xFF, 0x3F, 0x0E, 0xF1,
				};
				PN532_SendResponse(rsp, sizeof(rsp),1);
			} else if (down_frame.len == 0x18) {
				uint8_t rsp[] = {
					0xDC, 0xF4, 0x3F, 0x11,
					0x4D, 0x85, 0x61, 0xF1,
					0x26, 0x6A, 0x87
				};
				PN532_SendResponse(rsp, sizeof(rsp),1);
			} else {
				PN532_SendResponse(&status, 1,1);
			}
			break;
		}
	    case PN532_CMD_WRITE_REGISTER: {
	        uint8_t addr = down_frame.data[1];

	        if (addr == 0xFF) {
	            uint8_t err = 0x22; // Unknown SFR
	            PN532_SendResponse(&err, 1,1);
	        } else if (addr == 0x63) {
	            PN532_SendResponse(&status, 1,1);
	        } else {
	            PN532_SendResponse(&status, 1,1);
	        }
	        break;
	    }
	    case PN532_CMD_READ_GPIO: {
	        uint8_t rsp[3] = {0x20, 0x06, 0x00};
	        PN532_SendResponse(rsp, sizeof(rsp),1);
	        break;
	    }
	    case PN532_CMD_WRITE_GPIO: {
	        uint8_t sub  = down_frame.data[0];
	        uint8_t mode = down_frame.data[1];

	        if (sub == 0x08) {
	            // handle beep
	        } else if (sub == 0x01) {
	            // handle led
	        	platformLedToogle(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
	        	setLEDMode(mode);
	        }

//	        uint8_t rsp = 0x1C;
//	        PN532_SendResponse(&rsp, 1,1);
	        PN532_SendResponse(NULL, 0,1);
	        break;
	    }
		case PN532_CMD_SAM_CONFIGURATION:
			PN532_SendResponse(&status, 1,1);
			break;
		case PN532_CMD_SET_PARAMETERS:
			PN532_SendResponse(NULL, 0,1);
			break;
		case PN532_CMD_IN_DATA_EXCHANGE:
			pn532_in_data_exchange_proc(down_frame.data);
			break;
	    case PN532_CMD_IN_LIST_PASSIVE_TARGET:
	    	uint8_t brty = down_frame.len;
	    	if (brty == 0x09) {
	    	    pn532_rsp_list_passive_target_felica();
			}else{
				pn532_rsp_list_passive_target_type_a();
			}
	        break;
	    case PN532_CMD_IN_AUTO_POLL:{
	        uint8_t rsp[1] = {0x00};   // No target
	        PN532_SendResponse(rsp, 1,1);
	        break;
	    }
		case NAMCO_CMD_DIAGNOSE: {
//			uint8_t rsp = 0x12;
//			PN532_SendResponse(&rsp, 1);
			PN532_SendResponse(NULL, 0,1);
			break;
		}
		case NAMCO_CMD_READ_FELICA:{
			namco_read_felica();
		}
	    default:
//	        status = 0x7F;
//	        PN532_SendResponse(&status, 1,1);
	        break;
	}
}

static uint8_t pending_write_block;
static bool write_pending = false;

void pn532_in_data_exchange_proc(uint8_t *buf){
//	uint8_t target = buf[0];
	uint8_t cmd = buf[1];
	uint8_t status = PN532_STATUS_OK;
	switch(cmd){
		case MIFARE_CMD_AUTH_A:{
			uint8_t block = buf[2];
			uint8_t *key  = &buf[3];      // 6 bytes
//			uint8_t *uid  = &buf[9];      // 4 bytes
//			if(block != 0){
//				status = PN532_STATUS_AUTH_ERROR;
//			}
			if(memcmp(key,Card.mifare_right_key_a,6)){
				status = PN532_STATUS_AUTH_ERROR;
			}
			PN532_SendResponse(&status, 1,1);
			break;
		}
		case MIFARE_CMD_AUTH_B:{
			uint8_t block = buf[2];
			uint8_t *key  = &buf[3];      // 6 bytes
//			uint8_t *uid  = &buf[9];      // 4 bytes
//			if(block != 0){
//				status = PN532_STATUS_AUTH_ERROR;
//			}
			if(memcmp(key,Card.mifare_right_key_b,6)){
				status = PN532_STATUS_AUTH_ERROR;
			}
			PN532_SendResponse(&status, 1,1);
			break;
		}
		case MIFARE_CMD_READ:{
			uint8_t block = buf[2];
			uint8_t read_data[17] = {0};
			memcpy(read_data + 1,Card.mifare_data[block], 16);
			if(block < 4){
				PN532_SendResponse(read_data, 17,1);
			}else{
				goto error;
			}
			break;
		}
		case MIFARE_CMD_WRITE:{
			if(!write_pending){
				pending_write_block = buf[2];
				write_pending = true;
				PN532_SendResponse(NULL, 0,1);
				return;
			}else{
//				uint8_t *write_data = &buf[1];
				write_pending = false;
				//todo:write card
				status = PN532_STATUS_OK;
				PN532_SendResponse(&status, 1,1);
			}
			break;
		}
		case MIFARE_CMD_INCREMENT:
		case MIFARE_CMD_DECREMENT:
		case MIFARE_CMD_TRANSFER:
		case MIFARE_CMD_RESTORE:{
			//ignore
	        status = PN532_STATUS_OK;
	        PN532_SendResponse(&status, 1,1);
	        break;
	    }
	    case FELICA_CMD_POLLING: {
	        uint8_t rsp[18];

	        rsp[0] = 0x12;      // Length
	        rsp[1] = 0x01;      // Response Code = Polling Response

	        /* NFCID2 (8 bytes) */
	        memcpy(&rsp[2], Card.felica_IDm, 8);

	        /* PAD (8 bytes) */
	        memset(&rsp[10], 0x00, 8);

	        PN532_SendResponse(rsp, 18,1);
	        break;
	    }

	    case FELICA_CMD_READ_WO_ENCRYPTION: {
	        uint8_t rsp[32];

	        rsp[0] = 0x1A;      // Length（示例）
	        rsp[1] = 0x07;      // Read Response
	        rsp[2] = 0x00;      // Status Flag1
	        rsp[3] = 0x00;      // Status Flag2

	        /* TODO: 填充 Block Data（16 bytes / block） */
	        memset(&rsp[4], 0x00, 16);

	        PN532_SendResponse(rsp, rsp[0],1);
	        break;
	    }

	    case FELICA_CMD_WRITE_WO_ENCRYPTION: {
	        uint8_t rsp[4];

	        rsp[0] = 0x04;      // Length
	        rsp[1] = 0x09;      // Write Response
	        rsp[2] = 0x00;      // Status Flag1
	        rsp[3] = 0x00;      // Status Flag2

	        PN532_SendResponse(rsp, 4,1);
	        break;
	    }

	    case FELICA_CMD_REQUEST_SYSTEM_CODE: {
	        uint8_t rsp[6];

	        rsp[0] = 0x06;      // Length
	        rsp[1] = 0x0D;      // Response Code
	        rsp[2] = 0x01;      // Number of System Codes
	        rsp[3] = 0x88;      // System Code H
	        rsp[4] = 0xb4;      // System Code L
	        rsp[5] = 0x00;      // Padding / optional

	        PN532_SendResponse(rsp, 6,1);
	        break;
	    }
		default:
error:
			status = PN532_STATUS_GENERIC_ERROR;
			PN532_SendResponse(&status, 1,1);
			break;
	}
}

void namco_read_felica(){
	if(Card.type == Card_Type_Felica_AIC){
		uint8_t rsp[46];
		rsp[0] = 0x00;
		rsp[1] = 0x2d;
		rsp[2] = 0x07;
		//IDM
		memcpy(rsp+3, Card.felica_IDm, 8);
		rsp[11] = 0x00;
		rsp[12] = 0x00;
		rsp[13] = 0x02;
		//block 8082 ,actually is IDM
		memcpy(rsp+14, Card.felica_IDm, 8);
		memset(rsp+22, 0, 8);
		rsp[23] = 0x78;
		//block 8080 ,encrypted card serial
		memcpy(rsp+30,Card.block_8000,16);
		PN532_SendResponse(rsp, 46,1);
	}else{
		uint8_t no_card = 0x01;
		PN532_SendResponse(&no_card, 1,1);
	}
}

static PN532_Up_Frame up;

bool PN532_SendResponse(const uint8_t *data, uint8_t data_len,uint8_t ack_flag)
{

    uint8_t checksum = 0;
    uint8_t frame_len;
    uint8_t i;
    uint8_t tmp[64];
    uint8_t ack[6] = {0,0,0xFF,0,0xFF,0};
    /* ---------- 固定头 ---------- */
    up.preamble   = PN532_PREAMBLE;
    up.startCode1 = PN532_STARTCODE1;
    up.startCode2 = PN532_STARTCODE2;

    /* ---------- LEN / LCS ---------- */
    up.len = 2 + data_len;              // TFI + CMD + DATA
    up.lcs = (uint8_t)(0x100 - up.len);

    /* ---------- TFI / CMD ---------- */
    up.tfi = PN532_TFI_RESPONSE;
    up.cmd = down_frame.cmd + 1;

    /* ---------- DATA ---------- */
    if (data_len && data) {
        memcpy(up.data, data, data_len);
    }

    /* ---------- DCS ---------- */
    checksum = up.tfi + up.cmd;
    for (i = 0; i < data_len; i++) {
        checksum += up.data[i];
    }
    up.dcs = (uint8_t)(0x100 - checksum);

    /* ---------- POSTAMBLE ---------- */
    up.postamble = PN532_POSTAMBLE;

    /* ---------- 正确帧长度 ---------- */
    frame_len = 9 + data_len;

    /* ---------- 发送 ---------- */
    if(ack_flag == 1){
    	//send cmd with ack
    	memcpy(tmp,ack,6);
    	memcpy(tmp+6,up.head,7);
		memcpy(tmp+7+6,data,data_len);
		tmp[6 + 7 + data_len] = up.dcs;
		tmp[6 + 7 + data_len + 1] = up.postamble;
		return Interface_Send(tmp, frame_len+6);
    }else if(ack_flag == 2){
    	//only send ack
    	return Interface_Send(ack, 6);
    }else{
    	//send cmd without ack
        memcpy(tmp,up.head,7);
        memcpy(tmp+7,data,data_len);
        tmp[7 + data_len] = up.dcs;
        tmp[7 + data_len + 1] = up.postamble;
        return Interface_Send(tmp, frame_len);
    }
}

void pn532_rsp_list_passive_target_type_a(void)
{
	if(Card.type == Card_Type_Mifare_Classic){
	    uint8_t rsp[16];

	    rsp[0] = 0x01;        // 1 target
	    rsp[1] = 0x01;        // Target number
	    rsp[2] = 0x00;
	    rsp[3] = 0x04;		  // SENS_RES (ATQA)
	    rsp[4] = 0x08;        // SEL_RES (SAK)
	    rsp[5] = 4;

	    rsp[6] = Card.iso14443_uid4[0];
	    rsp[7] = Card.iso14443_uid4[1];
	    rsp[8] = Card.iso14443_uid4[2];
	    rsp[9] = Card.iso14443_uid4[3];

	    PN532_SendResponse(rsp, 6 + 4,1);
	}else{
		//only send ack
		PN532_SendResponse(NULL, 0,2);
	}
}

void pn532_rsp_list_passive_target_felica(void)
{
	if(Card.type == Card_Type_Felica_AIC){
		uint8_t rsp[22];

		rsp[0] = 0x01;    // NbTg
		rsp[1] = 0x01;    // Tg
		rsp[2] = 0x14;
		rsp[3] = 0x01;

		memcpy(rsp+4  ,Card.felica_IDm ,8);
		memcpy(rsp+12 ,Card.felica_PMm ,8);

		/* System Code */
		rsp[20] = 0x88;
		rsp[21] = 0xb4;

		PN532_SendResponse(rsp, 22,1);
	}else{
		//only send ack
		PN532_SendResponse(NULL, 0,2);
	}
}

static inline uint32_t millis(void)
{
    return HAL_GetTick();
}

void namco_led_service(void)
{
    uint32_t now = millis();

    switch (led.mode)
    {
    case LED_MODE_OFF:
        LED_show(0, 0, 0);
        break;

    case LED_MODE_BLUE_KEEP:
        LED_show(0, 0, 255);
        break;

    /* ---------- 绿 ↔ 蓝 闪烁 ---------- */
    case LED_MODE_GREEN_BLUE_BLINK:
        if (now - led.last_tick >= 150)
        {
            led.last_tick = now;
            led.sub_state ^= 1;
        }

        if (led.sub_state)
            LED_show(0, 0, 255);
        else
            LED_show(0, 255, 0);
        break;

    /* ---------- 红 ↔ 黄 闪烁 ---------- */
    case LED_MODE_RED_YELLOW_BLINK:
        if (now - led.last_tick >= 500)
        {
            led.last_tick = now;
            led.sub_state ^= 1;
        }

        if (led.sub_state)
            LED_show(255, 0, 0);
        else
            LED_show(255, 255, 0);
        break;

    /* ---------- 蓝色呼吸 ---------- */
    case LED_MODE_BLUE_BREATH:
    {
        uint32_t t = (now - led.last_tick) % 1000;
        uint8_t level;

        if (t < 500)
            level = (t * 255) / 500;
        else
            level = 255 - ((t - 500) * 255) / 500;

        LED_show(0, 0, level);
        break;
    }

    /* ---------- RGB 循环 ---------- */
    case LED_MODE_RGB_LOOP:
        if (now - led.last_tick >= 1000)
        {
            led.last_tick = now;
            led.sub_state = (led.sub_state + 1) % 3;
        }

        if (led.sub_state == 0)      LED_show(255, 0, 0);
        else if (led.sub_state == 1) LED_show(0, 255, 0);
        else                         LED_show(0, 0, 255);
        break;

    /* ---------- 绿 → 蓝 渐变 ---------- */
    case LED_MODE_GREEN_TO_BLUE:
        if (now - led.last_tick >= 30 && led.level > 0)
        {
            led.last_tick = now;
            led.level -= 25;
        }

        LED_show(0, led.level, 255 - led.level);
        break;

    /* ---------- 红 → 蓝 渐变 ---------- */
    case LED_MODE_RED_BLUE_KEEP:
        if (now - led.last_tick >= 30 && led.level > 0)
        {
            led.last_tick = now;
            led.level -= 25;
        }

        LED_show(led.level, 0, 255 - led.level);
        break;

    /* ---------- 红色渐灭 ---------- */
    case LED_MODE_RED_FALL:
        if (now - led.last_tick >= 50)
        {
            led.last_tick = now;

            if (led.level > 0)
                led.level -= 5;
            else
                led.mode = LED_MODE_OFF;
        }

        LED_show(led.level, 0, 0);
        break;

    default:
        break;
    }
}

void led_set_mode(LedMode_t mode)
{
    led.mode      = mode;
    led.last_tick = millis();
    led.level     = 255;
    led.sub_state = 0;
}

void setLEDMode(uint8_t mode)
{
    switch (mode)
    {
    case 0x11: led_set_mode(LED_MODE_GREEN_TO_BLUE); break;
    case 0x16: led_set_mode(LED_MODE_RED_BLUE_KEEP); break;
    case 0x0c: led_set_mode(LED_MODE_GREEN_BLUE_BLINK); break;
    case 0x00: led_set_mode(LED_MODE_OFF); break;
    case 0x05: led_set_mode(LED_MODE_BLUE_BREATH); break;
    case 0x0b: led_set_mode(LED_MODE_RGB_LOOP); break;
    case 0x08: led_set_mode(LED_MODE_RED_YELLOW_BLINK); break;
    case 0x1b: led_set_mode(LED_MODE_BLUE_KEEP); break;
    default: break;
    }
}

