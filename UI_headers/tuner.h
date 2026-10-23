/*
tuner.h
Music Pitch Detection using FFT for tuning
including harmonic product spectrum and autocorrelation.
*/

#ifndef TUNER_H
#define TUNER_H
#include <stdint.h>
#include "../FFT-implementation-in-C-reduced/include/fft_common.h"
#include "../FFT-implementation-in-C-reduced/include/fft_algorithms.h"

// Musical note frequencies (A4 = 440 Hz)
typedef struct {
    const char* name;
    double frequency; // Frequency in Hz
} musical_note;

musical_note notes[] = { // notes for comparison for bass (4/5/6 string) and guitar
	{"B0", 30.87}, {"E1", 41.20}, {"A1", 55.00},
	{"E2", 82.41}, {"G2", 98.00}, {"A2", 110.00},
	{"C3", 130.81},{"D3", 146.83},{"G3", 196.00},
	{"B3", 246.94},{"E4", 329.63}
};

int num_notes = sizeof(notes) / sizeof(notes[0]);

// Find closest musical note
const char* frequency_to_note_name(double freq) {
    int closest_idx = 0;
    double min_cents = 1200;  // Max cents difference

    for (int i = 0; i < num_notes; i++) {
        double cents = 1200 * log2(freq / notes[i].frequency);
        if (fabs(cents) < fabs(min_cents)) {
            min_cents = cents;
            closest_idx = i;
        }
    }

    static char result[32];
    if (fabs(min_cents) < 1) {
        snprintf(result, sizeof(result), "%s (in tune)\n", notes[closest_idx].name);
    } else {
        snprintf(result, sizeof(result), "%s (%+.0f cents)\n",
                notes[closest_idx].name, min_cents);
    }

    return result;
}

// Window functions for spectral analysis
void apply_window_hann(complex_t* signal, int n) {
    for (int i = 0; i < n; i++) {
        double window = 0.5 * (1.0 - cos(TWO_PI * i / (n - 1)));
        signal[i] *= window;
    }
}

void apply_window_hamming(complex_t* signal, int n) {
    for (int i = 0; i < n; i++) {
        double window = 0.54 - 0.46 * cos(TWO_PI * i / (n - 1));
        signal[i] *= window;
    }
}

void apply_window_blackman(complex_t* signal, int n) {
    for (int i = 0; i < n; i++) {
        double window = 0.42 - 0.5 * cos(TWO_PI * i / (n - 1))
                      + 0.08 * cos(4 * PI * i / (n - 1));
        signal[i] *= window;
    }
}

// Simple peak detection for fundamental frequency bass and guitar
double detect_pitch_peak(complex_t* spectrum, int n, double sample_rate) {
    double* magnitude = compute_magnitude(spectrum, n);

    // Find peak in reasonable frequency range (80-2000 Hz)
    int min_bin = (int)(80 * n / sample_rate);
    int max_bin = (int)(2000 * n / sample_rate);
    if (max_bin > n/2) max_bin = n/2;

    double max_mag = 0;
    int peak_bin = 0;

    for (int i = min_bin; i < max_bin; i++) {
        if (magnitude[i] > max_mag) {
            max_mag = magnitude[i];
            peak_bin = i;
        }
    }

    // Quadratic interpolation for more accurate frequency
    if (peak_bin > 0 && peak_bin < n/2 - 1) {
        double y1 = magnitude[peak_bin - 1];
        double y2 = magnitude[peak_bin];
        double y3 = magnitude[peak_bin + 1];

        double delta = 0.5 * (y1 - y3) / (y1 - 2*y2 + y3);
        peak_bin += delta;
    }

    free(magnitude);

    return peak_bin * sample_rate / n;
}

// Harmonic Product Spectrum (HPS) method (80-1500 Hz)
double detect_pitch_hps(complex_t* spectrum, int n, double sample_rate, int harmonics) {
    double* magnitude = compute_magnitude(spectrum, n);
    double* hps = (double*)malloc((n/2 + 1) * sizeof(double));

    // Initialize HPS with original spectrum
    for (int i = 0; i <= n/2; i++) {
        hps[i] = magnitude[i];
    }

    // Multiply with downsampled versions
    for (int h = 2; h <= harmonics; h++) {
        for (int i = 0; i <= n/(2*h); i++) {
            hps[i] *= magnitude[i * h];
        }
    }

    // Find peak in HPS
    int min_bin = (int)(80 * n / sample_rate);
    int max_bin = (int)(1500 * n / sample_rate);
    if (max_bin > n/(2*harmonics)) max_bin = n/(2*harmonics);

    double max_hps = 0;
    int peak_bin = 0;

    for (int i = min_bin; i < max_bin; i++) {
        if (hps[i] > max_hps) {
            max_hps = hps[i];
            peak_bin = i;
        }
    }

    free(magnitude);
    free(hps);

    return peak_bin * sample_rate / n;
}

// Autocorrelation-based pitch detection
double detect_pitch_autocorr(complex_t* signal, int n, double sample_rate) {
    // Compute autocorrelation using FFT
    complex_t* signal_copy = allocate_complex_array(n);
    memcpy(signal_copy, signal, n * sizeof(complex_t));

    // FFT
    radix2_dif_fft(signal_copy, n, FFT_FORWARD);

    // Power spectrum
    for (int i = 0; i < n; i++) {
        signal_copy[i] = signal_copy[i] * conj(signal_copy[i]);
    }

    // Inverse FFT to get autocorrelation
    radix2_dif_fft(signal_copy, n, FFT_INVERSE);

    // Find peak in autocorrelation (excluding lag 0)
    int min_lag = (int)(sample_rate / 1500);  // 1500 Hz max
    int max_lag = (int)(sample_rate / 80);     // 80 Hz min
    if (max_lag > n/2) max_lag = n/2;

    double max_corr = 0;
    int peak_lag = 0;

    for (int lag = min_lag; lag < max_lag; lag++) {
        double corr = creal(signal_copy[lag]);
        if (corr > max_corr) {
            max_corr = corr;
            peak_lag = lag;
        }
    }

    free_complex_array(signal_copy);

    if (peak_lag > 0) {
        return sample_rate / peak_lag;
    }

    return 0;
}

// Pitch detection with confidence estimation
typedef struct {
    double frequency;
    double confidence;
    const char* note;
    double cents_off;
} pitch_result_t;

pitch_result_t detect_pitch_with_confidence(complex_t* signal, int n, double sample_rate) {
    pitch_result_t result = {0};

    // Method 1: Peak detection
    complex_t* spectrum = allocate_complex_array(n);
    memcpy(spectrum, signal, n * sizeof(complex_t));
    radix2_dif_fft(spectrum, n, FFT_FORWARD);

    double pitch1 = detect_pitch_peak(spectrum, n, sample_rate);

    // Method 2: HPS
    double pitch2 = detect_pitch_hps(spectrum, n, sample_rate, 5);

    // Method 3: Autocorrelation
    double pitch3 = detect_pitch_autocorr(signal, n, sample_rate);

    // Combine results
    result.frequency = pitch2;  // HPS is often most reliable

    // Estimate confidence based on agreement between methods
    double avg_pitch = (pitch1 + pitch2 + pitch3) / 3;
    double variance = pow(pitch1 - avg_pitch, 2) +
                     pow(pitch2 - avg_pitch, 2) +
                     pow(pitch3 - avg_pitch, 2);
    variance /= 3;

    result.confidence = 1.0 / (1.0 + sqrt(variance) / avg_pitch);

    // Find musical note
    result.note = frequency_to_note_name(result.frequency);

    free_complex_array(spectrum);

    return result;
}

// Generate test signals
void generate_musical_note(complex_t* signal, int n, double freq, double sample_rate,
                          int num_harmonics, double* harmonic_amps) {
    for (int i = 0; i < n; i++) {
        signal[i] = 0;
        for (int h = 1; h <= num_harmonics; h++) {
            double amp = (harmonic_amps && h <= num_harmonics) ?
                        harmonic_amps[h-1] : 1.0 / h;
            signal[i] += amp * sin(2 * PI * freq * h * i / sample_rate);
        }
    }
}

void pitch_detection_demo() {
	printf("Music Pitch Detection using FFT\n");
	printf("================================\n\n");

	double sample_rate = 66666.67; // base on ADC sample rate(1MHz, 12b resolution, 2.5 cycles/sample) = 66666.67 Hz
	int n = 1024;
	complex_t* signal = allocate_complex_array(n);

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

	free_complex_array(signal);
}

#endif // TUNER_H
