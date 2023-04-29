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
    float    Dac1CalC0; // DAC 1 calibration constant coefficient
    float    Dac1CalC1; // DAC 1 calibration linear coefficient
    float    Dac2CalC0; // DAC 1 calibration constant coefficient
    float    Dac2CalC1; // DAC 1 calibration linear coefficient
} S_DacVariables;

extern S_DacVariables gDac;

typedef enum {
    DAC_SUCCESS = 0, // Success
    DAC_ERR_TIMEOUT, // Timeout while writing to DAC
    DAC_ERR_HW,      // DAC hardware error
    DAC_ERR_SPI,     // SPI error
    DAC_ERR_INVALID_CHANNEL // Invalid DAC channel
} DacError;

extern DacError DacInit(float Dac1CalC0, float Dac1CalC1, float Dac2CalC0, float Dac2CalC1);
extern DacError DacWriteOutput(uint32_t unit, uint32_t counts);

// #define			SET_RESET_VOLTAGE(V)		gDac.ActiveCountsReset = (uint16_t)(gRamConfig.Ana_Reset10VCnts * ((float)(V)/10.0))
// #define			SET_WP_ACC_VOLTAGE(V)		gDac.ActiveCountsWpAcc = (uint16_t)(gRamConfig.Ana_WpAcc10VCnts * ((float)(V)/10.0))

#define DAC1_CALIBRATED(V) (gDac.Dac1CalC0 + gDac.Dac1CalC1*((float)(V)))
#define DAC2_CALIBRATED(V) (gDac.Dac2CalC0 + gDac.Dac2CalC1*((float)(V)))
#define SET_RESET_MV(V) DacWriteOutput(1, DAC1_CALIBRATED(V)*128)
#define SET_WP_ACC_MV(V) DacWriteOutput(2, DAC2_CALIBRATED(V)*(128/2))

#define WP_ACC_LOW SET_WP_ACC_MV(0)
#define WP_ACC_HIGH SET_WP_ACC_MV(3300)
#define RESET_LOW SET_RESET_MV(0)
#define RESET_HIGH SET_RESET_MV(3300)

#if defined(__cplusplus)
}
#endif

#endif // !ANALOG_H