/*
 * mode_spice_api.h
 *
 *  Created on: Jul 28, 2026
 *      Author: Qinh
 */

#ifndef INC_MODE_SPICE_API_H_
#define INC_MODE_SPICE_API_H_

#include <stdint.h>

void spice_cardio_send_14443(uint8_t *uid);
void spice_iso14443_process();
void spice_cardio_send(uint8_t *uid);
void spice_felice_process();
void spice_icode_process();
uint8_t spice_request_check(uint8_t* data,uint8_t len);

#endif /* INC_MODE_SPICE_API_H_ */
