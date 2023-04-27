/**
 * @file analog.h
 * @brief DAC control routines
 *
 * This module manages the analog board, which has two 24-bit DAC channels.
 * Channel 1 is RESET# / Vwl
 * Channel 2 is WP#/ACC
 * The ch2 voltage is doubled on the interface board
 */

#ifndef ANALOG_H
#define ANALOG_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include <stdint.h>

typedef struct
{
    uint32_t ActiveCountsReset;
    uint32_t ActiveCountsWpAcc;
} S_DacVariables;

extern S_DacVariables gDac;

extern void DacWriteOutput(uint32_t unit, uint32_t counts);

// #define			SET_RESET_VOLTAGE(V)		gDac.ActiveCountsReset = (uint16_t)(gRamConfig.Ana_Reset10VCnts * ((float)(V)/10.0))
// #define			SET_WP_ACC_VOLTAGE(V)		gDac.ActiveCountsWpAcc = (uint16_t)(gRamConfig.Ana_WpAcc10VCnts * ((float)(V)/10.0))

#define SET_RESET_MV(V) DacWriteOutput(1, (V)*128)
#define SET_WP_ACC_MV(V) DacWriteOutput(2, (V)*128 / 2)

#define WP_ACC_LOW SET_WP_ACC_MV(0)
#define WP_ACC_HIGH SET_WP_ACC_MV(3300)
#define RESET_LOW SET_RESET_MV(0)
#define RESET_HIGH SET_RESET_MV(3300)

#if defined(__cplusplus)
}
#endif

#endif // !ANALOG_H