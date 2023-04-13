/**
 * @file det_driver_sw.h
 * @brief Software detector read/write driver
 */

#ifndef DET_DRIVER_SW_H
#define DET_DRIVER_SW_H

#include "det_ctrl.h"

extern void	    Manual_Open(void);
extern void	    Manual_Close(void);

extern void	    Manual_SetAddress(uint32_t addr);
extern void	    Manual_SetData(uint16_t data);
extern uint16_t	Manual_GetData(void);

extern uint16_t	Manual_ReadWord(uint32_t addr);
extern void	    Manual_ReadWords(uint32_t *addrs, uint16_t *dest, uint32_t count);
extern void	    Manual_ReadBlock(uint32_t base, uint32_t count, uint16_t *dest);
extern void	    Manual_ReadPage(uint32_t PageAddress, uint16_t *dest);
extern void	    Manual_WriteWord(uint32_t addr, uint16_t word);
extern void     Manual_WriteWords(uint32_t *addrs, uint16_t *words, uint32_t count);
extern void	    Manual_WriteBlock(uint32_t base, uint32_t count, void *block);
extern void     Manual_ProgramWord(uint32_t Address, uint16_t word);
extern void	    Manual_ProgramBuffer(uint32_t SectorAddress, void *data, uint16_t count);
extern void	    Manual_ProgramBuffer_single(uint32_t SectorAddress, uint16_t word, uint16_t count);
extern void	    Manual_EraseSector(uint32_t SectorAddress);
extern void	    Manual_EraseChip(void);

extern S_DetApi gManualDriver;

#endif // !DET_DRIVER_SW_H
