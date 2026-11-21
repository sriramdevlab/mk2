/*
 * bmi088.h
 *
 *  Created on: 20-Nov-2026
 *      Author: sriramkumark
 */


#ifndef INC_BMI088_H_
#define INC_BMI088_H_

#include "stm32h7xx_hal.h"
#include <stdint.h>

#define ACC_ID_VALUE             0x1E
#define GYRO_ID_VALUE            0x0F

#define ACC_CHIP_ID              0x00
#define ACC_DATA_X_LSB           0x12
#define ACC_DATA_Y_LSB           0x14
#define ACC_DATA_Z_LSB           0x16
#define ACC_CONF                 0x40
#define ACC_RANGE                0x41
#define ACC_PWR_CONF             0x7C
#define ACC_PWR_CTRL             0x7D
#define ACC_SOFTRESET            0x7E

#define ACC_PWR_CONF_ACTIVE      0x00
#define ACC_PWR_CTRL_NORMAL      0x04

#define ACC_NORMAL_100HZ         0XA8

#define GYRO_CHIP_ID             0x00
#define GYRO_DATA_X_LSB          0x02
#define GYRO_DATA_Y_LSB          0x04
#define GYRO_DATA_Z_LSB          0x06
#define GYRO_RANGE               0x0F
#define GYRO_BANDWIDTH           0x10
#define GYRO_LPM1                0x11
#define GYRO_SOFTRESET           0x14

#define GYRO_BANDWIDTH_116HZ     0x02
#define GYRO_LPM1_NORMAL		 0X00

#define TEMP_LSB                 0x22
#define TEMP_MSB                 0x23

#define SOFT_RESET_CMD           0xB6
#define BMI088_SPI_TIMEOUT       100

typedef enum
{
    BMI088_ACC_RANGE_3G  = 0x00,
    BMI088_ACC_RANGE_6G  = 0x01,
    BMI088_ACC_RANGE_12G = 0x02,
    BMI088_ACC_RANGE_24G = 0x03

} BMI088_AccelRange_t;


typedef enum
{
    BMI088_GYRO_RANGE_2000DPS = 0x00,
    BMI088_GYRO_RANGE_1000DPS = 0x01,
    BMI088_GYRO_RANGE_500DPS  = 0x02,
    BMI088_GYRO_RANGE_250DPS  = 0x03,
    BMI088_GYRO_RANGE_125DPS  = 0x04

} BMI088_GyroRange_t;


typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;

} BMI088_RawData_t;


typedef struct
{
    float x;
    float y;
    float z;

} BMI088_Data_t;


typedef struct
{
    SPI_HandleTypeDef *spi;

    GPIO_TypeDef *acc_cs_port;
    uint16_t acc_cs_pin;

    GPIO_TypeDef *gyro_cs_port;
    uint16_t gyro_cs_pin;

    BMI088_AccelRange_t accel_range;
    BMI088_GyroRange_t gyro_range;

    uint8_t acc_id;
    uint8_t gyro_id;

} BMI088_Handle_t;



HAL_StatusTypeDef BMI088_Init(BMI088_Handle_t *imu);

HAL_StatusTypeDef BMI088_ReadChipID(BMI088_Handle_t *imu);

HAL_StatusTypeDef BMI088_ReadAccel(
    BMI088_Handle_t *imu,
    BMI088_RawData_t *raw,
    BMI088_Data_t *converted);

HAL_StatusTypeDef BMI088_ReadGyro(
    BMI088_Handle_t *imu,
    BMI088_RawData_t *raw,
    BMI088_Data_t *converted);

HAL_StatusTypeDef BMI088_ReadTemperature(
    BMI088_Handle_t *imu,
    float *temperature);

#endif /* INC_BMI088_H_ */

