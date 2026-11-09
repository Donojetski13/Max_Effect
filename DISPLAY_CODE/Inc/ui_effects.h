/*
 * ui.effects.h
 *
 *  Created on: Oct 12, 2026
 *      Author: ewan
 */

#ifndef UI_EFFECTS
#define UI_EFFECTS

#include "lvgl.h"
#include "ui.h"
#include <stdbool.h>

typedef enum{
	EFFECT_MENU_DISTORTION,
	EFFECT_MENU_REVERB,
	EFFECT_MENU_DELAY,
	EFFECT_MENU_PITCH,
	EFFECT_MENU_WAH,
	EFFECT_MENU_BACK
} EffectMenu_t;

//Init effects menu
void UI_Effects_Init(void);

//effects.c screens
lv_obj_t *UI_Effects_GetScreen(void);

//distortion
lv_obj_t *UI_Distortion_GetScreen(void);
void UI_Distortion_Scroll(MenuDirection_t direction); //navigation
bool UI_Distortion_Enter(void); //back button

//navigation
void UI_Effects_Scroll(MenuDirection_t direction);
EffectMenu_t UI_Effects_GetSelection(void);

void UI_Distortion_SetParam(uint8_t index, int32_t value);

//Wah screen
lv_obj_t *UI_Wah_GetScreen(void);
void UI_Wah_Scroll(MenuDirection_t direction);
bool UI_Wah_Enter(void);
void UI_Wah_GetSettings(float *frequency, bool *enabled);


#endif /* SRC_UI_EFFECTS_H_ */
