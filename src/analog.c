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
void DacWriteOutput(uint32_t unit, uint32_t counts)
{
    union analog_set_frame spi_cmd;

    switch (unit)
    {
    case 1:
        gDac.ActiveCountsReset = counts & 0x007FFFFF;
        spi_cmd.rw = 1;
        spi_cmd.address = 0x0;
        spi_cmd.microvolts = gDac.ActiveCountsReset;
        spi_cmd.reserved = 0;
        spi_cmd.checksum = 0;
        spi_cmd.checksum = __builtin_parity(*(unsigned int *)&spi_cmd);
        HAL_SPI_Transmit(&hspi1, spi_cmd.frame, sizeof(spi_cmd), HAL_MAX_DELAY);
        // printf("[DacWriteOutput] RESET counts=%ld\n", counts);
        break;
    case 2:
        gDac.ActiveCountsWpAcc = counts & 0x007FFFFF;
        spi_cmd.rw = 1;
        spi_cmd.address = 0x1;
        spi_cmd.microvolts = gDac.ActiveCountsWpAcc;
        spi_cmd.reserved = 0;
        spi_cmd.checksum = 0;
        spi_cmd.checksum = __builtin_parity(*(unsigned int *)&spi_cmd);
        HAL_SPI_Transmit(&hspi1, spi_cmd.frame, sizeof(spi_cmd), HAL_MAX_DELAY);
        // printf("[DacWriteOutput] WP counts=%ld\n", counts);
        break;
    default:
        // printf("[DacWriteOUtput] Invalid channel %ld\n", unit);
        break;
    }

    // printf("[DacWriteOutputs] Write reset=% 4ldmV wp=% 4ldmV\n", gDac.ActiveCountsReset, gDac.ActiveCountsWpAcc);
}
