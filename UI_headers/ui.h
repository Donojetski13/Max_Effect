/*
ui.h
High-level interface for LCD UI
The acutal implementation may use LVGL, but the rest of the system
only needs to use these functions.
*/
#ifndef UI_H
#define UI_H


#include <stdint.h>
#include <stdbool.h>
#include "pedal_type.h"


typedef enum{
   EFFECT,
   LOOPER,
   PRESET,
   SETTINGS,
   TUNING
} LCD_menu; /* The main menus/screens that can be shown on the LCD*/


/*Init the LCD UI, LVGL, default screen*/
void ui_init();


/*Changes the current menu screen, the implementation will hide the old menu*/
void ui_set_menu(LCD_menu menu);


/*Select which effect is currently being viewed/edited*/
void ui_set_effect(effect selected_effect);


/*Show if an effect is enabled/disabled*/
void ui_set_effect_enabled(effect selected_effect, bool enabled);


/*Updates on parameter for effect*/
void ui_set_distortion_params(const distortion_params *params);
void ui_set_reverb_params(const reverb_params *params);
void ui_set_delay_params(const delay_params *params);
void ui_set_pitch_params(const pitch_params *params);
void ui_set_wah_params(const wah_params *params);


/*updates the currently selected preset on the LCD*/
void ui_set_preset(uint8_t preset_id);


/*updates whether the loop is recording, playing, or stopped*/
void ui_set_loop_state(bool recording, bool playing);


/*performs periodic UI processing/display updates*/
void ui_update();


#endif
