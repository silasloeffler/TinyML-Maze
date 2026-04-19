#ifndef STFT_PROCESSOR_H
#define STFT_PROCESSOR_H

#include <stdint.h>
#include "arm_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
   STFT Processing with CMSIS-DSP

   Input:  16 kHz mono audio (500ms chunks = 8000 samples for sliding window)
   Output: STFT spectrogram with direct int8 quantization for TFLite inference

   STFT Parameters (matching TensorFlow training):
   - Frame length: 255 samples
   - Frame step: 128 samples (50% overlap)
   - Window: Hann
   - FFT size: 256 (next power of 2)
   - Output frequencies: 129 (0 to Nyquist)
   ============================================================================ */

/**
 * Initialize STFT processor and CMSIS-DSP FFT engine
 *
 * Must be called once during system startup
 */
void STFT_Init(void);

/**
 * Add a new audio sample to the buffer
 *
 * @param adc_value: Raw 12-bit ADC value (0-4095)
 *
 * Converts 12-bit ADC to int16 (centered around 0)
 * For sliding window: collects chunks (8000 samples per 500ms)
 */
void STFT_AddSample(uint32_t adc_value);

/**
 * Calculate RMS (Root Mean Square) energy of current audio buffer
 * @return RMS energy as uint32_t
 * Used for energy-based voice activity detection (threshold = ~800-1000)
 */
uint32_t STFT_GetRMS(void);

/**
 * Process collected audio and compute STFT with direct int8 quantization
 *
 * @param out_int8: Output buffer (must be 124×129 = 16008 bytes of int8_t)
 * @param scale: Quantization scale from TFLite model
 * @param zero_point: Quantization zero point from TFLite model
 * @return 0 on success, -1 if buffer not full
 *
 * This version saves 64 KB of RAM by quantizing directly instead of
 * storing intermediate float32 buffer.
 * Duration: ~50-100ms on STM32F401 (CPU load approx 20%)
 */
int STFT_Process_Quantized(int8_t* out_int8, float32_t scale, int8_t zero_point);

/**
 * Reset audio buffer for next recording/window chunk
 * Called after STFT to prepare for next chunk or full 1-second recording
 */
void STFT_Reset(void);

/**
 * Get how many samples have been collected in current buffer
 * @return Sample count
 */
int STFT_GetSampleCount(void);

/**
 * Get number of STFT frames in the full 1-second recording
 * @return 124 (for 16000 samples at frame_step=128)
 */
int STFT_GetFrameCount(void);

/**
 * Get number of frequency bins per frame
 * @return 129 (FFT_SIZE/2 + 1 = 256/2 + 1)
 */
int STFT_GetFrequencyBins(void);

/**
 * Get debug information from last STFT processing
 * Useful for troubleshooting magnitude calculation issues
 */
typedef struct {
    float32_t first_magnitude[10];  // First 10 frequency bins of frame 0
    float32_t max_magnitude;
    float32_t min_magnitude;
    int frame_count;
} STFT_DebugInfo;

const STFT_DebugInfo* STFT_GetDebugInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* STFT_PROCESSOR_H */
