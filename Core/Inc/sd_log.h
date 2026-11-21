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

FRESULT SD_Logger_Init(void);

FRESULT SD_Logger_Write(const char *buffer, uint16_t length);

FRESULT SD_Logger_Close(void);

#endif /* INC_SD_LOG_H_ */
