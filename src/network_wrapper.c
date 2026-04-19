/**
 * @file network_wrapper.c
 * @brief Thin wrapper around the generated AI network runtime.
 * @author Silas Loeffler
 */

#include "network_wrapper.h"
#include "main.h"
#include "network.h"
#include "network_data.h"
#include "ai_platform.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static ai_handle network = AI_HANDLE_NULL;
AI_ALIGNED(32)
static ai_u8 activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];
static ai_i8 in_data[AI_NETWORK_IN_1_SIZE];
static ai_i8 out_data[AI_NETWORK_OUT_1_SIZE];
static ai_buffer* network_input = NULL;
static ai_buffer* network_output = NULL;

static const char* kClassNames[OUTPUT_CLASS_COUNT] = { "DOWN", "LEFT", "RANDOM", "RIGHT", "UP" };

static void Network_Log(const char* text) {
    if (text == NULL) {
        return;
    }

    HAL_UART_Transmit(&huart2, (uint8_t*)text, (uint16_t)strlen(text), 100);
}

static void Network_FatalError(uint16_t code) {
    char msg[64];
    int len = snprintf(msg, sizeof(msg), "AI ERR %u\r\n", code);
    if (len > 0) {
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, (uint16_t)len, 100);
    }

    while (1) {
        for (uint16_t i = 0; i < code; i++) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
            HAL_Delay(180);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
            HAL_Delay(180);
        }
        HAL_Delay(900);
    }
}

void AI_Init(void) {
    Network_Log("AI INIT START\r\n");

    memset(activations, 0, sizeof(activations));
    memset(out_data, 0, sizeof(out_data));

    if ((uintptr_t)activations % AI_NETWORK_ACTIVATIONS_ALIGNMENT != 0U) {
        Network_FatalError(10);
    }

    Network_Log("AI INIT ALIGN OK\r\n");

    const ai_handle act_addr[] = { AI_HANDLE_PTR(activations) };
    ai_error err = ai_network_create_and_init(&network, act_addr, NULL);
    if (err.type != AI_ERROR_NONE || network == AI_HANDLE_NULL) {
        char msg[64];
        int len = snprintf(msg, sizeof(msg), "AI CREATE FAIL T:%d C:%d\r\n", err.type, err.code);
        if (len > 0) {
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, (uint16_t)len, 100);
        }
        Network_FatalError(12);
    }

    Network_Log("AI CREATE OK\r\n");

    ai_u16 n_in = 0;
    ai_u16 n_out = 0;
    network_input = ai_network_inputs_get(network, &n_in);
    network_output = ai_network_outputs_get(network, &n_out);
    if (network_input == NULL || network_output == NULL || n_in != 1 || n_out != 1) {
        Network_FatalError(14);
    }

    Network_Log("AI IO OK\r\n");

    network_input[0].data = AI_HANDLE_PTR(in_data);
    network_output[0].data = AI_HANDLE_PTR(out_data);

    Network_Log("AI INIT DONE\r\n");
}

int AI_GetArgMax(void) {
    int best_index = 0;
    int best_value = (int)out_data[0];

    for (int index = 1; index < (int)OUTPUT_CLASS_COUNT; index++) {
        int current_value = (int)out_data[index];
        if (current_value > best_value) {
            best_value = current_value;
            best_index = index;
        }
    }

    return best_index;
}

int8_t* AI_GetInputBuffer(void) {
    return (int8_t*)in_data;
}

const char* AI_GetClassName(int index) {
    if (index < 0 || index >= (int)OUTPUT_CLASS_COUNT) {
        return "UNKNOWN";
    }

    return kClassNames[index];
}

void AI_Run(void) {
    if (network == AI_HANDLE_NULL || network_input == NULL || network_output == NULL) {
        Network_FatalError(20);
    }

    if (ai_network_run(network, &network_input[0], &network_output[0]) != 1) {
        ai_error run_err = ai_network_get_error(network);
        char msg[64];
        int len = snprintf(msg, sizeof(msg), "AI RUN FAIL T:%d C:%d\r\n", run_err.type, run_err.code);
        if (len > 0) {
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, (uint16_t)len, 100);
        }
        Network_FatalError(21);
    }
}
