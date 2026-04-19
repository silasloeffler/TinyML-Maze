/**
 * @file main.c
 * @brief Main application logic, including the state machine, audio flow, and game orchestration.
 * @author Silas Loeffler
 */
#include "main.h"
#include "stft_processor.h"
#include "ssd1306.h"
#include "config.h"
#include "hardware_config.h"
#include "display.h"
#include "game.h"
#include "network_wrapper.h"
#include "arm_math.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static uint16_t pr_buffer[PRE_ROLL_SAMPLES];
static uint32_t pr_head = 0;
static uint32_t pr_count = 0;

int player_x = 1;
int player_y = 1;
int current_level = 0;
static uint8_t g_show_congrats = 0;
static uint32_t level_finish_time = 0U;

static void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CTRL |= 1U;
}

static void DWT_EnsureEnabled(void) {
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U) {
        DWT_Init();
    }
}

typedef enum {
    STATE_WELCOME = 0,
    STATE_LISTENING,
    STATE_RECORDING,
    STATE_PROCESSING,
    STATE_ACTION,
    STATE_LEVEL_WAIT
} AppState;

typedef struct {
    uint32_t samples[VAD_WINDOW_SIZE];
    uint32_t sum;
    uint32_t index;
    uint32_t count;
} MovingAverageFilter;

static MovingAverageFilter g_vad_filter = {0};
static int g_last_prediction = CLASS_RANDOM;
static uint8_t g_vad_armed = 0;
static uint32_t g_vad_trigger_hits = 0;
static uint32_t g_welcome_trigger_hits = 0;
static uint32_t g_last_vad_log_ms = 0;

static inline uint32_t GetSamplePeriodCycles(void) {
    return SystemCoreClock / AUDIO_SAMPLE_RATE;
}

static void WaitUntilCycle(uint32_t target_cycle) {
    while ((int32_t)(DWT->CYCCNT - target_cycle) < 0) {
    }
}

static uint32_t ADC_ReadRaw(void) {
    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        return ADC_CENTER_VALUE;
    }
    if (HAL_ADC_PollForConversion(&hadc1, ADC_POLL_TIMEOUT_MS) != HAL_OK) {
        return ADC_CENTER_VALUE;
    }
    return HAL_ADC_GetValue(&hadc1);
}

static void VAD_Reset(MovingAverageFilter* filter) {
    if (filter != NULL) {
        memset(filter, 0, sizeof(*filter));
    }
}

static uint32_t VAD_Update(MovingAverageFilter* filter, uint32_t adc_value) {
    uint32_t amplitude = (uint32_t)abs((int32_t)adc_value - (int32_t)ADC_CENTER_VALUE);

    filter->sum -= filter->samples[filter->index];
    filter->samples[filter->index] = amplitude;
    filter->sum += amplitude;
    filter->index = (filter->index + 1U) % VAD_WINDOW_SIZE;

    if (filter->count < VAD_WINDOW_SIZE) {
        filter->count++;
    }

    return filter->sum / filter->count;
}

static void VAD_ResetState(void) {
    VAD_Reset(&g_vad_filter);
    g_vad_armed = 0;
    g_vad_trigger_hits = 0;
    g_welcome_trigger_hits = 0;
}

static uint8_t VAD_ShouldStartWelcome(uint32_t vad_value) {
    if (vad_value >= VAD_WELCOME_THRESHOLD) {
        if (g_welcome_trigger_hits < 0xFFFFFFFFU) {
            g_welcome_trigger_hits++;
        }
    } else {
        g_welcome_trigger_hits = 0;
    }

    if (g_welcome_trigger_hits >= VAD_WELCOME_REQUIRED_HITS) {
        g_welcome_trigger_hits = 0;
        return 1;
    }

    return 0;
}

static uint8_t VAD_ShouldTrigger(uint32_t vad_value) {
    if (!g_vad_armed) {
        if (vad_value <= VAD_RELEASE_THRESHOLD) {
            g_vad_armed = 1;
        }
        g_vad_trigger_hits = 0;
        return 0;
    }

    if (vad_value >= VAD_THRESHOLD) {
        if (g_vad_trigger_hits < 0xFFFFFFFFU) {
            g_vad_trigger_hits++;
        }
    } else {
        g_vad_trigger_hits = 0;
    }

    if (g_vad_trigger_hits >= VAD_REQUIRED_HITS) {
        g_vad_armed = 0;
        g_vad_trigger_hits = 0;
        return 1;
    }

    return 0;
}

static int RecordOneSecondPrecise(void) {
    DWT_EnsureEnabled();

    uint32_t sample_period_cycles = GetSamplePeriodCycles();
    uint32_t next_cycle = DWT->CYCCNT + sample_period_cycles;

    while (STFT_GetSampleCount() < (int)RECORD_SAMPLES) {
        uint32_t adc_value = ADC_ReadRaw();
        STFT_AddSample(adc_value);
        WaitUntilCycle(next_cycle);
        next_cycle += sample_period_cycles;
    }

    return (STFT_GetSampleCount() >= (int)RECORD_SAMPLES) ? 0 : -1;
}

void SysTick_Handler(void) {
    HAL_IncTick();
}

static void UART_Log(const char* text) {
    if (text == NULL) {
        return;
    }
    HAL_UART_Transmit(&huart2, (uint8_t*)text, (uint16_t)strlen(text), 100);
}
int main(void) {
    HAL_Init();
    SystemClock_Config();
    DWT_Init();

    /* Hardware init in the required order. */
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();

    ssd1306_Init();

    char msg[64];
    int boot_len = snprintf(msg, sizeof(msg), "BOOT OK\r\n");
    if (boot_len > 0) {
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, (uint16_t)boot_len, 100);
    }

    UART_Log("HW INIT OK\r\n");

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

    UART_Log("PRE AI\r\n");

    /* Start AI initialization */
    AI_Init();

    DWT_EnsureEnabled();

    snprintf(msg, sizeof(msg), "AI READY\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);

    STFT_Init();
    VAD_ResetState();
    UART_Log("STFT READY\r\n");
    UART_Log("STATE MACHINE READY\r\n");

    Game_ResetPlayerToLevelStart(current_level, &player_x, &player_y);

    AppState app_state = STATE_WELCOME;

    while (1) {
        switch (app_state) {
        case STATE_WELCOME: {
            Display_DrawWelcomeScreen();
            ssd1306_UpdateScreen();

            uint32_t adc_value = ADC_ReadRaw();

            pr_buffer[pr_head] = (uint16_t)adc_value;
            pr_head = (pr_head + 1U) % PRE_ROLL_SAMPLES;
            if (pr_count < PRE_ROLL_SAMPLES) {
                pr_count++;
            }

            uint32_t vad_value = VAD_Update(&g_vad_filter, adc_value);

            if (VAD_ShouldStartWelcome(vad_value) && g_vad_filter.count >= VAD_WINDOW_SIZE) {
                UART_Log("WELCOME->RECORDING|VAD TRIGGERED\r\n");
                STFT_Reset();

                {
                    uint32_t oldest_index = (pr_head + PRE_ROLL_SAMPLES - pr_count) % PRE_ROLL_SAMPLES;
                    for (uint32_t i = 0; i < pr_count; i++) {
                        uint32_t idx = (oldest_index + i) % PRE_ROLL_SAMPLES;
                        STFT_AddSample((uint32_t)pr_buffer[idx]);
                    }
                }
                pr_count = 0;
                pr_head = 0;

                VAD_ResetState();
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
                app_state = STATE_RECORDING;
            } else {
                HAL_Delay(VAD_POLL_DELAY_MS);
            }
            break;
        }

        case STATE_LISTENING: {
            uint32_t adc_value = ADC_ReadRaw();

            pr_buffer[pr_head] = (uint16_t)adc_value;
            pr_head = (pr_head + 1U) % PRE_ROLL_SAMPLES;
            if (pr_count < PRE_ROLL_SAMPLES) {
                pr_count++;
            }

            uint32_t vad_value = VAD_Update(&g_vad_filter, adc_value);
            uint32_t amplitude = (uint32_t)abs((int32_t)adc_value - (int32_t)ADC_CENTER_VALUE);

            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

            if (VAD_ShouldTrigger(vad_value) && g_vad_filter.count >= VAD_WINDOW_SIZE) {
                char msg[96];
                int len = snprintf(msg, sizeof(msg),
                    "LISTENING->RECORDING|ADC:%lu|VAD:%lu|HITS:%lu\r\n",
                    (unsigned long)adc_value,
                    (unsigned long)vad_value,
                    (unsigned long)g_vad_trigger_hits);
                if (len > 0) {
                    HAL_UART_Transmit(&huart2, (uint8_t*)msg, (uint16_t)len, 100);
                }
                STFT_Reset();

                {
                    uint32_t oldest_index = (pr_head + PRE_ROLL_SAMPLES - pr_count) % PRE_ROLL_SAMPLES;
                    for (uint32_t i = 0; i < pr_count; i++) {
                        uint32_t idx = (oldest_index + i) % PRE_ROLL_SAMPLES;
                        STFT_AddSample((uint32_t)pr_buffer[idx]);
                    }
                }
                pr_count = 0;
                pr_head = 0;

                VAD_ResetState();
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
                app_state = STATE_RECORDING;
            } else {
                uint32_t now = HAL_GetTick();
                if ((now - g_last_vad_log_ms) >= 500U) {
                    char msg[96];
                    int len = snprintf(msg, sizeof(msg),
                        "ADC:%lu|AMP:%lu|AVG:%lu|TH:%u|ARM:%u|H:%lu\r\n",
                        (unsigned long)adc_value,
                        (unsigned long)amplitude,
                        (unsigned long)vad_value,
                        (unsigned int)VAD_THRESHOLD,
                        (unsigned int)g_vad_armed,
                        (unsigned long)g_vad_trigger_hits);
                    if (len > 0) {
                        HAL_UART_Transmit(&huart2, (uint8_t*)msg, (uint16_t)len, 100);
                    }
                    g_last_vad_log_ms = now;
                }
                HAL_Delay(VAD_POLL_DELAY_MS);
            }
            break;
        }

        case STATE_RECORDING: {
            UART_Log("RECORDING\r\n");
            if (RecordOneSecondPrecise() == 0) {
                app_state = STATE_PROCESSING;
            } else {
                UART_Log("REC_FAIL\r\n");
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
                VAD_Reset(&g_vad_filter);
                app_state = STATE_LISTENING;
            }
            break;
        }

        case STATE_PROCESSING: {
            UART_Log("PROCESSING\r\n");

            int stft_result = STFT_Process_Quantized(AI_GetInputBuffer(), QUANT_SCALE, (int8_t)QUANT_ZERO_POINT);
            if (stft_result != 0) {
                char err_msg[64];
                int len = snprintf(err_msg, sizeof(err_msg), "STFT_FAIL:%d\r\n", stft_result);
                if (len > 0) {
                    HAL_UART_Transmit(&huart2, (uint8_t*)err_msg, (uint16_t)len, 100);
                }
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
                VAD_ResetState();
                app_state = STATE_LISTENING;
                break;
            }

            AI_Run();
            g_last_prediction = AI_GetArgMax();

            app_state = STATE_ACTION;
            break;
        }

        case STATE_ACTION: {
            if (level_finish_time == 0U) {
                Game_ProcessMove(g_last_prediction, &player_x, &player_y, &current_level, &g_show_congrats, &level_finish_time, HAL_GetTick());
            }
            Display_DrawGameScreen(current_level, player_x, player_y, g_show_congrats);
            ssd1306_UpdateScreen();

            if (level_finish_time > 0U) {
                app_state = STATE_LEVEL_WAIT;
            } else {
                if (g_show_congrats) {
                    UART_Log("ALL LEVELS COMPLETE\r\n");
                    g_show_congrats = 0U;
                }

                char msg[96];
                int len = snprintf(msg, sizeof(msg), "Inference Result: %s (Index %d)\r\n",
                    AI_GetClassName(g_last_prediction),
                    g_last_prediction);
                if (len > 0) {
                    HAL_UART_Transmit(&huart2, (uint8_t*)msg, (uint16_t)len, 100);
                }

                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
                STFT_Reset();
                VAD_ResetState();
                app_state = STATE_LISTENING;
            }
            break;
        }

        case STATE_LEVEL_WAIT: {
            uint32_t elapsed = HAL_GetTick() - level_finish_time;
            if (elapsed >= 2500U) {
                level_finish_time = 0U;
                if (g_show_congrats) {
                    Display_DrawFinalCongratsScreen();
                    ssd1306_UpdateScreen();
                    HAL_Delay(10U);
                } else {
                    STFT_Reset();
                    VAD_ResetState();
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
                    app_state = STATE_LISTENING;
                }
            } else {
                if (g_show_congrats) {
                    Display_DrawFinalCongratsScreen();
                } else {
                    Display_DrawLevelCompleteScreen();
                }
                ssd1306_UpdateScreen();
                HAL_Delay(10U);
            }
            break;
        }
        }
    }
}

