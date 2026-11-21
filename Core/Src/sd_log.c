/*
 * sd_log.c
 *
 *  Created on: 21-Nov-2026
 *      Author: sriramkumark
 */
#include "sd_log.h"
#include "fatfs.h"
#include "csv_log.h"
#include <stdio.h>

static uint8_t sd_file_open = 0U;

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
