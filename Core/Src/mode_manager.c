/*
 * mode_manager.c
 *
 *  Created on: Jun 26, 2025
 *      Author: Qinh
 */
#include "mode_manager.h"
#include "mode_sega_serial.h"
#include "usbd_hid_custom_if.h"
#include "usbd_cdc_acm_if.h"
#include "mode_spice_api.h"
#include "mode_namco_serial.h"
#include "mode_aimeio.h"
#include "LED.h"
#include <stdarg.h>

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern USBD_HandleTypeDef hUsbDevice;
extern TIM_HandleTypeDef htim17;


extern uint8_t spice_mode_detect_flag;
extern uint8_t dfu_flag;
Machine Reader;
uint8_t UART_FrameError = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM17) {
    	if(Reader.Current_Mode == MODE_SPICE_API){
    		spice_request(Flash.spice_setting &SYSTEM_MODE_SEETING);
    		return;
    	}
    	if(Reader.Current_Mode == MODE_NAMCO_SERIAL){
    		namco_led_service();
    	}
    }
}

void Mode_Poll(){
	if(UART_FrameError > 1){
		uint32_t baud = huart1.Init.BaudRate;
		switch (baud){
			case 115200:
				__HAL_UART_DISABLE(&huart1);
				huart1.Init.BaudRate = 38400;
				if (HAL_UART_Init(&huart1) != HAL_OK){
					Error_Handler();
				}
				__HAL_UART_ENABLE(&huart1);
				HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Reader.Uart_Buffer_Receive, 255);
				__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
				break;
			default:{
				__HAL_UART_DISABLE(&huart1);
				huart1.Init.BaudRate = 115200;
				if (HAL_UART_Init(&huart1) != HAL_OK){
					Error_Handler();
				}
				__HAL_UART_ENABLE(&huart1);
				HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Reader.Uart_Buffer_Receive, 255);
				__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
				break;
			}
		}
		UART_FrameError = 0;
	}
	if(Reader.Current_Mode != MODE_IDLE){
		return;
	}
	if(Reader.Current_Interface == INTERFACE_NONE){
		Reader.Current_Interface = INTERFACE_NONE;
	}
}

uint8_t Mode_Detect(uint8_t* data,uint8_t len){
	uint8_t test;
	test = sega_packet_check(data,len);
	if(test){
		Sega_Mode_Loop(test);
		return MODE_SEGA_SERIAL;
	}
	test = spice_request_check(data,len);
	if(test){
		HAL_TIM_Base_Start_IT(&htim17);
		return MODE_SPICE_API;
	}
	test = namco_packet_check(data,len);
	if(test){
		namco_packet_process(test);
		HAL_TIM_Base_Start_IT(&htim17);
		return MODE_NAMCO_SERIAL;
	}
	test = AimeIO_packet_check(data,len);
	if(test){
		AimeIO_process(test);
		return MODE_AIME_IO;
	}
	return MODE_IDLE;
}

void Packet_process(uint8_t* data, uint8_t len){
	switch(Reader.Current_Mode){
		case MODE_IDLE:{
			Reader.Current_Mode = Mode_Detect(data,len);
			break;
		}
		case MODE_SEGA_SERIAL:{
			uint8_t ret = sega_packet_check(data,len);
			if((ret == 0) || (ret == STATUS_SUM_ERROR)){
				goto error;
			}else{
				Sega_Mode_Loop(ret);
			}
			break;
		}
		case MODE_SPICE_API:{
			if(!spice_request_check(data,len)){
				goto error;
			}
			break;
		}
		case MODE_NAMCO_SERIAL:{
			if(!namco_packet_process(namco_packet_check(data,len))){
				goto error;
			}
			break;
		}
		default:{
			goto error;
			break;
		}
	}
	return;
	error:
	Mode_Detect(data,len);
}

void Reader_UART_Init(){
	uint8_t Uart_Parameter = Flash.system_setting & 0b1111;
	switch (Uart_Parameter){
		case 0:
			break;
		case 1:{
			__HAL_UART_DISABLE(&huart1);
			huart1.Init.BaudRate = 38400;
			if (HAL_UART_Init(&huart1) != HAL_OK){
				Error_Handler();
			}
			__HAL_UART_ENABLE(&huart1);
			break;
		}
		case 2:{
			__HAL_UART_DISABLE(&huart1);
			huart1.Init.BaudRate = 9600;
			if (HAL_UART_Init(&huart1) != HAL_OK){
				Error_Handler();
			}
			__HAL_UART_ENABLE(&huart1);
			break;
		}
	}
	while(HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Reader.Uart_Buffer_Receive, 255) != HAL_OK);
	__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if ((huart->Instance == USART1) && (HAL_UARTEx_GetRxEventType(huart) == 2))
    {
    	platformLedToogle(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
        Reader_UART_IRQHandler(Size);
		while(HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Reader.Uart_Buffer_Receive, 255) != HAL_OK);
        __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
    	platformLedOn(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
        if (huart->ErrorCode & HAL_UART_ERROR_FE)
        {
        	UART_FrameError++;
        }
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_OREFLAG(huart);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Reader.Uart_Buffer_Receive, 255);
         __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
    }
}


void Reader_UART_IRQHandler(uint16_t Size){
	if(Reader.Current_Interface == INTERFACE_NONE){
		Reader.Current_Interface = INTERFACE_UART;
	}
	if(Reader.Current_Interface == INTERFACE_UART){
		Packet_process(Reader.Uart_Buffer_Receive,Size);
	}
	if(Reader.Current_Mode == MODE_IDLE){
		Reader.Current_Interface = INTERFACE_NONE;
	}
}

void Reader_CDC_IRQHandler(uint8_t* data, uint8_t len){
	if(Reader.Current_Interface == MODE_IDLE){
		Reader.Current_Interface = INTERFACE_CDC;
	}
	if(Reader.Current_Interface == INTERFACE_CDC){
		Packet_process(data,len);
	}
	if(Reader.Current_Mode == MODE_IDLE){
		Reader.Current_Interface = INTERFACE_NONE;
	}
}

void Reader_HID_IRQHandler(uint8_t* data){
	if(data[0] == 0){
		return;
	}else if(data[0] == 3){
		Reader.Current_Interface = INTERFACE_HID;
		Reader.Current_Mode = MODE_CARD_IO;
		LED_show(data[1],data[2],data[3]);
	}else if(data[0] == 5){
		if(AimeIO_packet_check(data+1,32) != 0){
			Reader.Current_Interface = INTERFACE_HID;
			Reader.Current_Mode = MODE_AIME_IO;
			AimeIO_process(AimeIO_packet_check(data+1,32));
		}
	}
}

void Reader_Uart_SendCommand(uint8_t* data, uint8_t len){
	HAL_UART_Transmit_DMA(&huart1, data, len);
}

void Reader_CDC_SendCommand(uint8_t* data, uint8_t len){
	if(len == 64 && len == 128){
		len--;
	}
	CDC_Transmit(0, data, len);
}

void Reader_HID_SendReport(uint8_t* data, uint8_t len){
	USBD_CUSTOM_HID_SendReport(&hUsbDevice, data, len);
}

bool Interface_Send(const uint8_t* data ,uint8_t len){
	UART_FrameError = 0;
	switch(Reader.Current_Interface){
		case INTERFACE_CDC:
			Reader_CDC_SendCommand(data,len);
			break;
		case INTERFACE_UART:
			Reader_Uart_SendCommand(data,len);
			break;
		case INTERFACE_HID:
			USBD_CUSTOM_HID_SendReport(&hUsbDevice, data, len);
			break;
		default:
			return false;
	}
	return true;
}
