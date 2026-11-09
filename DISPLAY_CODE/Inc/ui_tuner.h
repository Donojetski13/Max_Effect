/*
 * ui_tuner.h
 *
 *  Created on: Nov 8, 2026
 *      Author: ewan
 */

#ifndef INC_UI_TUNER_H_
#define INC_UI_TUNER_H_
#include "lvgl.h"

void UI_Tuner_Init(void);
lv_obj_t *UI_Tuner_GetScreen(void);
void UI_Tuner_SetFrequency(double freq);


#endif /* INC_UI_TUNER_H_ */
