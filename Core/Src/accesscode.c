/*
 * accesscode.c
 *
 *  Created on: Jul 31, 2026
 *      Author: Qinh
 */
#include "sbox.h"
#include "Card_Reader.h"

#define N_TABLES 8
#define ITER_ADD 0x05

/**
 * 使用 amdaemon.exe 中的 S_BOX_INV 表解密 Amusement IC SPAD0 数据
 *
 * @param spad  [in/out] 16 字节 SPAD0 加密数据，原地解密
 *
 * 注意：amdaemon.exe 内部实现中参数字节使用 spad[0] 而非 spad[15]。
 * 如果你的数据来自 amdaemon 内部格式，将下面的 spad[15] 改为 spad[0]。
 * 如果你的数据是标准 FeliCa SPAD0 格式（最后字节为参数），保持 spad[15]。
 */
void decrypt_accesscode(uint8_t *spad)
{
    uint8_t i;

    /* 第一步：S_BOX_INV[8] 逆掩码，覆盖全部 16 字节 */
    for (i = 0; i < 16; i++) {
        spad[i] = S_BOX_INV[N_TABLES][spad[i]];
    }

    /* 第二步：从参数字节提取迭代参数
     *   标准规范：spad[15]
     *   amdaemon 内部：spad[0]（见上方注释）  */
    uint8_t count = (spad[15] >> 4) + 7;
    uint8_t table = spad[15] + ITER_ADD * count;

    /* 第三步：逆序迭代（解密方向） */
    for (uint8_t iter = 0; iter < count; iter++) {
        table -= ITER_ADD;

        /* rotate_right 前 15 字节，5 位 */
        {
            uint8_t prior = spad[14];
            for (i = 0; i < 15; i++) {
                uint8_t temp = spad[i];
                spad[i] = (spad[i] >> 5) | ((prior & 0x1F) << 3);
                prior = temp;
            }
        }

        /* S_BOX_INV[table % 8] 替换前 15 字节 */
        for (i = 0; i < 15; i++) {
            spad[i] = S_BOX_INV[table % N_TABLES][spad[i]];
        }
    }
    /*如果前6字节都是0，就使用IDM转换AccessCode*/
    if((spad[0] | spad[1] | spad[2] | spad[3] | spad[4] | spad[5])  != 0){
		memset(spad,0,6);
		idm_to_accesscode(Card.felica_IDm,spad+6);
	}
}

void ascii_to_accesscode(uint8_t *ascii_data ,uint8_t *accesscode){
	accesscode[0] = 0x00;
	accesscode[1] = 0x00;
	for(uint8_t i = 0;i<8;i++){
		accesscode[i + 2] = (ascii_data[2 * i] - 0x30) << 4 | (ascii_data[2 * i + 1] - 0x30);
	}
}

void hex_to_accesscode(uint8_t *hex ,uint8_t *accesscode){
	accesscode[0] = 0x00;
	accesscode[1] = 0x00;
	memcpy(accesscode + 2, hex, 8);
	for(uint8_t i = 0;i<8;i++){
		uint8_t tmp_high = (hex[i] >> 4);
		uint8_t tmp_low = (hex[i] & 0b1111);
		if(tmp_high >= 10){
			tmp_high -= 7;
		}
		if(tmp_low >= 10){
			tmp_low -= 7;
		}
		accesscode[i+2] = tmp_high << 4 | tmp_low;
	}
}

void idm_to_accesscode(const uint8_t uid[8], uint8_t out[16])
{
    uint64_t val = 0;

    for (int i = 0; i < 8; i++) {
        val = (val << 8) | uid[i];
    }

    for (int i = 0; i < 6; i++) {
        out[i] = 0x00;
    }

    for (int i = 0; i < 10; i++) {
        uint8_t lo = (uint8_t)(val % 10);
        val /= 10;
        uint8_t hi = (uint8_t)(val % 10);
        val /= 10;
        out[15 - i] = (uint8_t)((hi << 4) | lo);
    }
}
