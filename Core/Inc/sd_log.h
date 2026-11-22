/*
 * sd_log.h
 *
 *  Created on: 21-Nov-2026
 *      Author: sriramkumark
 */

#ifndef INC_SD_LOG_H_
#define INC_SD_LOG_H_

#include "ff.h"
#include <stdint.h>

#define RAM_LOG_BUFFER_SIZE    4096U

typedef enum
{
    RAM_LOG_FREE = 0,
    RAM_LOG_FILLING,
    RAM_LOG_READY,
    RAM_LOG_WRITING
} RAM_LogState_t;

typedef struct
{
    uint8_t data[RAM_LOG_BUFFER_SIZE];
    uint16_t length;
    volatile RAM_LogState_t state;
} RAM_LogBuffer_t;

FRESULT SD_Logger_Init(void);

FRESULT SD_Logger_Write(const char *buffer, uint16_t length);

FRESULT SD_Logger_Close(void);

void RAM_Log_Init(void);

bool RAM_Log_Append(const char *data, uint16_t length);

RAM_LogBuffer_t *RAM_Log_GetReady(void);

void RAM_Log_Release(RAM_LogBuffer_t *buffer);

#endif /* INC_SD_LOG_H_ */
