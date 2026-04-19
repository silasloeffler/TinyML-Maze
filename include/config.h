#ifndef CONFIG_H
#define CONFIG_H

/**
 * @file config.h
 * @brief Global project configuration and tunable parameters.
 * @author Silas Loeffler
 */

/* Voice command class indices used by the model and UI logic. */
#define CLASS_DOWN    0  // Voice-command class for DOWN.
#define CLASS_LEFT    1  // Voice-command class for LEFT.
#define CLASS_RANDOM  2  // Voice-command class for RANDOM.
#define CLASS_RIGHT   3  // Voice-command class for RIGHT.
#define CLASS_UP      4  // Voice-command class for UP.

#define ADC_CENTER_VALUE          1523U    // ADC midpoint used to convert raw samples into signed amplitudes.
#define VAD_WINDOW_SIZE           16U      // Moving average window size used by the VAD smoothing filter.
#define VAD_THRESHOLD             80U      // Main VAD trigger threshold for the normal recording flow.

#define VAD_RELEASE_THRESHOLD     130U     // Release threshold used to arm the normal VAD trigger after silence.
#define VAD_REQUIRED_HITS         1U       // Consecutive VAD hits required to trigger recording.
#define VAD_WELCOME_THRESHOLD     60U      // Lower threshold used only for the welcome screen start trigger.
#define VAD_WELCOME_REQUIRED_HITS 1U       // Consecutive hits required for the easier welcome start trigger.
#define VAD_POLL_DELAY_MS         1U       // Poll delay while listening for speech in milliseconds.

#define PRE_ROLL_SAMPLES          3200U    // Pre-roll buffer length in samples (200 ms at 16 kHz).
#define AUDIO_SAMPLE_RATE         16000U   // Audio sampling rate used by ADC timing and STFT processing.

#define RECORD_SAMPLES            16000U   // Number of samples recorded for one inference window.
#define ADC_POLL_TIMEOUT_MS       1U       // ADC poll timeout in milliseconds.
#define OUTPUT_CLASS_COUNT        5U       // Number of output classes predicted by the model.

/* Quantization settings used when writing the STFT output into the model input. */
#define QUANT_SCALE        0.072971389f    // Quantization scale (1/128) used for the model input.
#define QUANT_ZERO_POINT  (61)

/* Tile size in pixels for the OLED maze grid. */
#define TILE_SIZE                  8        // Tile size in pixels for the OLED maze grid.
#define OLED_LABEL_X               0U       // Left-hand x-coordinate of the score labels on the OLED.
#define OLED_BAR_X                 10U      // Left edge of the score bars on the OLED.
#define OLED_BAR_WIDTH             112U     // Width of each score bar on the OLED.
#define OLED_BAR_HEIGHT            12U      // Height of each score bar on the OLED.
#define OLED_ROW0_Y                4U       // Top row y-position for score rendering.

#define OLED_ROW1_Y                24U      // Middle row y-position for score rendering.
#define OLED_ROW2_Y                44U      // Bottom row y-position for score rendering.

#define DISPLAY_IDLE_MODE_LAST      0U      // Idle display behavior after processing.
#define DISPLAY_IDLE_MODE_EMPTY     1U      // Show empty bars while idle.
#define DISPLAY_IDLE_MODE_FULL      2U      // Show full bars while idle.

/* Configurable display behavior after processing a command. */
#define DISPLAY_IDLE_MODE          DISPLAY_IDLE_MODE_EMPTY

#endif
