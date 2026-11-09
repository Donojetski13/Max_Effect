/*
 * ui.h
 *
 *  Created on: Oct 12, 2026
 *      Author: ewan
 */

#ifndef UI_H
#define UI_H

#include "lvgl.h"

typedef enum{
	SCREEN_MAIN,
	SCREEN_EFFECTS,
	SCREEN_DISTORTION,
	SCREEN_WAH,
	SCREEN_LOOPER,
	SCREEN_PRESETS,
	SCREEN_TUNER
} Screen_t;

typedef enum{
	MENU_EFFECTS,
	MENU_LOOPS,
	MENU_PRESETS,
	MENU_TUNER
} Menu_t;

typedef enum{
	MENU_UP,
	MENU_DOWN
} MenuDirection_t;

//UI Init
void UI_Init(void);

//Screen Functions
void UI_MainMenu_Init(void);

void UI_LoopMenu_Init(void);
void UI_PresetsMenu_Init(void);

//Navigation
void UI_Menu_Scroll(MenuDirection_t direction);
void UI_SetScreen(Screen_t screen);
void UI_Menu_Enter(void);

#endif /* INC_UI_H_ */
