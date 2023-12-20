/**
 * @file det_driver_qspi.h
 * @brief QSPI detector read/write driver
 */

#ifndef DET_DRIVER_QSPI_H
#define DET_DRIVER_QSPI_H

#include "det_ctrl.h"

extern void	    QSPI_Open(void);
extern void	    QSPI_Close(void);

extern uint16_t	QSPI_ReadWord(uint32_t addr);
extern void	    QSPI_ReadWords(uint32_t *addrs, uint16_t *dest, uint32_t count);
extern void	    QSPI_ReadBlock(uint32_t base, uint32_t count, uint16_t *dest);
extern void	    QSPI_ReadPage(uint32_t PageAddress, uint16_t *dest);
extern void	    QSPI_WriteWord(uint32_t addr, uint16_t word);
extern void     QSPI_WriteWords(uint32_t *addrs, uint16_t *words, uint32_t count);
extern void	    QSPI_WriteBlock(uint32_t base, uint32_t count, uint16_t *block);
extern void     QSPI_ProgramWord(uint32_t Address, uint16_t word);
extern void	    QSPI_ProgramBuffer(uint32_t SectorAddress, uint16_t *data, uint32_t count);
extern void	    QSPI_ProgramBuffer_single(uint32_t SectorAddress, uint16_t word, uint32_t count);
extern void	    QSPI_EraseSector(uint32_t SectorAddress);
extern void	    QSPI_EraseChip(void);
extern void     QSPI_EnterVt(void);
extern void     QSPI_ExitVt(void);

extern S_DetApi gQSPIDriver;

#endif // !DET_DRIVER_QSPI_H
