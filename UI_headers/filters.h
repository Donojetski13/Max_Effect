/*
filters.h
Filter implementations for the audio processing system
*/

#ifndef FILTERS_H
#define FILTERS_H
#include <stdint.h>
#include "../FFT-implementation-in-C-reduced/include/fft_common.h"
#include "../FFT-implementation-in-C-reduced/include/fft_algorithms.h"

// Filter types
typedef enum {
    FILTER_LOWPASS,
    FILTER_HIGHPASS,
    FILTER_BANDPASS,
    FILTER_BANDSTOP
} filter_type_t;

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

#endif // FILTERS_H