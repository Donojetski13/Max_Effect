/*
pedal_effect.h
Effect suite based on pedal types, parameters, and preset definitions for the system
*/

#ifndef PEDAL_EFFECT_H
#define PEDAL_EFFECT_H
#include <stdint.h>
#include "../FFT-implementation-in-C/include/fft_common.h"
#include "../FFT-implementation-in-C/include/fft_algorithms.h"
#include "pedal_type.h"


// Functions that apply specific pedal effects to an input signal
void apply_bypass(complex_t* signal, int n);
void apply_distortion(complex_t* signal, int n, double gain);
void apply_reverb(complex_t* signal, int n, double reverb_time);
void apply_delay(complex_t* signal, int n, double delay_time, double feedback);
void apply_pitch_shift(complex_t* signal, int n, double shift_amount);
void apply_wah(complex_t* signal, int n, double wah_frequency);

// Functions to modify the effect chain
void add_effect(effect e, params p);
void remove_effect();

// Function to apply a chain of effects based on chain parameters, array of effects, and their parameters
void apply_effect_chain(complex_t* signal, int n);

#endif // PEDAL_EFFECT_H