/*
pedal_effect.h
Effect suite based on pedal types, parameters, and preset definitions for the system
*/

#ifndef PEDAL_EFFECT_H
#define PEDAL_EFFECT_H
#include <stdint.h>
#include "../FFT-implementation-in-C-reduced/include/fft_common.h"
#include "../FFT-implementation-in-C-reduced/include/fft_algorithms.h"
#include "pedal_type.h"
#include "audio_codec.h"

#define Fs 46875.0  // Hz
#define MIN_FREQ 40.0
#define MAX_FREQ 2000.0
#define WAH_B 40.0

// Functions that apply specific pedal effects to an input signal
// signal quality untouched. can consider signal strengthing with gain if signals get weak
void apply_bypass(int16_t* signal, int n){
	float32_t gain = 1;
	for (uint16_t i = 0; i < n; ++i) {
		signal[i] *= gain;
	}
}

//
void apply_distortion(int16_t* signal, int n, params p){
	;
}

//
void apply_reverb(int16_t* signal, int n, params p){
	;
}

//
void apply_delay(int16_t* signal, int n, params p){
	;
}

//
void apply_pitch_shift(int16_t* signal, int n, params p){
	;
}

// shift bandpass pass filter of WAH_B window length between min/max frequency
// assumes parameter setting is 0 - 100
void apply_wah(int16_t* signal, int n, params p){
	static float32_t last_freqSet = 0;
	if (p.enable) {
		uint8_t freqset = p.settings[0];

		if (freqset != last_freqSet) {
			float32_t freq = ((MAX_FREQ-WAH_B-MIN_FREQ)*.01)*freqset + MIN_FREQ;
			generate_fir_coefficients(firCoeffsBuffer,NUM_TAPS,FILTER_BANDPASS,  freq, freq+WAH_B, Fs);
		}
		Process_DSP_Block(&signal[0], &signal[0]);
	}
	// Bypass if not enabled
}

// Functions to modify the effect chain
//append an effect to the chain with its parameters & return the new length of the chain
uint8_t insert_effect(effect e, params p, effect_pair* effect_chain, uint8_t loc, uint8_t chain_length){
	if (chain_length == NUM_EFFECTS) return chain_length; // at max size already
	loc = (loc > chain_length)? chain_length: loc;

	// shift loop right
	for (uint8_t i = chain_length-1; i>= loc; --i) {
		effect_chain[i+1] = effect_chain[i];
	}

	// add effect
	effect_chain[loc].effect_type = e;
	effect_chain[loc].effect_params = p;

	return ++chain_length;
}

//append an effect to the chain with its parameters & return the new length of the chain
uint8_t append_effect(effect e, params p, effect_pair* effect_chain, uint8_t chain_length){
	return insert_effect(e, p, effect_chain, chain_length,chain_length);
}

// remove effect from location in the chain and return the new length of the chain
uint8_t remove_effect(effect_pair* effect_chain, uint8_t loc, uint8_t chain_length) {
	if (chain_length == 0) return 0; // at min size already
	loc = (loc >= chain_length)? chain_length-1: loc;

	//shift loop left
	for (uint8_t i = loc; i< chain_length-1; ++i) {
		effect_chain[i] = effect_chain[i+1];
	}

	return --chain_length;
}

// update effect from the chain. put e & p into chain at loc
void update_effect(effect e, params p, effect_pair* effect_chain, uint8_t loc){
	effect_chain[loc].effect_type = e;
	effect_chain[loc].effect_params = p;
}

// toggle effect on/off from the chain at loc
void toggle_effect(effect_pair* effect_chain, uint8_t loc){
	effect_chain[loc].effect_params.enable = !effect_chain[loc].effect_params.enable;
}

// Function to apply a chain of effects based on chain parameters, array of effects, and their parameters
void apply_effect_chain(int16_t* signal, uint16_t n, effect_pair* effect_chain, uint8_t chain_length) {
    // FFT to frequency domain
    // Apply each effect in the chain
    for (int i = 0; i < chain_length; i++) {
        effect e = effect_chain[i].effect_type;
        params p = effect_chain[i].effect_params;
        switch (e) {
            case E_BYPASS:
                apply_bypass(signal, n);
                break;
            case E_DISTORTION:
                apply_distortion(signal, n, p);
                break;
            case E_REVERB:
                apply_reverb(signal, n, p);
                break;
            case E_DELAY:
                apply_delay(signal, n, p);
                break;
            case E_PITCH:
                apply_pitch_shift(signal, n, p);
                break;
            case E_WAH:
                apply_wah(signal, n, p);
                break;
        }
    }
}

// save a preset
void save_preset(Preset* presets, uint8_t loc, effect_pair* effect_chain, uint8_t chain_length){
	presets[loc].chain = effect_chain;
	presets[loc].length = chain_length;
}
// load a preset return new chain length
uint8_t load_preset(Preset* presets, uint8_t loc, effect_pair* effect_chain){
	effect_chain = presets[loc].chain;
	return presets[loc].length;
}
#endif // PEDAL_EFFECT_H
/*
 * // FFT to frequency domain
 *     radix2_dif_fft(signal, n, FFT_FORWARD);
 *     // Inverse FFT to time domain
    radix2_dif_fft(signal, n, FFT_INVERSE);
 */
