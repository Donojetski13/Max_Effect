// my_first_fft.c
#include <stdio.h>
#include "UI_headers/tuner.h"
#include "UI_headers/pedal_effect.h"
#include "UI_headers/audio_codec.h"

int chain_length = 0; // Current length of the effect chain
// #define RELEASE
// #define TUNER
//#define PEDAL_EFFECTS
// #define SCREEN
#define CODEC

int main() {
    /* USER CODE BEGIN 2 */
    effect_pair effect_chain[NUM_EFFECTS];

    //  pitch_detection_demo();

    // tuning and pitch detection
    /* Performance considerations
    - FFT size: Larger = better frequency resolution
    - Sample rate: Higher = detect higher frequencies
    - Window function: Reduces spectral leakage
    - HPS: Good for harmonic sounds
    - Autocorrelation: Robust to noise
    - Combine methods for better accuracy
    */
    double sample_rate = 66666.67; // base on ADC sample rate(1MHz, 12b resolution, 2.5 cycles/sample) = 66666.67 Hz
    int n_tune = 1024, n_effects = 8; // signal length/chunks
    complex_t *effect_signal = allocate_complex_array(n_effects);
    complex_t *tune_signal = allocate_complex_array(n_tune);

    #ifdef CODEC
    // Initialize the audio codec
    if (!codec_init()) {
        printf("Codec initialization failed!\n");
        return -1;
    }
    
    codec_set_intput_gain(1.0f); // Set input gain (example value, adjust as needed)
    #endif
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
    #ifdef TUNER
    printf("\nGetting signal...\n");
    // ADC value to complex_t array conversion
    get_input_signal(tune_signal, n_tune);
    printf("\nProcessing signal...\n");
    // Apply window to reduce spectral leakage
    //	  apply_window_hann(signal, n);

    // Process signal with FFT
    //	  memcpy(spectrum, signal, n * sizeof(complex_t));
    radix2_dif_fft(tune_signal, n_tune, FFT_FORWARD);

    printf("\nGetting frequency...\n");
    double pitch_hps = detect_pitch_hps(tune_signal, n_tune, sample_rate, 3);
    //	  double pitch_autocorr = detect_pitch_autocorr(signal, n, sample_rate);

    //	  printf("True frequency: 440.00 Hz (A4)\n");
    printf("HPS method: %.2f Hz\n", pitch_hps);
    //	  printf("Autocorrelation: %.2f Hz (error: %.2f Hz)\n",
    //		  pitch_autocorr, pitch_autocorr - 440);

    // code for tuning)
    printf(frequency_to_note_name(pitch_hps));
    //	  HAL_Delay(1);
    #endif

    #ifdef PEDAL_EFFECTS
    printf("\nGetting signal...\n");
    get_input_signal(effect_signal, n_effects);
    printf("\nAdding effects...\n");
    // Example: Add effects to the chain
    params distortion_params = {true, 0.8f, 0.5f, 0.7f}; // Example distortion parameters
    chain_length = add_effect(E_DISTORTION, distortion_params, effect_chain, chain_length);

    // Apply the effect chain to the signal
    printf("\nApplying effect chain...\n");
    apply_effect_chain(effect_signal, n_effects, effect_chain, chain_length);

    // Output the processed signal (implementation depends on hardware)
    printf("\nOutputting signal...\n");
    output2codec(effect_signal, n_effects);
    #endif

    #ifdef CODEC
    // Get input signal from ADC
    get_input_signal(effect_signal, n_effects);

    // send signal to codec for output (testing no effects latency)
    output2codec(effect_signal, n_effects);
    #endif
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  free_complex_array(tune_signal);
  free_complex_array(effect_signal);
  /* USER CODE END 3 */
    return 0;
}