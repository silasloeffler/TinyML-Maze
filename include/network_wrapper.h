#ifndef NETWORK_WRAPPER_H
#define NETWORK_WRAPPER_H

/**
 * @file network_wrapper.h
 * @brief AI network wrapper around the generated TinyML runtime.
 * @author Silas Loeffler
 */

#include <stdint.h>

int8_t* AI_GetInputBuffer(void);
const char* AI_GetClassName(int index);
void AI_Init(void);
void AI_Run(void);
int AI_GetArgMax(void);

#endif
