/**
 * @file analog.c
 * @brief DAC control routines
 *
 * This module manages the analog board, which has two 24-bit DAC channels.
 * Channel 1 is RESET# / Vwl
 * Channel 2 is WP#/ACC
 * The ch2 voltage is doubled on the interface board
 */

#include "analog.h"
#include "stm32h7xx_hal.h"
#include "main.h"

// globals
S_DacVariables gDac;

// dac management
extern SPI_HandleTypeDef hspi1; // defined in main.c
union analog_set_frame {
    uint8_t frame[4];
    struct
    {
        uint32_t rw         : 1;
        uint32_t address    : 4;
        uint32_t microvolts : 23;
        uint32_t reserved   : 3;
        uint32_t checksum   : 1;
    };
};

DacError DacInit(float Dac1CalC0, float Dac1CalC1, float Dac2CalC0, float Dac2CalC1)
{
    gDac.Dac1CalC0 = Dac1CalC0;
    gDac.Dac1CalC1 = Dac1CalC1;
    gDac.Dac2CalC0 = Dac2CalC0;
    gDac.Dac2CalC1 = Dac2CalC1;

    DacError err = DAC_SUCCESS;
    err = DacWriteOutput(1, 0);
    if (err != DAC_SUCCESS) return err;
    err = DacWriteOutput(2, 0);
    if (err != DAC_SUCCESS) return err;

    return DAC_SUCCESS;
}

static HAL_StatusTypeDef dac_send(uint32_t unit, uint32_t counts)
{
    union analog_set_frame spi_cmd;

    switch (unit) {
        case 1:
            spi_cmd.microvolts = gDac.ActiveCountsReset = counts & 0x007FFFFF;
            spi_cmd.address = 0x0;
            break;
        case 2:
            spi_cmd.microvolts = gDac.ActiveCountsWpAcc = counts & 0x007FFFFF;
            spi_cmd.address = 0x1;
            break;
    }

    spi_cmd.rw = 1;
    spi_cmd.checksum = 0;
    spi_cmd.checksum = __builtin_parity(*(unsigned int *)&spi_cmd);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, spi_cmd.frame, sizeof(spi_cmd), HAL_MAX_DELAY);
    return status;
}

DacError DacWriteOutput(uint32_t unit, uint32_t counts)
{
    DacError ret = DAC_SUCCESS;

    GPIO_PinState dac_err_state = HAL_GPIO_ReadPin(ANA_ERR_GPIO_Port, ANA_ERR_Pin);
    if (dac_err_state == GPIO_PIN_SET)
        return DAC_ERR_HW;

    if (unit == 1 || unit == 2) {
        HAL_StatusTypeDef status = dac_send(unit, counts);
        if (status == HAL_TIMEOUT)
            ret = DAC_ERR_TIMEOUT;
        else if (status != HAL_OK)
            ret = DAC_ERR_SPI;
    } else {
        ret = DAC_ERR_INVALID_CHANNEL;
    }

    return ret;
}
