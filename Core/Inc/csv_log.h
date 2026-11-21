/*
 * csv_log.h
 *
 *  Created on: 21-Nov-2026
 *      Author: sriramkumark
 */

#ifndef INC_CSV_LOG_H_
#define INC_CSV_LOG_H_

#include "bmi088.h"
#include <stdint.h>
#include <stddef.h>

#define TOTAL_IMU 3
#define LOG_BUFFER 256

void csv_log(char *buffer, size_t buffer_size,const BMI088_Data_t accel[TOTAL_IMU],
		const BMI088_Data_t gyro[TOTAL_IMU]);


#endif /* INC_CSV_LOG_H_ */
