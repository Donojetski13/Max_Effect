/*
pedals_type.h
Shared data types for the system
Contains effect types, parameters, and preset definitions
*/


#ifndef PEDAL_TYPE_H
#define PEDAL_TYPE_H
#include <stdint.h>
#include <stdbool.h>

const uint8_t NUM_EFFECTS = 5; /*Number of effects supported by the pedal*/
/*Audio effects supported by the pedal*/
typedef enum{
   E_BYPASS,
   E_DISTORTION,
   E_REVERB,
   E_DELAY,
   E_PITCH,
   E_WAH
} effect;

/*Effect parameters. Settings vary by effect type*/
typedef struct{
	bool enable;
	float* settings;
} params;

// Effect chain struct
typedef struct {
    effect effect_type;
    params effect_params;
} effect_pair;

/*Preset struct -> */
typedef struct{
//   uint8_t preset_id; /*preset identifier if we have multiple presets*/
	uint8_t length;		// chain length
	effect_pair* chain;
} Preset;
/*
Distortion effect parameters
   float gain;
   float tone;
   float level;

Reverb effect Parameters
   float mix;
   float decay;
   float tone;

Delay effect parameters
   float delay_ms;
   float feedback;
   float mix;

Shift effect parameters
   float shift;

Wah effect parameters
   float frequency;
*/


#endif
