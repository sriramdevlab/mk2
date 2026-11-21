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

#define LOG_BUFFER 256

int csv_log(char *buffer, size_t buffer_size,const BMI088_Data_t accel[TOTAL_IMU],
		const BMI088_Data_t gyro[TOTAL_IMU],const BMI088_Data_Fusion_t *fused);

int csv_log_Header(char *buffer, size_t buffer_size);

#endif /* INC_CSV_LOG_H_ */
