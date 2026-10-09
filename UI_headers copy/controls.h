/*
controls.h


High level interface for physical user controls (buttons, encoder)
Hides GPIO, interrupts, and low level implementation
*/
#ifndef CONTROLS_H
#define CONTROLS_H


#include <stdbool.h>


/*Physical buttons on the module*/
typedef enum{
   BACK_BUTTON,
   SAVE_BUTTON,
   MENU_BUTTON,
   TAP_BUTTON,
   TUNER_BUTTON,
   TAB_1,
   TAB_2,
   TAB_3
} button;


/*Button events*/
typedef enum{
   BUTTON_PRESSED,
   BUTTON_HELD
} button_event;


/*Initialize buttons(including tabs), encoder, interrupts*/
void controls_init();


/*Check if a certain button is pressed/held*/
bool get_button_event(button btn, button_event event);


/*Return encoder movement since last read. Positive = clockwise, negative = CCW*/
int get_encoder_movement();


/*Returns true if encoder is pressed since last time it was checked*/
bool encoder_pressed();
#endif CONTROLS_H
