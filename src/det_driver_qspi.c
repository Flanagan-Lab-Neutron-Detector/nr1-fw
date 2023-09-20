/**
 * @file det_driver_qspi.c
 * @brief QSPI detector read/write driver
 */

#include "det_driver_qspi.h"
#include "main.h"
#include "det_ctrl.h"

extern OSPI_HandleTypeDef hospi1;

S_DetApi gQSPIDriver = {
	.Open = QSPI_Open,
	.Close = QSPI_Close,

	.ReadWord				= QSPI_ReadWord,
	.ReadWords				= QSPI_ReadWords,
	.ReadBlock				= QSPI_ReadBlock,
	.ReadPage				= QSPI_ReadPage,
	.WriteWord				= QSPI_WriteWord,
	.WriteWords				= QSPI_WriteWords,
	.WriteCommandWord       = QSPI_WriteWord,
	.WriteCommandWords      = QSPI_WriteWords,
	.WriteBlock				= QSPI_WriteBlock,
	.ProgramWord            = QSPI_ProgramWord,
	.ProgramBuffer			= QSPI_ProgramBuffer,
	.ProgramBuffer_single	= QSPI_ProgramBuffer_single,
	.EraseSector			= QSPI_EraseSector,
	.EraseChip				= QSPI_EraseChip,
	.EnterVt                = QSPI_EnterVt,
	.ExitVt                 = QSPI_ExitVt
};

void QSPI_Open(void)
{
	gDetApi = &gQSPIDriver;

	CE_SWITCH_DIG;
}

void QSPI_Close(void)
{
	// do nothing
}

uint16_t QSPI_ReadWord(uint32_t addr)
{
	OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = 0x0B,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = addr,
		.AddressMode           = HAL_OSPI_ADDRESS_4_LINES,
		.AddressSize           = HAL_OSPI_ADDRESS_32_BITS,
		.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE,
		.AlternateBytes        = 0,
		.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS,
		.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE,
		.DataMode              = HAL_OSPI_DATA_4_LINES,
		.NbData                = 2,
		.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE,
		.DummyCycles           = 20,
		.DQSMode               = HAL_OSPI_DQS_DISABLE,
		.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD
	};
	/*OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = 0xFA,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = addr,
		.AddressMode           = HAL_OSPI_ADDRESS_4_LINES,
		.AddressSize           = HAL_OSPI_ADDRESS_32_BITS,
		.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE,
		.AlternateBytes        = 0,
		.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS,
		.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE,
		.DataMode              = HAL_OSPI_DATA_4_LINES,
		.NbData                = 2,
		.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE,
		.DummyCycles           = 0,
		.DQSMode               = HAL_OSPI_DQS_DISABLE,
		.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD
	};*/

	HAL_StatusTypeDef status;
	status = HAL_OSPI_Command(&hospi1, &cmd, HAL_MAX_DELAY);
	if (status != HAL_OK) Error_Handler();

	uint16_t data = 0;
	status = HAL_OSPI_Receive(&hospi1, (uint8_t*)&data, HAL_MAX_DELAY);
	while (HAL_OSPI_GetState(&hospi1) != HAL_OSPI_STATE_READY) ;
	if (status != HAL_OK) Error_Handler();
	// OSPI shifts in little-endian but we need big-endian
	data = ((data & 0x00FF) << 8) | ((data & 0xFF00) >> 8);
	return data;
}

void QSPI_ReadWords(uint32_t *addrs, uint16_t *dest, uint32_t count)
{
	for (uint32_t i=0; i<count; i++) {
		dest[i] = QSPI_ReadWord(addrs[i]);
	}
}

void QSPI_ReadBlock(uint32_t base, uint32_t count, uint16_t *dest)
{
	for (uint32_t i=0; i<count; i++)
		dest[i] = QSPI_ReadWord(base + i);
}

void QSPI_ReadPage(uint32_t PageAddress, uint16_t *dest)
{
	QSPI_ReadBlock(PageAddress, 8, dest);
}

void QSPI_WriteWord(uint32_t addr, uint16_t word) {
	// OSPI shifts out little-endian but we need big-endian
	word = ((word & 0x00FF) << 8) | ((word & 0xFF00) >> 8);
	OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = 0xF8,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = addr,
		.AddressMode           = HAL_OSPI_ADDRESS_4_LINES,
		.AddressSize           = HAL_OSPI_ADDRESS_32_BITS,
		.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE,
		.AlternateBytes        = 0,
		.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS,
		.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE,
		.DataMode              = HAL_OSPI_DATA_4_LINES,
		.NbData                = 2,
		.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE,
		.DummyCycles           = 0,
		.DQSMode               = HAL_OSPI_DQS_DISABLE,
		.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD
	};
	HAL_StatusTypeDef status;
	status = HAL_OSPI_Command(&hospi1, &cmd, HAL_MAX_DELAY);
	if (status != HAL_OK) Error_Handler();

	status = HAL_OSPI_Transmit(&hospi1, (uint8_t*)&word, HAL_MAX_DELAY);
	while (HAL_OSPI_GetState(&hospi1) != HAL_OSPI_STATE_READY) ;
	if (status != HAL_OK) Error_Handler();
}

void QSPI_WriteWords(uint32_t *addrs, uint16_t *words, uint32_t count)
{
	for (uint32_t i=0; i<count; i++) {
		QSPI_WriteWord(addrs[i], words[i]);
	}
}

void QSPI_WriteBlock(uint32_t base, uint32_t count, uint16_t *block)
{
	for (uint32_t i=0; i<count; i++)
		QSPI_WriteWord(base + i, block[i]);
}

void QSPI_ProgramWord(uint32_t Address, uint16_t word)
{
	OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = 0xF2,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = Address,
		.AddressMode           = HAL_OSPI_ADDRESS_4_LINES,
		.AddressSize           = HAL_OSPI_ADDRESS_32_BITS,
		.AddressDtrMode        = HAL_OSPI_ADDRESS_DTR_DISABLE,
		.AlternateBytes        = 0,
		.AlternateBytesMode    = HAL_OSPI_ALTERNATE_BYTES_NONE,
		.AlternateBytesSize    = HAL_OSPI_ALTERNATE_BYTES_8_BITS,
		.AlternateBytesDtrMode = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE,
		.DataMode              = HAL_OSPI_DATA_4_LINES,
		.NbData                = 2,
		.DataDtrMode           = HAL_OSPI_DATA_DTR_DISABLE,
		.DummyCycles           = 0,
		.DQSMode               = HAL_OSPI_DQS_DISABLE,
		.SIOOMode              = HAL_OSPI_SIOO_INST_EVERY_CMD
	};
	HAL_StatusTypeDef status;
	status = HAL_OSPI_Command(&hospi1, &cmd, HAL_MAX_DELAY);
	if (status != HAL_OK) Error_Handler();

	status = HAL_OSPI_Transmit(&hospi1, (uint8_t*)&word, HAL_MAX_DELAY);
	while (HAL_OSPI_GetState(&hospi1) != HAL_OSPI_STATE_READY) ;
	if (status != HAL_OK) Error_Handler();
}

void QSPI_ProgramBuffer(uint32_t SectorAddress, uint16_t *data, uint16_t count)
{
	for (uint32_t i=0; i<count; i++)
		QSPI_ProgramWord(SectorAddress + i, data[i]);
}

void QSPI_ProgramBuffer_single(uint32_t SectorAddress, uint16_t word, uint16_t count)
{
	for (uint32_t i=0; i<count; i++)
		QSPI_ProgramWord(SectorAddress + i, word);
}

void QSPI_EraseSector(uint32_t SectorAddress)
{
	OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = 0xD8,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = SectorAddress,
		.AddressMode           = HAL_OSPI_ADDRESS_4_LINES,
		.AddressSize           = HAL_OSPI_ADDRESS_32_BITS,
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

	int dummy = 0;
	HAL_OSPI_Transmit(&hospi1, (uint8_t*)&dummy, HAL_MAX_DELAY);
	while (HAL_OSPI_GetState(&hospi1) != HAL_OSPI_STATE_READY) ;
	if (status != HAL_OK) Error_Handler();
}

void QSPI_EraseChip(void)
{
	OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = 0x60,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = 0,
		.AddressMode           = HAL_OSPI_ADDRESS_NONE,
		.AddressSize           = HAL_OSPI_ADDRESS_32_BITS,
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
	// Instruction-only commands are sent immediately
}

void QSPI_EnterVt(void)
{
	OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = 0xFB,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = 0,
		.AddressMode           = HAL_OSPI_ADDRESS_NONE,
		.AddressSize           = HAL_OSPI_ADDRESS_32_BITS,
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
	// Instruction-only commands are sent immediately
}

void QSPI_ExitVt(void)
{
	OSPI_RegularCmdTypeDef cmd = {
		.OperationType         = HAL_OSPI_OPTYPE_COMMON_CFG,
		.FlashId               = HAL_OSPI_FLASH_ID_1,
		.Instruction           = 0xF0,
		.InstructionMode       = HAL_OSPI_INSTRUCTION_1_LINE,
		.InstructionSize       = HAL_OSPI_INSTRUCTION_8_BITS,
		.InstructionDtrMode    = HAL_OSPI_INSTRUCTION_DTR_DISABLE,
		.Address               = 0,
		.AddressMode           = HAL_OSPI_ADDRESS_NONE,
		.AddressSize           = HAL_OSPI_ADDRESS_32_BITS,
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
	// Instruction-only commands are sent immediately
}