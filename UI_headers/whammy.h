/*
whammy.h


High level interface for digital whammy bar
*/


#ifndef WHAMMY_H
#define WHAMMY_H


#include <stdbool.h>


/*Initialize whammy bar and any sensor/hardware related to it*/
void whammy_init();


/*Returns whammy position*/
float whammy_get_position();


/*Used if need to calibrate the bar*/
void whammy_calibrate();


#endif WHAMMY_H
