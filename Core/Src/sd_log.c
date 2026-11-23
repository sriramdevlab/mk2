/*
 * sd_log.c
 *
 *  Created on: 21-Nov-2026
 *      Author: sriramkumark
 */
#include "main.h"

#include "sd_log.h"
#include "fatfs.h"
#include "csv_log.h"
#include <stdio.h>
#include "string.h"
#include "sd_diskio.h"
#include "bsp_driver_sd.h"
extern SD_HandleTypeDef hsd1;
__attribute__((section(".RAM_D2"), aligned(32)))
static RAM_LogBuffer_t log_buffers[2];

#define SD_DMA_BUFFER_SIZE 4096U

__attribute__((section(".RAM_D1"), aligned(32)))
static uint8_t sd_dma_buffer[SD_DMA_BUFFER_SIZE];

static int8_t filling_buffer = -1;

static uint8_t sd_file_open = 0U;
volatile uint8_t sd_write_complete = 0U;
volatile uint8_t sd_write_error = 0U;
FRESULT SD_Logger_Init(void)
{
    FRESULT result;
    char file_path[32];
    char header[LOG_BUFFER];
    int header_length;
    UINT bytes_written;

    if (retSD != 0U)
    {
        return FR_NOT_READY;
    }

    result = f_mount(&SDFatFS, (TCHAR const *)SDPath, 1);

    if (result != FR_OK)
    {
        return result;
    }

    snprintf(file_path, sizeof(file_path),
             "%s/BMI088_Log.CSV", SDPath);

    result = f_open(&SDFile, file_path, FA_OPEN_ALWAYS | FA_WRITE);

    if (result != FR_OK)
    {
        return result;
    }

    sd_file_open = 1U;

    if (f_size(&SDFile) == 0U)
    {
        header_length = csv_log_Header(
            header, sizeof(header));

        if (header_length <= 0)
        {
            SD_Logger_Close();
            return FR_INVALID_PARAMETER;
        }

        result = f_write(&SDFile, header,
                         (UINT)header_length, &bytes_written);

        if (result != FR_OK ||
            bytes_written != (UINT)header_length)
        {
            SD_Logger_Close();
            return (result != FR_OK) ? result : FR_DISK_ERR;
        }
    }

    result = f_lseek(&SDFile, f_size(&SDFile));

    if (result != FR_OK)
    {
        SD_Logger_Close();
        return result;
    }

    return FR_OK;
}

FRESULT SD_Logger_Write(const char *buffer, uint16_t length)
{
    FRESULT result;
    UINT bytes_written;

    if ((buffer == NULL) || (sd_file_open == 0U))
    {
        return FR_INVALID_OBJECT;
    }

    result = f_lseek(&SDFile, f_size(&SDFile));

    if (result != FR_OK)
    {
        return result;
    }

    result = f_write(&SDFile, buffer, (UINT)length,
                     &bytes_written);

    if (result != FR_OK)
    {
        return result;
    }

    if (bytes_written != (UINT)length)
    {
        return FR_DISK_ERR;
    }

    return f_sync(&SDFile);
}

FRESULT SD_Logger_Close(void)
{
    FRESULT result;

    if (sd_file_open == 0U)
    {
        return FR_OK;
    }

    result = f_close(&SDFile);

    if (result == FR_OK)
    {
        sd_file_open = 0U;
    }

    return result;
}

void RAM_Log_Init(void)
{
    memset(log_buffers, 0, sizeof(log_buffers));

    log_buffers[0].state = RAM_LOG_FILLING;
    log_buffers[1].state = RAM_LOG_FREE;

    filling_buffer = 0;
}

bool RAM_Log_Append(const char *data, uint16_t length)
{
    if (data == NULL || length == 0U ||
        length > RAM_LOG_BUFFER_SIZE)
    {
        return false;
    }

    if (filling_buffer < 0)
    {
        return false;
    }

    RAM_LogBuffer_t *buffer = &log_buffers[filling_buffer];

    if ((uint32_t)buffer->length + length > RAM_LOG_BUFFER_SIZE)
    {
        buffer->state = RAM_LOG_READY;

        int8_t next_buffer = -1;

        for (uint8_t i = 0; i < 2U; i++)
        {
            if (log_buffers[i].state == RAM_LOG_FREE)
            {
                next_buffer = (int8_t)i;
                break;
            }
        }

        if (next_buffer < 0)
        {
            filling_buffer = -1;
            return false;
        }

        filling_buffer = next_buffer;
        log_buffers[filling_buffer].length = 0U;
        log_buffers[filling_buffer].state = RAM_LOG_FILLING;

        buffer = &log_buffers[filling_buffer];
    }

    memcpy(&buffer->data[buffer->length], data, length);
    buffer->length += length;

    return true;
}

RAM_LogBuffer_t *RAM_Log_GetReady(void)
{
    for (uint8_t i = 0; i < 2U; i++)
    {
        if (log_buffers[i].state == RAM_LOG_READY)
        {
            log_buffers[i].state = RAM_LOG_WRITING;
            return &log_buffers[i];
        }
    }

    return NULL;
}

void RAM_Log_Release(RAM_LogBuffer_t *buffer)
{
    if (buffer == NULL)
    {
        return;
    }

    for (uint8_t i = 0; i < 2U; i++)
    {
        if (buffer == &log_buffers[i] &&
            buffer->state == RAM_LOG_WRITING)
        {
            buffer->length = 0U;
            buffer->state = RAM_LOG_FREE;
            return;
        }
    }
}

void BSP_SD_WriteCpltCallback(void)
{
    sd_write_complete = 1U;
}

void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
{
    if (hsd == &hsd1)
    {
        sd_write_error = 1U;
    }
}
uint8_t BSP_SD_WriteBlocks(uint32_t *pData,
                           uint32_t WriteAddr,
                           uint32_t NumOfBlocks,
                           uint32_t Timeout)
{
    uint32_t start_tick;
    uint32_t blocks_to_write;
    uint32_t remaining = NumOfBlocks;
    uint32_t sector = WriteAddr;

    if (pData == NULL || NumOfBlocks == 0U)
    {
        return MSD_ERROR;
    }

    while (remaining > 0U)
    {
        blocks_to_write = (remaining > 8U) ? 8U : remaining;

        memcpy(sd_dma_buffer,
               (uint8_t *)pData,
               blocks_to_write * 512U);

        sd_write_complete = 0U;
        sd_write_error = 0U;

        start_tick = HAL_GetTick();

        if (HAL_SD_WriteBlocks_DMA(&hsd1,
                                   sd_dma_buffer,
                                   sector,
                                   blocks_to_write) != HAL_OK)
        {
            return MSD_ERROR;
        }

        while ((sd_write_complete == 0U) &&
               (sd_write_error == 0U))
        {
            if ((HAL_GetTick() - start_tick) >= Timeout)
            {
                return MSD_ERROR;
            }
        }

        if (sd_write_error != 0U)
        {
            return MSD_ERROR;
        }

        while (HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER)
        {
            if ((HAL_GetTick() - start_tick) >= Timeout)
            {
                return MSD_ERROR;
            }
        }

        pData += blocks_to_write * 128U;
        sector += blocks_to_write;
        remaining -= blocks_to_write;
    }

    return MSD_OK;
}
