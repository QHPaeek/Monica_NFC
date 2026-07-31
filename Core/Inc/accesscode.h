/*
 * accesscode.h
 *
 *  Created on: Jul 31, 2026
 *      Author: Qinh
 */

#ifndef INC_ACCESSCODE_H_
#define INC_ACCESSCODE_H_

void decrypt_accesscode(uint8_t *spad);
void ascii_to_accesscode(uint8_t *ascii_data ,uint8_t *accesscode);
void hex_to_accesscode(uint8_t *hex ,uint8_t *accesscode);
void idm_to_accesscode(const uint8_t uid[8], uint8_t out[16]);

#endif /* INC_ACCESSCODE_H_ */
