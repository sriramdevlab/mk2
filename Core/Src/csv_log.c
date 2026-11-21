/*
 * csv_log.c
 *
 *  Created on: 21-Nov-2026
 *      Author: sriramkumark
 */

#include "csv_log.h"
#include <stdio.h>

int csv_log(char *buffer, size_t buffer_size,const BMI088_Data_t accel[TOTAL_IMU],
		const BMI088_Data_t gyro[TOTAL_IMU], const BMI088_Data_Fusion_t *fused)
{
	int length = snprintf(buffer, buffer_size,
    		        "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,"
    		        "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,"
					"%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,"
    		        "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\r\n",

    		        accel[0].x, accel[0].y, accel[0].z,
    		        gyro[0].x, gyro[0].y, gyro[0].z,

    		        accel[1].x, accel[1].y, accel[1].z,
    		        gyro[1].x, gyro[1].y, gyro[1].z,

    		        accel[2].x, accel[2].y, accel[2].z,
    		        gyro[2].x, gyro[2].y, gyro[2].z,

					fused->accel.x,fused->accel.y,fused->accel.z,
					fused->gyro.x,fused->gyro.y,fused->gyro.z


    		    );
    return length;

}

int csv_log_Header(char *buffer, size_t buffer_size)
{

	int length = snprintf(buffer, buffer_size,
        "imu1_ax,imu1_ay,imu1_az,"
        "imu1_gx,imu1_gy,imu1_gz,"
        "imu2_ax,imu2_ay,imu2_az,"
        "imu2_gx,imu2_gy,imu2_gz,"
        "imu3_ax,imu3_ay,imu3_az,"
        "imu3_gx,imu3_gy,imu3_gz,"
		"fused_ax,fused_ay,fused_az,"
		"fused_gx,fused_gy,fused_gz\r\n");
    return length;


}
