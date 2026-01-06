/*
 * flash.c
 *
 *  Created on: Jul 18, 2025
 *      Author: Qinh
 */

#ifndef SRC_FLASH_C_
#define SRC_FLASH_C_

#include <stdint.h>
#include <string.h>
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_flash.h"
#include "stm32f0xx_hal_flash_ex.h"
#include "flash.h"
#include "stm32f0xx_hal.h"

#define TARGET_OFFSET 252  // 使用页的最后16字节(252~255字)
#define LAST_PAGE_ADDR 0x0800FC00

FlashData Flash;

const uint8_t default_setting[5] = {0x40,0xff,0xfe,0x7f,0xfe};

void flash_write(uint32_t data[4]) {
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = LAST_PAGE_ADDR,
        .NbPages = 1
    };
    uint32_t error;
    HAL_FLASHEx_Erase(&erase, &error);
    for(int i=0; i<4; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                         LAST_PAGE_ADDR + (TARGET_OFFSET+i)*4,
                         data[i]);
    }
    HAL_FLASH_Lock();
}

void flash_read(uint8_t* data){
	memcpy(data, (void*)(LAST_PAGE_ADDR + TARGET_OFFSET * sizeof(uint32_t)) , 16);
}

#endif /* SRC_FLASH_C_ */
