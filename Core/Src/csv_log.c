/*
 * csv_log.c
 *
 *  Created on: 21-Nov-2026
 *      Author: sriramkumark
 */

#include "csv_log.h"
#include <stdio.h>

void csv_log(char *buffer, size_t buffer_size,const BMI088_Data_t accel[TOTAL_IMU],
		const BMI088_Data_t gyro[TOTAL_IMU])
{
	 snprintf(buffer, buffer_size,
    		        "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,"
    		        "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,"
    		        "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\r\n",

    		        accel[0].x, accel[0].y, accel[0].z,
    		        gyro[0].x, gyro[0].y, gyro[0].z,

    		        accel[1].x, accel[1].y, accel[1].z,
    		        gyro[1].x, gyro[1].y, gyro[1].z,

    		        accel[2].x, accel[2].y, accel[2].z,
    		        gyro[2].x, gyro[2].y, gyro[2].z
    		    );
}

