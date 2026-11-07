/*
filters.h
Filter implementations for the audio processing system
*/

#ifndef FILTERS_H
#define FILTERS_H
#include <stdint.h>
#include <math.h>
#include "../FFT-implementation-in-C-reduced/include/fft_common.h"
#include "arm_math.h"

//#define CHUNKS
#define FIR
//#define IIR
// Filter types

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

typedef enum {
    FILTER_LOWPASS,
    FILTER_HIGHPASS,
    FILTER_BANDPASS,
    FILTER_BANDSTOP
} filter_type_t;

#ifdef CHUNKS
// Apply frequency domain filter. signal needs to be in frequency domain (after FFT)
void filter(complex_t* spectrum, int n, double sample_rate,filter_type_t type,
                double freq1, double freq2) {

    // Apply filter
    for (int k = 0; k < n; k++) {
        // Negative frequencies for k > n/2
        double freq = (k > n/2) ? (n - k) * sample_rate / n : k * sample_rate / n;

        double gain;

        switch (type) {
            case FILTER_LOWPASS:
                gain = (freq <= freq1) ? 1.0 : 0.0;
                break;

            case FILTER_HIGHPASS:
                gain = (freq >= freq1) ? 1.0 : 0.0;
                break;

            case FILTER_BANDPASS:
                gain = (freq >= freq1 && freq <= freq2) ? 1.0 : 0.0;
                break;

            case FILTER_BANDSTOP:
                gain = (freq < freq1 || freq > freq2) ? 1.0 : 0.0;
                break;

            default: // no filter (faster to just not call this function)
                gain = 1.0;
        }
        spectrum[k] *= gain;
    }
}

// Smooth filter transitions to reduce ringing
void transition_filter(complex_t* spectrum, int n, double sample_rate, filter_type_t type,
                         double freq1, double freq2, double transition_width) {
    for (int k = 0; k < n; k++) {
        // Negative frequencies for k > n/2
        double freq = (k > n/2) ? (n - k) * sample_rate / n : k * sample_rate / n;

        double gain;
        switch (type) {
            case FILTER_LOWPASS:
                if (freq < freq1 - transition_width/2) {
                    gain = 1.0;
                } else if (freq > freq1 + transition_width/2) {
                    gain = 0.0;
                } else {
                    // Smooth transition using raised cosine
                    double t = (freq - freq1 + transition_width/2) / transition_width;
                    gain = 0.5 * (1 + cos(PI * t));
                }
                break;
            case FILTER_HIGHPASS:
                if (freq < freq1 - transition_width/2) {
                    gain = 0.0;
                } else if (freq > freq1 + transition_width/2) {
                    gain = 1.0;
                } else {
                    // Smooth transition using raised cosine
                    double t = (freq - freq1 + transition_width/2) / transition_width;
                    gain = 0.5 * (1 + cos(PI * t));
                }
                break;
            case FILTER_BANDPASS:
                if (freq < freq1 - transition_width/2 || freq > freq2 + transition_width/2) {
                    gain = 0.0;
                } else if (freq >= freq1 + transition_width/2 && freq <= freq2 - transition_width/2) {
                    gain = 1.0;
                } else {
                    // Smooth transition using raised cosine
                    double t = (freq - freq1 + transition_width/2) / transition_width;
                    gain = 0.5 * (1 + cos(PI * t));
                }
                break;
            case FILTER_BANDSTOP:
                if (freq < freq1 - transition_width/2 || freq > freq2 + transition_width/2) {
                    gain = 1.0;
                } else if (freq >= freq1 + transition_width/2 && freq <= freq2 - transition_width/2) {
                    gain = 0.0;
                } else {
                    // Smooth transition using raised cosine
                    double t = (freq - freq1 + transition_width/2) / transition_width;
                    gain = 0.5 * (1 + cos(PI * t));
                }
                break;
            default: // no filter (faster to just not call this function)
                gain = 1.0;
        }
        spectrum[k] *= gain;
    }
}
#endif

#ifdef FIR
/**
 * @brief Generates FIR coefficients at runtime for CMSIS-DSP.
 * @note  CMSIS-DSP expects coefficients in TIME-REVERSED order.
 *        This function automatically handles the time-reversal.
 */
void generate_fir_coefficients(float32_t *pCoeffs, uint16_t numTaps, filter_type_t type,
                               float32_t cutoff1_hz, float32_t cutoff2_hz, float32_t fs)
{
    int M = numTaps - 1; // Filter Order
    float32_t f_c1 = cutoff1_hz / fs;
    float32_t f_c2 = cutoff2_hz / fs;

    // Temporary buffer to hold regular forward coefficients
    float32_t tmp[numTaps];

    for (int n = 0; n < numTaps; n++) {
        // Ideal impulse response calculation
        float32_t ideal = 0.0f;
//        float32_t arg = 2.0f * M_PI * ((float32_t)n - (float32_t)M / 2.0f);

        if (n == M / 2 && (M % 2 == 0)) {
            // Handle the central division-by-zero singularity point
            switch (type) {
                case FILTER_LOWPASS:  ideal = 2.0f * f_c1; break;
                case FILTER_HIGHPASS: ideal = 1.0f - 2.0f * f_c1; break;
                case FILTER_BANDPASS: ideal = 2.0f * (f_c2 - f_c1); break;
                case FILTER_BANDSTOP: ideal = 1.0f - 2.0f * (f_c2 - f_c1); break;
            }
        } else {
            // Standard Sinc math
            float32_t n_offset = (float32_t)n - (float32_t)M / 2.0f;
            switch (type) {
                case FILTER_LOWPASS:
                    ideal = sinf(2.0f * M_PI * f_c1 * n_offset) / (M_PI * n_offset);
                    break;
                case FILTER_HIGHPASS:
                    ideal = -sinf(2.0f * M_PI * f_c1 * n_offset) / (M_PI * n_offset);
                    break;
                case FILTER_BANDPASS:
                    ideal = (sinf(2.0f * M_PI * f_c2 * n_offset) - sinf(2.0f * M_PI * f_c1 * n_offset)) / (M_PI * n_offset);
                    break;
                case FILTER_BANDSTOP:
                    ideal = (sinf(2.0f * M_PI * f_c1 * n_offset) - sinf(2.0f * M_PI * f_c2 * n_offset)) / (M_PI * n_offset);
                    break;
            }
        }

        // Apply a Hamming Window to smooth out ripples
        float32_t window = 0.54f - 0.46f * cosf(2.0f * M_PI * n / M);
        tmp[n] = ideal * window;
    }

    /*
     * CRITICAL STEP FOR CMSIS-DSP:
     * arm_fir_f32 expects the coefficients buffer to be TIME-REVERSED.
     * We populate pCoeffs backwards from our calculated tmp array.
     */
    for (int i = 0; i < numTaps; i++) {
        pCoeffs[i] = tmp[numTaps - 1 - i];
    }
}

#define NUM_TAPS     65     // Must be ODD for highpass/bandstop to function correctly here
#define BLOCK_SIZE   256     // Number of samples processed per batch (based on half buffer frames)

// Memory Allocations (Must be in RAM, not Flash)
extern float32_t firCoeffsBuffer[NUM_TAPS];
extern float32_t firStateBuffer[NUM_TAPS + BLOCK_SIZE - 1]; //
extern float32_t dsp_input[BLOCK_SIZE],dsp_output[BLOCK_SIZE];

extern arm_fir_instance_f32 S;

void update_my_FIR_filter(filter_type_t new_type, float32_t f_low, float32_t f_high, float32_t fs)
{
    // 1. Calculate the new math on-the-fly
    generate_fir_coefficients(firCoeffsBuffer, NUM_TAPS, new_type, f_low, f_high, fs);

    // 2. Clear out the state history to prevent loud clicks or transitions
    memset(firStateBuffer, 0, sizeof(firStateBuffer));

    // 3. Re-initialize the CMSIS runtime instance
    arm_fir_init_f32(&S, NUM_TAPS, firCoeffsBuffer, firStateBuffer, BLOCK_SIZE);
}

// Helper function to process data blocks
void Process_DSP_Block(uint16_t *in_buf, uint16_t *out_buf) {
    // 1. Scale integer ADC values (0 to 4095) to float (-1.0 to 1.0)
    for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
        dsp_input[i] = ((float32_t)in_buf[i] - 2048.0f) / 2048.0f;
    }

    // 2. RUN THE OPTIMIZED FIR FILTER (Processes an entire block in microseconds)
    arm_fir_f32(&S, dsp_input, dsp_output, BLOCK_SIZE); //

    // 3. Rescale and convert float back to 12-bit Integer for the DAC
    for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
        // Clamp bounds to prevent overflow wrapping distortions
        if (dsp_output[i] > 1.0f) dsp_output[i] = 1.0f;
        if (dsp_output[i] < -1.0f) dsp_output[i] = -1.0f;

        out_buf[i] = (uint16_t)((dsp_output[i] * 2047.0f) + 2048.0f);
    }
}
#endif

#ifdef IIR

#endif
#endif // FILTERS_H
