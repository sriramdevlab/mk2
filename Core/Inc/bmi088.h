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

#define TOTAL_IMU 3

#define ACC_ID_VALUE             0x1E
#define GYRO_ID_VALUE            0x0F

#define ACC_CHIP_ID              0x00
#define ACC_DATA_X_LSB           0x12
#define ACC_DATA_X_MSB           0x13
#define ACC_DATA_Y_LSB           0x14
#define ACC_DATA_Y_MSB           0x15
#define ACC_DATA_Z_LSB           0x16
#define ACC_DATA_Z_MSB           0x17
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
#define GYRO_DATA_X_MSB          0x03
#define GYRO_DATA_Y_LSB          0x04
#define GYRO_DATA_Y_MSB          0x05
#define GYRO_DATA_Z_LSB          0x06
#define GYRO_DATA_Z_MSB          0x07
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
	BMI088_Data_t accel;
	BMI088_Data_t gyro;

}BMI088_Data_Fusion_t;

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

extern BMI088_Handle_t imu1;
extern BMI088_Handle_t imu2;
extern BMI088_Handle_t imu3;
extern BMI088_RawData_t accel_raw[3];
extern BMI088_RawData_t gyro_raw[3];
extern BMI088_Data_t accel_data[3];
extern BMI088_Data_t gyro_data[3];
extern BMI088_Data_Fusion_t fused_data;

typedef enum
{
    IMU_DMA_IDLE = 0,

    IMU1_ACC_START,
    IMU1_ACC_WAIT,

    IMU1_GYRO_START,
    IMU1_GYRO_WAIT,

    IMU2_ACC_START,
    IMU2_ACC_WAIT,

    IMU2_GYRO_START,
    IMU2_GYRO_WAIT,

    IMU3_ACC_START,
    IMU3_ACC_WAIT,

    IMU3_GYRO_START,
    IMU3_GYRO_WAIT,

    IMU_DMA_FUSION,
    IMU_DMA_LOG

} IMU_DMA_State_t;

typedef enum
{
    BMI088_DMA_IDLE = 0,
    BMI088_DMA_BUSY,
    BMI088_DMA_COMPLETE,
    BMI088_DMA_ERROR

} BMI088_DMA_State_t;

typedef enum
{
    BMI088_DMA_ACCEL = 0,
    BMI088_DMA_GYRO

} BMI088_DMA_Sensor_t;

typedef struct
{
    uint8_t tx_buffer[32] __attribute__((aligned(32)));
    uint8_t rx_buffer[32] __attribute__((aligned(32)));

    BMI088_Handle_t *imu;

    volatile BMI088_DMA_State_t state;

    BMI088_DMA_Sensor_t sensor;

    uint16_t transfer_length;
    uint8_t data_offset;

    BMI088_RawData_t raw;
    BMI088_Data_t converted;

} BMI088_DMA_Context_t;

extern volatile IMU_DMA_State_t imu_dma_state;

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

void fusion(const BMI088_Data_t accel[TOTAL_IMU],const BMI088_Data_t gyro[TOTAL_IMU],
		BMI088_Data_Fusion_t *output);

HAL_StatusTypeDef BMI088_ReadAccel_DMA(
    BMI088_DMA_Context_t *context,
    BMI088_Handle_t *imu
);

HAL_StatusTypeDef BMI088_ReadGyro_DMA(
    BMI088_DMA_Context_t *context,
    BMI088_Handle_t *imu
);

HAL_StatusTypeDef BMI088_ProcessDMA(
    BMI088_DMA_Context_t *context
);
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi);
 HAL_StatusTypeDef IMU_DMA_StartAccel(uint8_t index);
 HAL_StatusTypeDef IMU_DMA_StartGyro(uint8_t index);
 HAL_StatusTypeDef IMU_DMA_StoreResult(uint8_t index);
 void IMU_DMA_Task(void);


#endif /* INC_BMI088_H_ */

