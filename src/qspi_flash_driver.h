/**
 * @file qspi_flash_driver.h
 * @brief Read/write to interface FPGA configuration flash
 */

#ifndef QSPI_FLASH_DRIVER_H
#define QSPI_FLASH_DRIVER_H

#include <stdint.h>

// read
extern uint8_t	QSPI_Flash_ReadWord(uint32_t addr);
extern void	    QSPI_Flash_ReadWords(uint32_t *addrs, uint8_t *dest, uint32_t count);
extern void	    QSPI_Flash_ReadBlock(uint32_t base, uint32_t count, uint8_t *dest);

// program
extern void     QSPI_Flash_ProgramWord(uint32_t addr, uint8_t word);
extern void	    QSPI_Flash_ProgramBuffer(uint32_t base, uint8_t *data, uint32_t count);

// erase
extern void	    QSPI_Flash_EraseSector(uint32_t SectorAddress); // erase 4kB sector
extern void	    QSPI_Flash_EraseBlock32(uint32_t BlockAddress); // erase 32kB block
extern void	    QSPI_Flash_EraseBlock64(uint32_t BlockAddress); // erase 64kB block
extern void	    QSPI_Flash_EraseChip(void);

// write enable/disable
extern void     QSPI_Flash_WriteEnable(void); // Sets WEL
extern void     QSPI_Flash_WriteDisable(void); // Clears WEL

// power down / ID
extern void     QSPI_Flash_PowerDown(void); // Power down
extern void     QSPI_Flash_ReleasePowerDown(void); // Release from deep power down. Note: Does not read ID
extern void     QSPI_Flash_ReadMfgDevID(uint8_t *mf, uint8_t *id); // Read manufacturer and device ID
extern void     QSPI_Flash_ReadJEDECID(uint8_t *mf, uint8_t *id_type, uint8_t *id_cap); // Read JEDEC ID
extern void     QSPI_Flash_ReadUniqueID(uint8_t uid[4]); // Read unique ID

// status registers
typedef enum {
    QSPI_FLASH_STATUS_REG_1,
    QSPI_FLASH_STATUS_REG_2,
    QSPI_FLASH_STATUS_REG_3
} QSPI_FLASH_STATUS_REG;
extern uint8_t  QSPI_Flash_ReadStatusReg(QSPI_FLASH_STATUS_REG sr); // Read status register 1/2/3
extern void     QSPI_Flash_WriteStatusReg(QSPI_FLASH_STATUS_REG sr, uint8_t value); // Write status register 1/2/3

// reset
extern void     QSPI_Flash_Reset(void); // Reset device

#endif // !QSPI_FLASH_DRIVER_H
