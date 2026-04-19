#include "stft_processor.h"
#include "arm_math.h"
#include <math.h>
#include <string.h>

/* ============================================================================
    STFT + INT8 quantization for TinyML keyword spotting on STM32F401

    Based on the TensorFlow notebook:
    - Frame length: 255
    - Frame step: 128
    - STFT output: 124 frames x 129 frequency bins (including Nyquist)
    - Quantization: int8 (as used by the TFLite model)
    ============================================================================ */

/* STFT parameters (match the Python training setup) */
#define AUDIO_SAMPLE_RATE       16000
#define AUDIO_DURATION_MS       1000
#define AUDIO_SAMPLES           (AUDIO_SAMPLE_RATE * AUDIO_DURATION_MS / 1000)  // 16000

#define STFT_FRAME_LENGTH       255     // Window length
#define STFT_FRAME_STEP         128     // Frame overlap: 255-128=127 samples
#define FFT_SIZE                256     // Next power of 2
#define NUM_FREQUENCIES         129     // 256/2 + 1 (including DC and Nyquist)
#define NUM_FRAMES              124     // (16000-255)/128 + 1

/*
 * Log compression as used in TensorFlow pipelines:
 * spectrogram = log(spectrogram + eps)
 *
 * Important: This preprocessing must match training.
 */
#define STFT_USE_LOG_COMPRESSION 1
#define STFT_LOG_EPSILON         1e-6f

/*
 * ARM DSP normalization:
 * Factor 22 compressed the spectrum too much (almost everything became -128 after quantization).
 * Factor 1 keeps the features more dynamic and easier to classify.
 */
#define ARM_FFT_NORM_FACTOR      1.0f

/* Buffer management optimized for a 96 KB RAM MCU. */
typedef struct {
    int16_t audio_buffer[AUDIO_SAMPLES];        // 16000 * 2 = 32 KB
    int audio_idx;

    // No separate STFT output buffer: quantize directly into the int8_t buffer.

    float32_t fft_input[FFT_SIZE];      // 256 * 4 = 1 KB
    float32_t fft_output[FFT_SIZE];     // 256 * 4 = 1 KB

} STFT_Processor;

static STFT_Processor stft_proc = {0};
static arm_rfft_fast_instance_f32 rfft_instance;

/* Debug statistics */
static STFT_DebugInfo stft_debug = {0};

/* Hann window for STFT (precomputed to save memory). */
static float32_t hann_window[STFT_FRAME_LENGTH];

/**
 * Initialize the Hann window.
 * w[n] = 0.5 * (1 - cos(2π*n/(N-1)))
 */
static void init_hann_window(void) {
    for (int i = 0; i < STFT_FRAME_LENGTH; i++) {
        float32_t val = 0.5f * (1.0f - arm_cos_f32(2.0f * PI * i / (STFT_FRAME_LENGTH - 1)));
        hann_window[i] = val;
    }
}

/** Initialize the STFT engine. */
void STFT_Init(void) {
    memset(&stft_proc, 0, sizeof(stft_proc));
    init_hann_window();

    // Initialize the CMSIS-DSP FFT.
    arm_rfft_fast_init_f32(&rfft_instance, FFT_SIZE);
}

/**
 * Add a new ADC sample to the audio buffer.
 *
 * Converts the 12-bit ADC value to int16 centered around zero.
 */
void STFT_AddSample(uint32_t adc_value) {
    if (stft_proc.audio_idx < AUDIO_SAMPLES) {
        // Center the 12-bit ADC value around 1550.
        stft_proc.audio_buffer[stft_proc.audio_idx++] = (int16_t)((int32_t)adc_value - 1550);
    }
}

/**
 * Compute the RMS energy of the audio buffer.
 * Used for energy-based voice activity detection.
 */
uint32_t STFT_GetRMS(void) {
    uint64_t sum_sq = 0;
    int samples = stft_proc.audio_idx > 0 ? stft_proc.audio_idx : AUDIO_SAMPLES;

    for (int i = 0; i < samples; i++) {
        int32_t val = (int32_t)stft_proc.audio_buffer[i];
        sum_sq += val * val;
    }

    float32_t mean_sq = (float32_t)sum_sq / (float32_t)samples;
    float32_t rms = sqrtf(mean_sq);

    return (uint32_t)rms;
}

/**
 * Return debug statistics from the last STFT run.
 */
const STFT_DebugInfo* STFT_GetDebugInfo(void) {
    return &stft_debug;
}

/**
 * Compute the STFT after one second of audio and quantize it directly to int8.
 *
 * This version:
 * 1. Computes the ARM FFT (CMSIS-DSP)
 * 2. Normalizes the ARM magnitude by 128 to match the TensorFlow STFT
 * 3. Quantizes using the TFLite scale/zero_point
 * 4. Stores the result directly in the int8 output buffer
 *
 * Return: 0 on success, -1 if there is not enough audio
 */
int STFT_Process_Quantized(int8_t* out_int8, float32_t scale, int8_t zero_point) {
    if (out_int8 == NULL || scale <= 0.0f) {
        return -2;
    }

    if (stft_proc.audio_idx < AUDIO_SAMPLES) {
        return -1;  // Not enough audio
    }

    int output_idx = 0;
    memset(&stft_debug, 0, sizeof(stft_debug));
    stft_debug.max_magnitude = -1e10f;
    stft_debug.min_magnitude = 1e10f;
    int frame_idx = 0;

    // Process each frame.
    for (int frame_start = 0; frame_start + STFT_FRAME_LENGTH <= AUDIO_SAMPLES;
         frame_start += STFT_FRAME_STEP) {

        // 1. Extract audio frame with Hann window.
        // Normalize: int16 [-2048, 2047] -> float [-1.0, 1.0]
        for (int i = 0; i < STFT_FRAME_LENGTH; i++) {
            float32_t sample = (float32_t)stft_proc.audio_buffer[frame_start + i] / 2048.0f;
            stft_proc.fft_input[i] = sample * hann_window[i];
        }

        // 2. Zero-pad to FFT_SIZE.
        for (int i = STFT_FRAME_LENGTH; i < FFT_SIZE; i++) {
            stft_proc.fft_input[i] = 0.0f;
        }

        // 3. Real FFT (CMSIS-DSP).
        arm_rfft_fast_f32(&rfft_instance, stft_proc.fft_input, stft_proc.fft_output, 0);

        // 4 & 5. Compute magnitude and quantize directly.
        // FFT output: [re0, im0, re1, im1, ..., re128]
        for (int k = 0; k < NUM_FREQUENCIES; k++) {
            float32_t real, imag, mag_sq, mag_float;

            if (k == 0 || k == NUM_FREQUENCIES - 1) {
                // DC and Nyquist: real part only.
                real = stft_proc.fft_output[2 * k];
                imag = 0.0f;
            } else {
                real = stft_proc.fft_output[2 * k];
                imag = stft_proc.fft_output[2 * k + 1];
            }

            // Magnitude: sqrt(re^2 + im^2)
            mag_sq = real * real + imag * imag;
            mag_float = sqrtf(mag_sq);

            // Critical: normalize ARM DSP output to match TensorFlow.
            mag_float = mag_float / ARM_FFT_NORM_FACTOR;

#if STFT_USE_LOG_COMPRESSION
            // Optional dynamic compressor as used in Colab/TensorFlow.
            // Stable against log(0) thanks to epsilon.
            mag_float = logf(mag_float + STFT_LOG_EPSILON);
#endif

            // Track debug info (frame 0, first 10 bins only).
            if (frame_idx == 0 && k < 10) {
                stft_debug.first_magnitude[k] = mag_float;
            }
            if (mag_float > stft_debug.max_magnitude) {
                stft_debug.max_magnitude = mag_float;
            }
            if (mag_float < stft_debug.min_magnitude) {
                stft_debug.min_magnitude = mag_float;
            }

            // TFLite quantization: q_i8 = round((value / scale) + zero_point)
            float32_t scaled = (mag_float / scale) + (float32_t)zero_point;
            int32_t tmp = (int32_t)roundf(scaled);

            // Saturate to the int8 range.
            if (tmp < -128) tmp = -128;
            if (tmp > 127) tmp = 127;

            out_int8[output_idx++] = (int8_t)tmp;
        }

        frame_idx++;
    }

    stft_debug.frame_count = frame_idx;

    // Reset for the next recording.
    stft_proc.audio_idx = 0;
    return 0;  // Success
}

/**
 * Legacy compatibility wrapper for STFT_Process.
 */
int STFT_Process(void) {
    // Kept only for compatibility.
    return -1;
}

/* STFT_GetSpectrogram was removed because the pipeline now quantizes directly to int8. */

/**
 * Reset the audio buffer for a new recording.
 */
void STFT_Reset(void) {
    stft_proc.audio_idx = 0;
    memset(stft_proc.audio_buffer, 0, sizeof(stft_proc.audio_buffer));
}

/** Return the number of collected samples. */
int STFT_GetSampleCount(void) {
    return stft_proc.audio_idx;
}

/** Return the number of STFT frames. */
int STFT_GetFrameCount(void) {
    return NUM_FRAMES;
}

/** Return the number of frequency bins per frame. */
int STFT_GetFrequencyBins(void) {
    return NUM_FREQUENCIES;
}
