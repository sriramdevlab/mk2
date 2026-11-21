/*
 * bmi088.c
 *
 *  Created on: 20-Nov-2026
 *      Author: sriramkumark
 */


#include "bmi088.h"


static void BMI088_AccSelect(BMI088_Handle_t *imu)
{
    HAL_GPIO_WritePin(imu->acc_cs_port,
                      imu->acc_cs_pin,
                      GPIO_PIN_RESET);
}

static void BMI088_AccDeselect(BMI088_Handle_t *imu)
{
    HAL_GPIO_WritePin(imu->acc_cs_port,
                      imu->acc_cs_pin,
                      GPIO_PIN_SET);
}

static void BMI088_GyroSelect(BMI088_Handle_t *imu)
{
    HAL_GPIO_WritePin(imu->gyro_cs_port,
                      imu->gyro_cs_pin,
                      GPIO_PIN_RESET);
}

static void BMI088_GyroDeselect(BMI088_Handle_t *imu)
{
    HAL_GPIO_WritePin(imu->gyro_cs_port,
                      imu->gyro_cs_pin,
                      GPIO_PIN_SET);
}


static HAL_StatusTypeDef BMI088_AccRead(
    BMI088_Handle_t *imu,
    uint8_t reg,
    uint8_t *data,
    uint16_t len)
{
    uint8_t tx[8] = {0};
    uint8_t rx[8] = {0};

    if (imu == NULL || data == NULL || len == 0 || len > 30)
        return HAL_ERROR;

    tx[0] = reg | 0x80;

    BMI088_AccSelect(imu);

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(
        imu->spi, tx, rx, len + 2, BMI088_SPI_TIMEOUT);

    BMI088_AccDeselect(imu);

    if (status != HAL_OK)
        return status;

    for (uint16_t i = 0; i < len; i++)
        data[i] = rx[i + 2];

    return HAL_OK;
}


static HAL_StatusTypeDef BMI088_GyroRead(
    BMI088_Handle_t *imu,
    uint8_t reg,
    uint8_t *data,
    uint16_t len)
{
    uint8_t tx[8] = {0};
    uint8_t rx[8] = {0};

    if (imu == NULL || data == NULL || len == 0 || len > 31)
        return HAL_ERROR;

    tx[0] = reg | 0x80;

    BMI088_GyroSelect(imu);

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(
        imu->spi, tx, rx, len + 1, BMI088_SPI_TIMEOUT);

    BMI088_GyroDeselect(imu);

    if (status != HAL_OK)
        return status;

    for (uint16_t i = 0; i < len; i++)
        data[i] = rx[i + 1];

    return HAL_OK;
}


static HAL_StatusTypeDef BMI088_AccWrite(
    BMI088_Handle_t *imu,
    uint8_t reg,
    uint8_t value)
{
    uint8_t tx[2];

    tx[0] = reg & 0x7F;
    tx[1] = value;

    BMI088_AccSelect(imu);

    HAL_StatusTypeDef status = HAL_SPI_Transmit(
        imu->spi, tx, 2, BMI088_SPI_TIMEOUT);

    BMI088_AccDeselect(imu);

    return status;
}


static HAL_StatusTypeDef BMI088_GyroWrite(
    BMI088_Handle_t *imu,
    uint8_t reg,
    uint8_t value)
{
    uint8_t tx[2];

    tx[0] = reg & 0x7F;
    tx[1] = value;

    BMI088_GyroSelect(imu);

    HAL_StatusTypeDef status = HAL_SPI_Transmit(
        imu->spi, tx, 2, BMI088_SPI_TIMEOUT);

    BMI088_GyroDeselect(imu);

    return status;
}


HAL_StatusTypeDef BMI088_ReadChipID(BMI088_Handle_t *imu)
{
    if (imu == NULL || imu->spi == NULL)
        return HAL_ERROR;

    if (BMI088_AccRead(
        imu, ACC_CHIP_ID, &imu->acc_id, 1) != HAL_OK)
        return HAL_ERROR;

    if (BMI088_GyroRead(
        imu, GYRO_CHIP_ID, &imu->gyro_id, 1) != HAL_OK)
        return HAL_ERROR;

    if (imu->acc_id != ACC_ID_VALUE)
        return HAL_ERROR;

    if (imu->gyro_id != GYRO_ID_VALUE)
        return HAL_ERROR;

    return HAL_OK;
}


HAL_StatusTypeDef BMI088_Init(BMI088_Handle_t *imu)
{
    if (imu == NULL || imu->spi == NULL ||
        imu->acc_cs_port == NULL ||
        imu->gyro_cs_port == NULL)
        return HAL_ERROR;


    BMI088_AccDeselect(imu);
    BMI088_GyroDeselect(imu);

    HAL_Delay(10);


    uint8_t dummy_id = 0;

    if (BMI088_AccRead(
        imu, ACC_CHIP_ID, &dummy_id, 1) != HAL_OK)
        return HAL_ERROR;

    HAL_Delay(1);


    if (BMI088_AccWrite(
        imu, ACC_SOFTRESET, SOFT_RESET_CMD) != HAL_OK)
        return HAL_ERROR;

    HAL_Delay(2);


    if (BMI088_GyroWrite(
        imu, GYRO_SOFTRESET, SOFT_RESET_CMD) != HAL_OK)
        return HAL_ERROR;

    HAL_Delay(30);

    if (BMI088_AccWrite(imu, ACC_PWR_CONF, ACC_PWR_CONF_ACTIVE) != HAL_OK)
        return HAL_ERROR;

    HAL_Delay(5);

    if (BMI088_AccWrite(imu, ACC_PWR_CTRL, ACC_PWR_CTRL_NORMAL) != HAL_OK)
        return HAL_ERROR;

    HAL_Delay(50);

    if (BMI088_AccWrite(
        imu, ACC_RANGE, imu->accel_range) != HAL_OK)
        return HAL_ERROR;

    if (BMI088_AccWrite(imu, ACC_CONF, ACC_NORMAL_100HZ) != HAL_OK)
        return HAL_ERROR;

    if (BMI088_GyroWrite(
        imu, GYRO_RANGE, imu->gyro_range) != HAL_OK)
        return HAL_ERROR;

    if (BMI088_GyroWrite(
        imu, GYRO_BANDWIDTH, GYRO_BANDWIDTH_116HZ) != HAL_OK)
        return HAL_ERROR;

    if (BMI088_GyroWrite(
        imu, GYRO_LPM1, GYRO_LPM1_NORMAL) != HAL_OK)
        return HAL_ERROR;

    HAL_Delay(10);

    return BMI088_ReadChipID(imu);
}


static float BMI088_AccelSensitivity(
    BMI088_AccelRange_t range)
{
    switch (range)
    {
        case BMI088_ACC_RANGE_3G:  return 10920.0f;
        case BMI088_ACC_RANGE_6G:  return 5460.0f;
        case BMI088_ACC_RANGE_12G: return 2730.0f;
        case BMI088_ACC_RANGE_24G: return 1365.0f;
        default: return 0.0f;
    }
}



HAL_StatusTypeDef BMI088_ReadAccel(
    BMI088_Handle_t *imu,
    BMI088_RawData_t *raw,
    BMI088_Data_t *converted)
{
    uint8_t buffer[6];

    if (imu == NULL || raw == NULL || converted == NULL)
        return HAL_ERROR;

    if (BMI088_AccRead(
        imu, ACC_DATA_X_LSB, buffer, 6) != HAL_OK)
        return HAL_ERROR;

    raw->x = (int16_t)(((uint16_t)buffer[1] << 8) | buffer[0]);
    raw->y = (int16_t)(((uint16_t)buffer[3] << 8) | buffer[2]);
    raw->z = (int16_t)(((uint16_t)buffer[5] << 8) | buffer[4]);

    float sensitivity = BMI088_AccelSensitivity(imu->accel_range);

    if (sensitivity == 0.0f)
        return HAL_ERROR;

    converted->x = (float)raw->x / sensitivity;
    converted->y = (float)raw->y / sensitivity;
    converted->z = (float)raw->z / sensitivity;

    return HAL_OK;
}



static float BMI088_GyroSensitivity(
    BMI088_GyroRange_t range)
{
    switch (range)
    {
        case BMI088_GYRO_RANGE_2000DPS: return 16.4f;
        case BMI088_GYRO_RANGE_1000DPS: return 32.8f;
        case BMI088_GYRO_RANGE_500DPS:  return 65.6f;
        case BMI088_GYRO_RANGE_250DPS:  return 131.2f;
        case BMI088_GYRO_RANGE_125DPS:  return 262.4f;
        default: return 0.0f;
    }
}


HAL_StatusTypeDef BMI088_ReadGyro(
    BMI088_Handle_t *imu,
    BMI088_RawData_t *raw,
    BMI088_Data_t *converted)
{
    uint8_t buffer[6];

    if (imu == NULL || raw == NULL || converted == NULL)
        return HAL_ERROR;

    if (BMI088_GyroRead(
        imu, GYRO_DATA_X_LSB, buffer, 6) != HAL_OK)
        return HAL_ERROR;

    raw->x = (int16_t)(((uint16_t)buffer[1] << 8) | buffer[0]);
    raw->y = (int16_t)(((uint16_t)buffer[3] << 8) | buffer[2]);
    raw->z = (int16_t)(((uint16_t)buffer[5] << 8) | buffer[4]);

    float sensitivity = BMI088_GyroSensitivity(imu->gyro_range);

    if (sensitivity == 0.0f)
        return HAL_ERROR;

    converted->x = (float)raw->x / sensitivity;
    converted->y = (float)raw->y / sensitivity;
    converted->z = (float)raw->z / sensitivity;

    return HAL_OK;
}

HAL_StatusTypeDef BMI088_ReadTemperature(
    BMI088_Handle_t *imu,
    float *temperature)
{
    uint8_t buffer[2];

    if (imu == NULL || temperature == NULL)
        return HAL_ERROR;

    if (BMI088_AccRead(
        imu, TEMP_LSB, buffer, 2) != HAL_OK)
        return HAL_ERROR;

    int16_t temp_raw =
        (int16_t)(((uint16_t)buffer[1] << 3) |
                  (buffer[0] >> 5));

    if (temp_raw & 0x0400)
        temp_raw |= (int16_t)0xF800;

    *temperature = 23.0f + ((float)temp_raw * 0.125f);

    return HAL_OK;
}
