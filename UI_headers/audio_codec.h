/*
audio_codec.h


High-level interface for the audio codec.
The rest of the system should use these functions instead
of trying to directly access codec registers
*/


#ifndef AUDIO_CODEC_H
#define AUDIO_CODEC_H


#include <stdint.h>
#include <stdbool.h>


/*Initialize codec and configure ADC/DAC, sample rate, interface*/
/*Returns TRUE if successful*/
bool codec_init();


/*starts audio i/o through codec*/
void codec_start();


/*stop audio i/o through codec*/
void codec_stop();


/*Set analog input gain from guitar*/
void codec_set_intput_gain(float gain);
#endif
