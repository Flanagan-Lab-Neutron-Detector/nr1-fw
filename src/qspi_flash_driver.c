/**
 * @file qspi_flash_driver.c
 * @brief QSPI detector read/write driver
 */

#include "qspi_flash_driver.h"
#include "main.h"

extern OSPI_HandleTypeDef hospi1;

static void qspi_delay(uint32_t ticks)
{
	while (ticks--) __asm__("");
}

// read, no address
static void txn_read_na(uint8_t inst, uint8_t dummy, uint8_t *dest, uint32_t count)
{
    OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = inst,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = 0,
		.AddressMode           = HAL_OSPI_ADDRESS_NONE,
		.AddressSize           = HAL_OSPI_ADDRESS_24_BITS,
		.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE,
		.AlternateBytes        = 0,
		.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS,
		.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE,
		.DataMode              = HAL_OSPI_DATA_1_LINE,
		.NbData                = count,
		.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE,
		.DummyCycles           = dummy,
		.DQSMode               = HAL_OSPI_DQS_DISABLE,
		.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD
	};

	HAL_StatusTypeDef status;
	status = HAL_OSPI_Command(&hospi1, &cmd, HAL_MAX_DELAY);
	if (status != HAL_OK) Error_Handler();

	status = HAL_OSPI_Receive(&hospi1, dest, HAL_MAX_DELAY);
	while (HAL_OSPI_GetState(&hospi1) != HAL_OSPI_STATE_READY) ;
	if (status != HAL_OK) Error_Handler();
}

// read, with address
static void txn_read(uint8_t inst, uint8_t dummy, uint32_t addr, uint8_t *dest, uint32_t count)
{
    OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = inst,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = addr,
		.AddressMode           = HAL_OSPI_ADDRESS_1_LINE,
		.AddressSize           = HAL_OSPI_ADDRESS_24_BITS,
		.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE,
		.AlternateBytes        = 0,
		.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS,
		.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE,
		.DataMode              = HAL_OSPI_DATA_1_LINE,
		.NbData                = count,
		.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE,
		.DummyCycles           = dummy,
		.DQSMode               = HAL_OSPI_DQS_DISABLE,
		.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD
	};

	HAL_StatusTypeDef status;
	status = HAL_OSPI_Command(&hospi1, &cmd, HAL_MAX_DELAY);
	if (status != HAL_OK) Error_Handler();

	status = HAL_OSPI_Receive(&hospi1, dest, HAL_MAX_DELAY);
	while (HAL_OSPI_GetState(&hospi1) != HAL_OSPI_STATE_READY) ;
	if (status != HAL_OK) Error_Handler();
}

// instruction only
static void txn_inst(uint8_t inst)
{
    OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = inst,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = 0,
		.AddressMode           = HAL_OSPI_ADDRESS_NONE,
		.AddressSize           = HAL_OSPI_ADDRESS_24_BITS,
		.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE,
		.AlternateBytes        = 0,
		.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS,
		.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE,
		.DataMode              = HAL_OSPI_DATA_NONE,
		.NbData                = 0,
		.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE,
		.DummyCycles           = 0,
		.DQSMode               = HAL_OSPI_DQS_DISABLE,
		.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD
	};
    HAL_StatusTypeDef status;
	status = HAL_OSPI_Command(&hospi1, &cmd, HAL_MAX_DELAY);
	if (status != HAL_OK) Error_Handler();
	// No-data commands are sent immediately
}

// instruction with address
static void txn_inst_a(uint8_t inst, uint32_t addr)
{
    OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = inst,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = addr,
		.AddressMode           = HAL_OSPI_ADDRESS_1_LINE,
		.AddressSize           = HAL_OSPI_ADDRESS_24_BITS,
		.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE,
		.AlternateBytes        = 0,
		.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS,
		.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE,
		.DataMode              = HAL_OSPI_DATA_NONE,
		.NbData                = 0,
		.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE,
		.DummyCycles           = 0,
		.DQSMode               = HAL_OSPI_DQS_DISABLE,
		.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD
	};
    HAL_StatusTypeDef status;
	status = HAL_OSPI_Command(&hospi1, &cmd, HAL_MAX_DELAY);
	if (status != HAL_OK) Error_Handler();
	// No-data commands are sent immediately
}

// read

uint8_t	QSPI_Flash_ReadWord(uint32_t addr)
{
    uint8_t data = 0;
    txn_read(0x03, 0, addr, &data, 1);
    return data;
}

void QSPI_Flash_ReadWords(uint32_t *addrs, uint8_t *dest, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
        dest[i] = QSPI_Flash_ReadWord(addrs[i]);
}

void QSPI_Flash_ReadBlock(uint32_t base, uint32_t count, uint8_t *dest)
{
    txn_read(0x03, 0, base, dest, count);
}

// program

void QSPI_Flash_ProgramWord(uint32_t addr, uint8_t word)
{
    QSPI_Flash_ProgramBuffer(addr, &word, 1);
}

void QSPI_Flash_ProgramBuffer(uint32_t base, uint8_t *data, uint32_t count)
{
	QSPI_Flash_WriteEnable();
    OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = 0x02,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = base,
		.AddressMode           = HAL_OSPI_ADDRESS_1_LINE,
		.AddressSize           = HAL_OSPI_ADDRESS_24_BITS,
		.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE,
		.AlternateBytes        = 0,
		.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS,
		.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE,
		.DataMode              = HAL_OSPI_DATA_1_LINE,
		.NbData                = count,
		.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE,
		.DummyCycles           = 0,
		.DQSMode               = HAL_OSPI_DQS_DISABLE,
		.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD
	};

    HAL_StatusTypeDef status;
	status = HAL_OSPI_Command(&hospi1, &cmd, HAL_MAX_DELAY);
	if (status != HAL_OK) Error_Handler();

	status = HAL_OSPI_Transmit(&hospi1, data, HAL_MAX_DELAY);
	while (HAL_OSPI_GetState(&hospi1) != HAL_OSPI_STATE_READY) ;
	if (status != HAL_OK) Error_Handler();

	// write disabled after program
}

// erase

void QSPI_Flash_EraseSector(uint32_t SectorAddress) // erase 4kB sector
{
	QSPI_Flash_WriteEnable();
    txn_inst_a(0x20, SectorAddress);
	// write disabled after sector erase
}

void QSPI_Flash_EraseBlock32(uint32_t BlockAddress) // erase 32kB block
{
	QSPI_Flash_WriteEnable();
    txn_inst_a(0x52, BlockAddress);
	// write disabled after block erase
}

void QSPI_Flash_EraseBlock64(uint32_t BlockAddress) // erase 64kB block
{
	QSPI_Flash_WriteEnable();
    txn_inst_a(0xD8, BlockAddress);
	// write disabled after block erase
}

void QSPI_Flash_EraseChip(void)
{
	QSPI_Flash_WriteEnable();
    txn_inst(0x60);
	// write disabled after chip erase
}

// write enable/disable

void QSPI_Flash_WriteEnable(void) // Sets WEL
{
    txn_inst(0x06);
}

void QSPI_Flash_WriteDisable(void) // Clears WEL
{
    txn_inst(0x04);
}

// power down / ID

void QSPI_Flash_PowerDown(void) // Power down
{
    txn_inst(0xB9);
}

void QSPI_Flash_ReleasePowerDown(void) // Release from deep power down. Note: Does not read ID
{
    txn_inst(0xAB);
}

void QSPI_Flash_ReadMfgDevID(uint8_t *mf, uint8_t *id) // Read manufacturer and device ID
{
    uint8_t data[2];
    txn_read(0x90, 0, 0, data, 2);
    *mf = data[0];
    *id = data[1];
}

void QSPI_Flash_ReadJEDECID(uint8_t *mf, uint8_t *id_type, uint8_t *id_cap) // Read JEDEC ID
{
    uint8_t data[3];
    txn_read_na(0x9F, 0, data, 3);
    *mf      = data[0];
    *id_type = data[1];
    *id_cap  = data[2];
}

void QSPI_Flash_ReadUniqueID(uint8_t *uid) // Read unique ID
{
    txn_read_na(0x4B, 8*4, uid, 4);
}

// status registers

uint8_t QSPI_Flash_ReadStatusReg(QSPI_FLASH_STATUS_REG sr) // Read status register 1
{
    uint8_t inst = 0;
    switch (sr) {
        case QSPI_FLASH_STATUS_REG_1: inst = 0x05; break;
        case QSPI_FLASH_STATUS_REG_2: inst = 0x35; break;
        case QSPI_FLASH_STATUS_REG_3: inst = 0x15; break;
    }

    uint8_t data = 0;
    txn_read_na(inst, 0, &data, 1);
    return data;
}

void QSPI_Flash_WriteStatusReg(QSPI_FLASH_STATUS_REG sr, uint8_t value) // Write status register 1/2/3
{
	QSPI_Flash_WriteEnable();
    uint8_t inst = 0;
    switch (sr) {
        case QSPI_FLASH_STATUS_REG_1: inst = 0x01; break;
        case QSPI_FLASH_STATUS_REG_2: inst = 0x31; break;
        case QSPI_FLASH_STATUS_REG_3: inst = 0x11; break;
    }

    OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = inst,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = 0,
		.AddressMode           = HAL_OSPI_ADDRESS_NONE,
		.AddressSize           = HAL_OSPI_ADDRESS_24_BITS,
		.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE,
		.AlternateBytes        = 0,
		.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS,
		.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE,
		.DataMode              = HAL_OSPI_DATA_1_LINE,
		.NbData                = 1,
		.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE,
		.DummyCycles           = 0,
		.DQSMode               = HAL_OSPI_DQS_DISABLE,
		.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD
	};

    HAL_StatusTypeDef status;
	status = HAL_OSPI_Command(&hospi1, &cmd, HAL_MAX_DELAY);
	if (status != HAL_OK) Error_Handler();

	status = HAL_OSPI_Transmit(&hospi1, &value, HAL_MAX_DELAY);
	while (HAL_OSPI_GetState(&hospi1) != HAL_OSPI_STATE_READY) ;
	if (status != HAL_OK) Error_Handler();

	// write disabled after write status register
}

// reset

void QSPI_Flash_Reset(void) // Reset device
{
    // enable reset
    txn_inst(0x66);

    // reset
    txn_inst(0x99);
}