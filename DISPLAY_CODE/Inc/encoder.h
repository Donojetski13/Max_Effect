/*
 * encoder.h
 *
 *  Created on: Oct 12, 2026
 *      Author: ewan
 */

#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

typedef enum{
	ENCODER_NONE,
	ENCODER_CW,
	ENCODER_CCW,
	ENCODER_PRESSED
} EncoderEvent_t;

void Encoder_Init(void);

EncoderEvent_t Encoder_GetEvent(void);

void Encoder_ButtonIRQ(void);


#endif /* INC_ENCODER_H_ */
