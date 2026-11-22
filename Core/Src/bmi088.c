/*
 * bmi088.c
 *
 *  Created on: 20-Nov-2026
 *      Author: sriramkumark
 */


#include "bmi088.h"
#include "csv_log.h"
#include "sd_log.h"

BMI088_DMA_Context_t *active_dma_context = NULL;
BMI088_DMA_Context_t imu_dma_context;
char csv_buffer[LOG_BUFFER];
int frame_length;
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

static void BMI088_DMA_PrepareBuffers(BMI088_DMA_Context_t *context)
{
    SCB_CleanDCache_by_Addr((uint32_t *)context->tx_buffer, 32);
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)context->rx_buffer, 32);
    __DSB();
}

static void BMI088_DMA_InvalidateRx(BMI088_DMA_Context_t *context)
{
    __DSB();
    SCB_InvalidateDCache_by_Addr((uint32_t *)context->rx_buffer, 32);
    __DSB();
}

static HAL_StatusTypeDef BMI088_AccRead(
    BMI088_Handle_t *imu,
    uint8_t reg,
    uint8_t *data,
    uint16_t len)
{
    uint8_t tx[8] = {0};
    uint8_t rx[8] = {0};

    if (imu == NULL || data == NULL || len == 0 || (len + 2U) > sizeof(tx))
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

    if (imu == NULL || data == NULL || len == 0 || (len + 1U) > sizeof(tx))
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

void fusion(const BMI088_Data_t accel[TOTAL_IMU],const BMI088_Data_t gyro[TOTAL_IMU],
		BMI088_Data_Fusion_t *output){

	 BMI088_Data_t accel_f[3];
	 BMI088_Data_t gyro_f[3];

	 accel_f[0].x = accel[0].x;
	 accel_f[0].y = accel[0].y;
	 accel_f[0].z = accel[0].z;

	 gyro_f[0].x = gyro[0].x;
	 gyro_f[0].y = gyro[0].y;
	 gyro_f[0].z = gyro[0].z;

	 accel_f[1].x = -accel[1].y;
	 accel_f[1].y = accel[1].x;
	 accel_f[1].z = accel[1].z;

	 gyro_f[1].x = -gyro[1].y;
	 gyro_f[1].y = gyro[1].x;
	 gyro_f[1].z = gyro[1].z;

	 accel_f[2].x = -accel[2].x;
	 accel_f[2].y = -accel[2].y;
	 accel_f[2].z = accel[2].z;

	 gyro_f[2].x = -gyro[2].x;
	 gyro_f[2].y = -gyro[2].y;
	 gyro_f[2].z = gyro[2].z;

	 output->accel.x = ( accel_f[0].x + accel_f[1].x + accel_f[2].x ) / 3.0f;
	 output->accel.y = ( accel_f[0].y + accel_f[1].y + accel_f[2].y ) / 3.0f;
	 output->accel.z = ( accel_f[0].z + accel_f[1].z + accel_f[2].z ) / 3.0f;

	 output->gyro.x = ( gyro_f[0].x + gyro_f[1].x + gyro_f[2].x) / 3.0f;
	 output->gyro.y = ( gyro_f[0].y + gyro_f[1].y + gyro_f[2].y) / 3.0f;
	 output->gyro.z = ( gyro_f[0].z + gyro_f[1].z + gyro_f[2].z) / 3.0f;

}
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (active_dma_context == NULL)
        return;

    if (hspi != active_dma_context->imu->spi)
        return;

    if (active_dma_context->sensor == BMI088_DMA_ACCEL)
    {
        BMI088_AccDeselect(active_dma_context->imu);
    }
    else
    {
        BMI088_GyroDeselect(active_dma_context->imu);
    }

    active_dma_context->state = BMI088_DMA_COMPLETE;

    active_dma_context = NULL;
}

HAL_StatusTypeDef BMI088_ReadAccel_DMA(
    BMI088_DMA_Context_t *context,
    BMI088_Handle_t *imu)
{
    if (context == NULL || imu == NULL || imu->spi == NULL)
        return HAL_ERROR;

    if (context->state == BMI088_DMA_BUSY)
        return HAL_BUSY;

    if (active_dma_context != NULL)
        return HAL_BUSY;

    context->imu = imu;
    context->sensor = BMI088_DMA_ACCEL;
    context->transfer_length = 8;
    context->data_offset = 2;

    context->tx_buffer[0] = ACC_DATA_X_LSB | 0x80;
    BMI088_DMA_PrepareBuffers(context);

    for (uint8_t i = 1; i < 8; i++)
        context->tx_buffer[i] = 0x00;

    context->state = BMI088_DMA_BUSY;
    active_dma_context = context;

    BMI088_AccSelect(imu);

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(
        imu->spi,
        context->tx_buffer,
        context->rx_buffer,
        context->transfer_length
    );

    if (status != HAL_OK)
    {
        BMI088_AccDeselect(imu);
        context->state = BMI088_DMA_ERROR;
        active_dma_context = NULL;
    }

    return status;
}

HAL_StatusTypeDef BMI088_ReadGyro_DMA(
    BMI088_DMA_Context_t *context,
    BMI088_Handle_t *imu)
{
    if (context == NULL || imu == NULL || imu->spi == NULL)
        return HAL_ERROR;

    if (context->state == BMI088_DMA_BUSY)
        return HAL_BUSY;

    if (active_dma_context != NULL)
        return HAL_BUSY;

    context->imu = imu;
    context->sensor = BMI088_DMA_GYRO;
    context->transfer_length = 7;
    context->data_offset = 1;

    context->tx_buffer[0] = GYRO_DATA_X_LSB | 0x80;
    BMI088_DMA_PrepareBuffers(context);

    for (uint8_t i = 1; i < 7; i++)
        context->tx_buffer[i] = 0x00;

    context->state = BMI088_DMA_BUSY;
    active_dma_context = context;

    BMI088_GyroSelect(imu);

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(
        imu->spi,
        context->tx_buffer,
        context->rx_buffer,
        context->transfer_length
    );

    if (status != HAL_OK)
    {
        BMI088_GyroDeselect(imu);
        context->state = BMI088_DMA_ERROR;
        active_dma_context = NULL;
    }

    return status;
}

HAL_StatusTypeDef BMI088_ProcessDMA(
    BMI088_DMA_Context_t *context)
{
    if (context == NULL || context->imu == NULL)
        return HAL_ERROR;

    if (context->state != BMI088_DMA_COMPLETE)
        return HAL_BUSY;

    BMI088_DMA_InvalidateRx(context);

    uint8_t *data = &context->rx_buffer[context->data_offset];

    context->raw.x =
        (int16_t)(((uint16_t)data[1] << 8) | data[0]);

    context->raw.y =
        (int16_t)(((uint16_t)data[3] << 8) | data[2]);

    context->raw.z =
        (int16_t)(((uint16_t)data[5] << 8) | data[4]);

    float sensitivity;

    if (context->sensor == BMI088_DMA_ACCEL)
    {
        sensitivity = BMI088_AccelSensitivity(
            context->imu->accel_range);
    }
    else
    {
        sensitivity = BMI088_GyroSensitivity(
            context->imu->gyro_range);
    }

    if (sensitivity == 0.0f)
    {
        context->state = BMI088_DMA_ERROR;
        return HAL_ERROR;
    }

    context->converted.x = (float)context->raw.x / sensitivity;
    context->converted.y = (float)context->raw.y / sensitivity;
    context->converted.z = (float)context->raw.z / sensitivity;

    context->state = BMI088_DMA_IDLE;

    return HAL_OK;
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (active_dma_context == NULL)
        return;

    if (hspi != active_dma_context->imu->spi)
        return;

    if (active_dma_context->sensor == BMI088_DMA_ACCEL)
    {
        BMI088_AccDeselect(active_dma_context->imu);
    }
    else
    {
        BMI088_GyroDeselect(active_dma_context->imu);
    }

    active_dma_context->state = BMI088_DMA_ERROR;
    active_dma_context = NULL;
}

 HAL_StatusTypeDef IMU_DMA_StartAccel(uint8_t index)
{
    switch (index)
    {
        case 0:
            return BMI088_ReadAccel_DMA(&imu_dma_context, &imu1);

        case 1:
            return BMI088_ReadAccel_DMA(&imu_dma_context, &imu2);

        case 2:
            return BMI088_ReadAccel_DMA(&imu_dma_context, &imu3);

        default:
            return HAL_ERROR;
    }
}

 HAL_StatusTypeDef IMU_DMA_StartGyro(uint8_t index)
{
    switch (index)
    {
        case 0:
            return BMI088_ReadGyro_DMA(&imu_dma_context, &imu1);

        case 1:
            return BMI088_ReadGyro_DMA(&imu_dma_context, &imu2);

        case 2:
            return BMI088_ReadGyro_DMA(&imu_dma_context, &imu3);

        default:
            return HAL_ERROR;
    }
}
 HAL_StatusTypeDef IMU_DMA_StoreResult(uint8_t index)
{
    if (index >= TOTAL_IMU)
    {
        return HAL_ERROR;
    }

    if (BMI088_ProcessDMA(&imu_dma_context) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (imu_dma_context.sensor == BMI088_DMA_ACCEL)
    {
        accel_raw[index] = imu_dma_context.raw;
        accel_data[index] = imu_dma_context.converted;
    }
    else if (imu_dma_context.sensor == BMI088_DMA_GYRO)
    {
        gyro_raw[index] = imu_dma_context.raw;
        gyro_data[index] = imu_dma_context.converted;
    }
    else
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}
 void IMU_DMA_Task(void)
{
    HAL_StatusTypeDef status;

    switch (imu_dma_state)
    {
        case IMU1_ACC_START:
            status = IMU_DMA_StartAccel(0);
            if (status == HAL_OK)
                imu_dma_state = IMU1_ACC_WAIT;
            else
                imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU1_ACC_WAIT:
            if (imu_dma_context.state == BMI088_DMA_COMPLETE)
            {
                if (IMU_DMA_StoreResult(0) == HAL_OK)
                    imu_dma_state = IMU1_GYRO_START;
                else
                    imu_dma_state = IMU_DMA_IDLE;
            }
            else if (imu_dma_context.state == BMI088_DMA_ERROR)
                imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU1_GYRO_START:
            status = IMU_DMA_StartGyro(0);
            if (status == HAL_OK)
                imu_dma_state = IMU1_GYRO_WAIT;
            else
                imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU1_GYRO_WAIT:
            if (imu_dma_context.state == BMI088_DMA_COMPLETE)
            {
                if (IMU_DMA_StoreResult(0) == HAL_OK)
                    imu_dma_state = IMU2_ACC_START;
                else
                    imu_dma_state = IMU_DMA_IDLE;
            }
            else if (imu_dma_context.state == BMI088_DMA_ERROR)
                imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU2_ACC_START:
            status = IMU_DMA_StartAccel(1);
            if (status == HAL_OK)
                imu_dma_state = IMU2_ACC_WAIT;
            else
                imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU2_ACC_WAIT:
            if (imu_dma_context.state == BMI088_DMA_COMPLETE)
            {
                if (IMU_DMA_StoreResult(1) == HAL_OK)
                    imu_dma_state = IMU2_GYRO_START;
                else
                    imu_dma_state = IMU_DMA_IDLE;
            }
            else if (imu_dma_context.state == BMI088_DMA_ERROR)
                imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU2_GYRO_START:
            status = IMU_DMA_StartGyro(1);
            if (status == HAL_OK)
                imu_dma_state = IMU2_GYRO_WAIT;
            else
                imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU2_GYRO_WAIT:
            if (imu_dma_context.state == BMI088_DMA_COMPLETE)
            {
                if (IMU_DMA_StoreResult(1) == HAL_OK)
                    imu_dma_state = IMU3_ACC_START;
                else
                    imu_dma_state = IMU_DMA_IDLE;
            }
            else if (imu_dma_context.state == BMI088_DMA_ERROR)
                imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU3_ACC_START:
            status = IMU_DMA_StartAccel(2);
            if (status == HAL_OK)
                imu_dma_state = IMU3_ACC_WAIT;
            else
                imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU3_ACC_WAIT:
            if (imu_dma_context.state == BMI088_DMA_COMPLETE)
            {
                if (IMU_DMA_StoreResult(2) == HAL_OK)
                    imu_dma_state = IMU3_GYRO_START;
                else
                    imu_dma_state = IMU_DMA_IDLE;
            }
            else if (imu_dma_context.state == BMI088_DMA_ERROR)
                imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU3_GYRO_START:
            status = IMU_DMA_StartGyro(2);
            if (status == HAL_OK)
                imu_dma_state = IMU3_GYRO_WAIT;
            else
                imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU3_GYRO_WAIT:
            if (imu_dma_context.state == BMI088_DMA_COMPLETE)
            {
                if (IMU_DMA_StoreResult(2) == HAL_OK)
                    imu_dma_state = IMU_DMA_FUSION;
                else
                    imu_dma_state = IMU_DMA_IDLE;
            }
            else if (imu_dma_context.state == BMI088_DMA_ERROR)
                imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU_DMA_FUSION:
            fusion(accel_data, gyro_data, &fused_data);
            imu_dma_state = IMU_DMA_LOG;
            break;

        case IMU_DMA_LOG:
            frame_length = csv_log(
                csv_buffer,
                sizeof(csv_buffer),
                accel_data,
                gyro_data,
                &fused_data
            );

            if (frame_length > 0 &&
                frame_length < sizeof(csv_buffer))
            {
                SD_Logger_Write(csv_buffer, (uint16_t)frame_length);
            }

            imu_dma_state = IMU_DMA_IDLE;
            break;

        case IMU_DMA_IDLE:
        default:
            break;
    }
}
