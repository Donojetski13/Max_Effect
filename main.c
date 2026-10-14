// my_first_fft.c
#include <stdio.h>
#include "UI_headers/tuner.h"
#include "UI_headers/pedal_effect.h"

int chain_length = 0; // Current length of the effect chain


int main() {
    effect_pair effect_chain[NUM_EFFECTS];

    // tuning and pitch detection
    /* Performance considerations
    - FFT size: Larger = better frequency resolution
    - Sample rate: Higher = detect higher frequencies
    - Window function: Reduces spectral leakage
    - HPS: Good for harmonic sounds
    - Autocorrelation: Robust to noise
    - Combine methods for better accuracy
    */
    printf("Music Pitch Detection using FFT\n");
    printf("================================\n\n");
    
    double sample_rate = 44100; // base on ADC sample rate
    int n = 4096;
    complex_t* signal = allocate_complex_array(n);
    complex_t* spectrum = allocate_complex_array(n);
    
    // Test 1: Pure sine wave
    printf("Test 1: Pure Sine Wave (A4 = 440 Hz)\n");
    printf("------------------------------------\n");
    
    for (int i = 0; i < n; i++) {
        signal[i] = sin(2 * PI * 440 * i / sample_rate);
    }
    
    pitch_result_t result = detect_pitch_with_confidence(signal, n, sample_rate);
    printf("Detected pitch: %.2f Hz\n", result.frequency);
    printf("Musical note: %s\n", result.note);
    printf("Confidence: %.1f%%\n\n", result.confidence * 100);
    
    // Test 2: Complex tone with harmonics
    printf("Test 2: Complex Tone (E4 with harmonics)\n");
    printf("----------------------------------------\n");
    
    double harmonic_amps[] = {1.0, 0.5, 0.3, 0.2, 0.1};
    generate_musical_note(signal, n, 329.63, sample_rate, 5, harmonic_amps);
    
    result = detect_pitch_with_confidence(signal, n, sample_rate);
    printf("Detected pitch: %.2f Hz\n", result.frequency);
    printf("Musical note: %s\n", result.note);
    printf("Confidence: %.1f%%\n\n", result.confidence * 100);
    
    // Test 3: Chord detection (challenging)
    printf("Test 3: C Major Chord (C4, E4, G4)\n");
    printf("-----------------------------------\n");
    
    for (int i = 0; i < n; i++) {
        signal[i] = sin(2 * PI * 261.63 * i / sample_rate) +  // C4
                   sin(2 * PI * 329.63 * i / sample_rate) +   // E4
                   sin(2 * PI * 392.00 * i / sample_rate);    // G4
    }
    
    // Apply window to reduce spectral leakage
    apply_window_hann(signal, n);
    
    result = detect_pitch_with_confidence(signal, n, sample_rate);
    printf("Detected pitch: %.2f Hz\n", result.frequency);
    printf("Musical note: %s\n", result.note);
    printf("Confidence: %.1f%%\n", result.confidence * 100);
    printf("(Note: Chord detection is challenging - usually detects root note)\n\n");
    
    // Test 4: Noisy signal
    printf("Test 4: Noisy Signal (A4 with 20%% noise)\n");
    printf("-----------------------------------------\n");
    
    for (int i = 0; i < n; i++) {
        signal[i] = sin(2 * PI * 440 * i / sample_rate) + 
                   0.2 * ((double)rand() / RAND_MAX - 0.5);
    }
    
    result = detect_pitch_with_confidence(signal, n, sample_rate);
    printf("Detected pitch: %.2f Hz\n", result.frequency);
    printf("Musical note: %s\n", result.note);
    printf("Confidence: %.1f%%\n\n", result.confidence * 100);
    
    while (1) {
        // Compare pitch detection methods
        printf("Method Comparison:\n");
        printf("==================\n");
        
        // read adc input or generate test signal
        generate_musical_note(signal, n, 220, sample_rate, 3, NULL);  // A3

        // // ADC value to complex_t array conversion
        // // (Assuming mono input, real part = ADC value, imaginary part = 0)
        // for (int i = 0; i < n; i++) {
        //     signal[i] = /*ADC value*/;  // Already generated as real values
        // }

        // Apply window to reduce spectral leakage
        apply_window_hann(signal, n);
        
        // Process signal with FFT
        memcpy(spectrum, signal, n * sizeof(complex_t));
        radix2_dif_fft(spectrum, n, FFT_FORWARD);
        
        double pitch_peak = detect_pitch_peak(spectrum, n, sample_rate);
        double pitch_hps = detect_pitch_hps(spectrum, n, sample_rate, 5);
        double pitch_autocorr = detect_pitch_autocorr(signal, n, sample_rate);
        
        printf("True frequency: 220.00 Hz (A3)\n");
        printf("Peak detection: %.2f Hz (error: %.2f Hz)\n", 
            pitch_peak, pitch_peak - 220);
        printf("HPS method: %.2f Hz (error: %.2f Hz)\n", 
            pitch_hps, pitch_hps - 220);
        printf("Autocorrelation: %.2f Hz (error: %.2f Hz)\n", 
            pitch_autocorr, pitch_autocorr - 220);

        // ... (code for tuning)
        printf(frequency_to_note_name(pitch_peak));
    }
    
    free_complex_array(signal);
    free_complex_array(spectrum);

    return 0;
}