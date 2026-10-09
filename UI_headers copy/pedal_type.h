/*
pedals_type.h
Shared data types for the system
Contains effect types, parameters, and preset definitions
*/


#ifndef PEDAL_TYPE_H
#define PEDAL_TYPE_H
#include <stdint.h>


/*Audio effects supported by the pedal*/
typedef enum{
   E_BYPASS,
   E_DISTORTION,
   E_REVERB,
   E_DELAY,
   E_PITCH,
   E_WAH
} effect;


/*Distortion effect parameters*/
typedef struct{
   bool enabled;
   float gain;
   float tone;
   float level;
} distortion_params;


/*Reverb effect Parameters*/
typedef struct{
   bool enabled;
   float mix;
   float decay;
   float tone;
} reverb_params;


/*Delay effect parameters*/
typedef struct{
   bool enabled;
   float delay_ms;
   float feedback;
   float mix;
} delay_params;


/*Shift effect parameters*/
typedef struct{
   bool enabled;
   float shift;
} pitch_params;


/*Wah effect parameters*/
typedef struct{
   bool enabled;
   float frequency;
   float resonance;
} wah_params;


/*Preset struct -> Ex. have 3 tabs and have 3 effects in preset -> each tab can activate one of the effects*/
typedef struct{
   uint8_t preset_id; /*preset identifier if we have multiple presets*/


   distortion_params distortion;
   reverb_params reverb;
   delay_params delay;
   pitch_params pitch;
   wah_params wah;
} preset;


#endif
