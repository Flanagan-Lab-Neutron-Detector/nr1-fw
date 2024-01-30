/**
 * @file comms_usb_hpt.c
 * @brief USB communications driver for HPT protocol
 */

#define COMMS_USB_HPT_C
#include "comms_usb_hpt.h"
#include "comms_hpt_msgs.h"
#include "usbd_cdc_if.h"
#include "det_ctrl.h"
#include "analog.h"
#include "qspi_flash_driver.h"
#include <stdbool.h>

// debug
#include <stdio.h>

// TODO: Move this to a header
extern uint32_t g_config_save_requested;

extern CRC_HandleTypeDef hcrc;

/**
 * @brief Global scratch response buffer
 *
 */
static HPT_MsgRsp g_msg_rsp;

/**
 * @brief Global current requested command
 */
static HPT_MsgCmd g_msg_cmd;

static HPT_MsgCmd m_cmd_copy; // copy for slow processing (outside of interrupt)

void comms_usb_hpt_reset(void)
{
	g_comms_cmd_req = HPT_NULL_MSG_CMD;
}

/**
 * @brief Handle kpage bit count with voltage request
 *
 * @note Runs in USB interrupt
 *
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_vt_get_bit_count_kpage_cmd(HPT_VtGetBitCountKPageCmd *cmd, HPT_MsgRsp *rsp)
{
	UNUSED(cmd);
	rsp->CmdRsp = HPT_FAILED_COMMAND_RSP;
	rsp->FailureRsp.FailureCodes[rsp->FailureRsp.Failures] = HPT_FAILURE_CODE_CMD_UNIMPLEMENTED;
	rsp->FailureRsp.Failures++;
}

/**
 * @brief Handle sector bit count with voltage request
 *
 * @note Runs in USB interrupt
 *
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_get_sector_bit_count_cmd(HPT_GetSectorBitCountCmd *cmd, HPT_MsgRsp *rsp)
{
	UNUSED(cmd);
	rsp->CmdRsp = HPT_FAILED_COMMAND_RSP;
	rsp->FailureRsp.FailureCodes[rsp->FailureRsp.Failures] = HPT_FAILURE_CODE_CMD_UNIMPLEMENTED;
	rsp->FailureRsp.Failures++;
}

static void read_data(uint32_t addr, uint16_t *data, uint32_t count)
{
	// The last 512-word chunk of each sector may read garbage when ReadBlock is used.
	// Use ReadWords for addresses in [0xFE00, 0x10000) and ReadBlock otherwise.

	uint32_t sector_offset = addr & 0xFFFF;
	
	if (sector_offset + count < 0xFE00) {
		gDetApi->ReadBlock(addr, count, data);
	} else {
		// Block 0 is addr up to the last 512-word chunk of the sector
		// Block 1 is the last 512-word chunk of the sector
		// Block 2 is the remaining data in the next sector
		// Block 0 and Block 2 may be empty. Block 1 may be partial.
		uint32_t block0_addr  = addr;
		int32_t  block0_count = sector_offset         < 0xFE00  ? 0xFE00  - sector_offset : 0; //MAX(0xFE00 - sector_offset, 0);
		uint32_t block1_addr  = block0_addr + block0_count; // = sector + 0xFE00
		int32_t  block1_count = sector_offset         > 0xFE00  ? 0x10000 - sector_offset : (
		                        sector_offset + count < 0x10000 ? count   - block0_count  : 512); //MIN(count - block0_count, 512);	
		uint32_t block2_addr  = block1_addr + block1_count; // = sector + 0x10000
		int32_t  block2_count = count - block0_count - block1_count;

		if (block0_count > 0)
			gDetApi->ReadBlock(block0_addr, block0_count, data);
		for (int32_t i = 0; i < block1_count; i++)
			data[block0_count + i] = gDetApi->ReadWord(block1_addr + i);
		if (block2_count > 0)
			gDetApi->ReadBlock(block2_addr, block2_count, data + block0_count + block1_count);
	}
}

/**
 * @brief Handle data read with voltage request
 *
 * @note Runs in USB interrupt
 *
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_read_data_cmd(HPT_ReadDataCmd *cmd, HPT_MsgRsp *rsp)
{
	uint32_t isVt = cmd->VtMode;
	uint32_t vtMv = cmd->BitReadMv;
	uint32_t addr = cmd->BaseAddress;

	int iserr = 0;

	if (isVt) {
		iserr |= DetEnterVtMode();
		iserr |= DetSetVt(vtMv);
	} else {
		iserr |= DetExitVtMode();
	}

	// Cap at words in response buffer
	uint32_t count_max = sizeof(rsp->ReadDataRsp.Data) / sizeof(uint16_t);
	uint32_t count = cmd->NumWords > count_max ? count_max : cmd->NumWords;

	// The last 512-word chunk of each sector may read garbage when ReadBlock is used.
	// Use ReadWords for addresses in [0xFE00, 0x10000) and ReadBlock otherwise.
	if (count > 0)
		read_data(addr, rsp->ReadDataRsp.Data, count);

	/*if (isVt)
	{
		iserr |= DetExitVtMode();
	}*/

	if (iserr) {
		rsp->CmdRsp = HPT_FAILED_COMMAND_RSP;
		rsp->FailureRsp.FailureCodes[g_msg_rsp.FailureRsp.Failures] = HPT_FAILURE_CODE_ANA_DAC_ERR;
		rsp->FailureRsp.Failures++;
	} else {
		rsp->CmdRsp = HPT_READ_DATA_RSP;
		rsp->Length += 2*(count + count % 2); // round up to nearest 4-byte boundary
	}
}

/**
 * @brief Handle read word with voltage request
 *
 * @note Runs in USB interrupt
 *
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_read_word_cmd(HPT_ReadWordCmd *cmd, HPT_MsgRsp *rsp)
{
	uint32_t isVt    = cmd->VtMode;
	uint32_t vtMv    = cmd->BitReadMv;
	uint32_t addr    = cmd->WordAddress;
	uint32_t samples = cmd->Samples > 0 ? cmd->Samples : 1;

	// initialize sample array
	for (uint32_t b=0; b<16; b++) rsp->ReadWordRsp.BitSample[b] = 0;

	if (isVt) {
		DetEnterVtMode();
		DetSetVt(vtMv);
	} else {
		DetExitVtMode();
	}

	for (uint32_t i=0; i<samples; i++) {
		uint16_t word = gDetApi->ReadWord(addr);
		for (uint32_t b=0; b<16; b++) {
			rsp->ReadWordRsp.BitSample[b] += ((word >> b) & 0x1) ? 1 : 0;
		}
	}

	rsp->ReadWordRsp.Samples = samples;

	/*if (isVt)
	{
		DetExitVtMode();
	}*/

	rsp->CmdRsp = HPT_READ_WORD_RSP;
	rsp->Length += sizeof(HPT_ReadWordRsp);
}

/**
 * @brief Handle write config
 * 
 * @note Runs in USB interrupt
 * 
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_write_cfg_cmd(HPT_WriteCfgCmd *cmd, HPT_MsgRsp *rsp)
{
	uint32_t addr = cmd->Address | 0x80000000;
	uint16_t word = (uint16_t)cmd->Data;
	gDetApi->WriteCommandWord(addr, word);
	rsp->CmdRsp = HPT_WRITE_CFG_RSP;
}

/**
 * @brief Handle read config
 * 
 * @note Runs in USB interrupt
 * 
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_read_cfg_cmd(HPT_ReadCfgCmd *cmd, HPT_MsgRsp *rsp)
{
	uint32_t addr = cmd->Address | 0x80000000;
	rsp->ReadCfgRsp.Data = gDetApi->ReadWord(addr);
	rsp->CmdRsp = HPT_READ_CFG_RSP;
	rsp->Length += sizeof(HPT_ReadCfgRsp);
}

/**
 * @brief Handle config flash enter
 * 
 * @note Runs in USB interrupt
 * 
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_cfg_flash_enter_cmd(HPT_NoDataCmdRsp *cmd, HPT_MsgRsp *rsp)
{
	UNUSED(cmd);
	gDetApi->EnterCfgFlash();
	rsp->CmdRsp = HPT_CFG_FLASH_ENTER_RSP;
}

/**
 * @brief Handle config flash exit
 * 
 * @note Runs in USB interrupt
 * 
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_cfg_flash_exit_cmd(HPT_NoDataCmdRsp *cmd, HPT_MsgRsp *rsp)
{
	UNUSED(cmd);
	gDetApi->ExitCfgFlash();
	rsp->CmdRsp = HPT_CFG_FLASH_EXIT_RSP;
}

/**
 * @brief Handle config flash read
 * 
 * @note Runs in USB interrupt
 * 
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_cfg_flash_read_cmd(HPT_CfgFlashReadCmd *cmd, HPT_MsgRsp *rsp)
{
	QSPI_Flash_ReleasePowerDown();
	QSPI_Flash_ReadBlock(cmd->Address, cmd->NumBytes, rsp->CfgFlashReadRsp.Data);
	QSPI_Flash_PowerDown();
	rsp->CmdRsp = HPT_CFG_FLASH_READ_RSP;
	rsp->Length += cmd->NumBytes * sizeof(uint8_t);
}

/**
 * @brief Handle config flash write
 * 
 * @note Runs in USB interrupt
 * 
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_cfg_flash_write_cmd(HPT_CfgFlashWriteCmd *cmd, HPT_MsgRsp *rsp)
{
	QSPI_Flash_ReleasePowerDown();
	uint32_t remaining = cmd->NumWords;
	uint32_t address = cmd->BaseAddress;
	uint8_t *data = cmd->Data;
	while (remaining > 0) {
		uint32_t count = remaining > 256 ? 256 : remaining;
		QSPI_Flash_ProgramBuffer(address, data, count);
		remaining -= count;
		address += count;
		data += count;

		// wait for write to complete
		uint8_t sr1 = QSPI_Flash_ReadStatusReg(QSPI_FLASH_STATUS_REG_1);
		while (sr1 & 0x01) sr1 = QSPI_Flash_ReadStatusReg(QSPI_FLASH_STATUS_REG_1);
	}
	//QSPI_Flash_ProgramBuffer(cmd->BaseAddress, cmd->Data, cmd->NumWords);
	QSPI_Flash_PowerDown();
	rsp->CmdRsp = HPT_CFG_FLASH_WRITE_RSP;
}

/**
 * @brief Handle config flash erase
 * 
 * @note Runs in USB interrupt
 * 
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_cfg_flash_erase_cmd(HPT_CfgFlashEraseCmd *cmd, HPT_MsgRsp *rsp)
{
	QSPI_Flash_ReleasePowerDown();
	rsp->CmdRsp = HPT_CFG_FLASH_ERASE_RSP;
	switch (cmd->EraseType) {
		case 0: QSPI_Flash_EraseSector(cmd->Address); break;
		case 1: QSPI_Flash_EraseBlock32(cmd->Address); break;
		case 2: QSPI_Flash_EraseBlock64(cmd->Address); break;
		case 3: QSPI_Flash_EraseChip(); break;
		default:
			rsp->CmdRsp = HPT_FAILED_COMMAND_RSP;
			rsp->FailureRsp.FailureCodes[g_msg_rsp.FailureRsp.Failures] = HPT_FAILURE_CODE_CMD_INVALID_PARAM;
			rsp->FailureRsp.Failures++;
			break;
	}
	QSPI_Flash_PowerDown();
}

/**
 * @brief Handle config flash get device info
 * 
 * @note Runs in USB interrupt
 * 
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_cfg_flash_dev_info_cmd(HPT_NoDataCmdRsp *cmd, HPT_MsgRsp *rsp)
{
	UNUSED(cmd);
	uint8_t mf, id, jedec_type, jedec_cap;
	uint8_t sr1, sr2, sr3;

	QSPI_Flash_ReleasePowerDown();
	QSPI_Flash_ReadMfgDevID(&mf, &id);
	QSPI_Flash_ReadJEDECID(&mf, &jedec_type, &jedec_cap);
	QSPI_Flash_ReadUniqueID(rsp->CfgFlashDevInfoRsp.UniqueID);
	sr1 = QSPI_Flash_ReadStatusReg(QSPI_FLASH_STATUS_REG_1);
	sr2 = QSPI_Flash_ReadStatusReg(QSPI_FLASH_STATUS_REG_2);
	sr3 = QSPI_Flash_ReadStatusReg(QSPI_FLASH_STATUS_REG_3);
	QSPI_Flash_PowerDown();

	rsp->CfgFlashDevInfoRsp.ManufacturerID  = mf;
	rsp->CfgFlashDevInfoRsp.DeviceID        = id;
	rsp->CfgFlashDevInfoRsp.JEDECType       = jedec_type;
	rsp->CfgFlashDevInfoRsp.JEDECCapacity   = jedec_cap;
	rsp->CfgFlashDevInfoRsp.StatusRegister1 = sr1;
	rsp->CfgFlashDevInfoRsp.StatusRegister2 = sr2;
	rsp->CfgFlashDevInfoRsp.StatusRegister3 = sr3;
	rsp->CfgFlashDevInfoRsp._Pad[0] = 0;
	rsp->CmdRsp = HPT_CFG_FLASH_DEV_INFO_RSP;
	rsp->Length += sizeof(HPT_CfgFlashDevInfoRsp);
}

/**
 * @brief Handle analog set calibration counts
 *
 * Sets calibration counts for the specified analog unit. NR1B supports units 1 (reset/vwl) and 2 (wp).
 *
 * @note Runs in USB interrupt
 *
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_ana_set_cal_counts(HPT_AnaSetCalCountsCmd *cmd, HPT_MsgRsp *rsp)
{
	switch (cmd->AnalogUnit)
	{
		case 1:
			gDac1.CalC0 = cmd->CalC0;
			gDac1.CalC1 = cmd->CalC1;
			rsp->CmdRsp = HPT_ANA_SET_CAL_COUNTS_RSP;
			g_config_save_requested = 1;
			break;
		case 2:
			gDac2.CalC0 = cmd->CalC0;
			gDac2.CalC1 = cmd->CalC1;
			rsp->CmdRsp = HPT_ANA_SET_CAL_COUNTS_RSP;
			g_config_save_requested = 1;
			break;
		default:
			rsp->CmdRsp = HPT_FAILED_COMMAND_RSP;
			rsp->FailureRsp.FailureCodes[g_msg_rsp.FailureRsp.Failures] = HPT_FAILURE_CODE_CMD_INVALID_PARAM;
			rsp->FailureRsp.Failures++;
			break;
	}
}

/**
 * @brief Handle analog set active counts
 *
 * Sets counts for the specified analog unit. NR1B supports units 1 (reset/vwl) and 2 (wp).
 *
 * @note Runs in USB interrupt
 *
 * @param cmd Command
 * @param rsp Response
 */
void comms_hpt_handle_ana_set_active_counts(HPT_AnaSetActiveCountsCmd *cmd, HPT_MsgRsp *rsp)
{
	switch (cmd->AnalogUnit)
	{
		case 1:
			DacWriteOutput(1, cmd->UnitCounts);
			rsp->CmdRsp = HPT_ANA_SET_ACTIVE_COUNTS_RSP;
			break;
		case 2:
			DacWriteOutput(2, cmd->UnitCounts);
			rsp->CmdRsp = HPT_ANA_SET_ACTIVE_COUNTS_RSP;
			break;
		default:
			rsp->CmdRsp = HPT_FAILED_COMMAND_RSP;
			rsp->FailureRsp.FailureCodes[g_msg_rsp.FailureRsp.Failures] = HPT_FAILURE_CODE_CMD_INVALID_PARAM;
			rsp->FailureRsp.Failures++;
			break;
	}
}

/**
 * @brief Handle an HPT Bus message
 *
 * Receives an HPT Bus message from the comms subsystem.
 * Simple messages (e.g. ping) are handled here, otherwise
 * this function calls other functions or sets flags to
 * perform the request.
 *
 * @note Runs in USB interrupt
 * @note Fills out global message response structure @ref g_msg_rsp
 *
 * @param msg Incoming HPT Bus message
 * @return uint32_t Length of response. Zero indicates no response.
 */
uint32_t comms_usb_hpt_receive_msg(HPT_MsgCmd *msg)
{

// Dispatch a long message for further processing, if detector is available
#define COMMS_CHECK_DISPATCH(M) do { \
	if (g_comms_cmd_req == HPT_NULL_MSG_CMD) { \
		g_msg_rsp.CmdRsp = M##_RSP; \
		memcpy(&m_cmd_copy, msg, sizeof(HPT_MsgCmd)); \
		g_comms_cmd_req = M##_CMD; \
	} else { g_msg_rsp.CmdRsp = HPT_FAILED_COMMAND_RSP; \
			g_msg_rsp.FailureRsp.FailureCodes[g_msg_rsp.FailureRsp.Failures] = HPT_FAILURE_CODE_CMD_BUSY; \
			g_msg_rsp.FailureRsp.Failures++; } \
} while (0)

	puts("Received message");

	HPT_CmdRespEnum cmd = msg->CmdRsp;

	// we unconditionally send a message
	g_msg_rsp.StartChar = HPT_MSG_SOM_CHAR;
	g_msg_rsp.CmdRsp = HPT_UNKNOWN_COMMAND_RSP;
	g_msg_rsp.Length = HPT_SIZE_OF_HEADER + HPT_SIZE_OF_CRC;
	// Start FailureRsp.Failures at 0 so failures can be appended
	g_msg_rsp.FailureRsp.Failures = 0;

	if (cmd == HPT_PING_CMD) {
		g_msg_rsp.Length += sizeof(HPT_PingRsp);
		g_msg_rsp.CmdRsp = HPT_PING_RSP;
		g_msg_rsp.PingRsp.UptimeSeconds = HAL_GetTick() / 1000;
		g_msg_rsp.PingRsp.VersionString[0] = 'N';
		g_msg_rsp.PingRsp.VersionString[1] = 'R';
		g_msg_rsp.PingRsp.VersionString[2] = '1';
		g_msg_rsp.PingRsp.VersionString[3] = ' ';
		g_msg_rsp.PingRsp.VersionString[4] = 't';
		g_msg_rsp.PingRsp.VersionString[5] = 'e';
		g_msg_rsp.PingRsp.VersionString[6] = 's';
		g_msg_rsp.PingRsp.VersionString[7] = 't';
		g_msg_rsp.PingRsp.VersionString[8] = '\0';
		for (uint32_t i=9; i<16; i++) g_msg_rsp.PingRsp.VersionString[i] = 0;
		g_msg_rsp.PingRsp.IsDetectorBusy   = g_comms_cmd_req != HPT_NULL_MSG_CMD;
		g_msg_rsp.PingRsp.ResetFlags       = gResetFlags;
		g_msg_rsp.PingRsp.Task             = g_comms_cmd_req;
		g_msg_rsp.PingRsp.TaskState        = g_comms_cmd_req_state;
	} else if (cmd == HPT_ANA_GET_CAL_COUNTS_CMD && (msg->AnaGetCalCountsCmd.AnalogUnit == 1 || msg->AnaGetCalCountsCmd.AnalogUnit == 2)) {
		// Parse DAC unit
		float CalC0 = 0, CalC1 = 0;
		switch (msg->AnaGetCalCountsCmd.AnalogUnit) {
			case 1:
				CalC0 = gDac1.CalC0;
				CalC1 = gDac1.CalC1;
				break;
			case 2:
				CalC0 = gDac2.CalC0;
				CalC1 = gDac2.CalC1;
				break;
		}
		// Send response
		g_msg_rsp.Length += sizeof(HPT_AnaGetCalCountsRsp);
		g_msg_rsp.CmdRsp = HPT_ANA_GET_CAL_COUNTS_RSP;
		g_msg_rsp.AnaGetCalCountsRsp.CalC0 = CalC0;
		g_msg_rsp.AnaGetCalCountsRsp.CalC1 = CalC1;
	} else if (g_comms_cmd_req != HPT_NULL_MSG_CMD) {
		// Command in progress
		g_msg_rsp.CmdRsp = HPT_FAILED_COMMAND_RSP;
		g_msg_rsp.FailureRsp.FailureCodes[g_msg_rsp.FailureRsp.Failures] = HPT_FAILURE_CODE_CMD_BUSY;
		g_msg_rsp.FailureRsp.Failures++;
	} else {
		switch (cmd) {
			case HPT_VT_GET_BIT_COUNT_KPAGE_CMD:
				comms_hpt_handle_vt_get_bit_count_kpage_cmd(&msg->VtGetBitCountKPageCmd, &g_msg_rsp);
				break;
			case HPT_ERASE_CHIP_CMD:
				COMMS_CHECK_DISPATCH(HPT_ERASE_CHIP);
				break;
			case HPT_ERASE_SECTOR_CMD:
				COMMS_CHECK_DISPATCH(HPT_ERASE_SECTOR);
				break;
			case HPT_PROGRAM_SECTOR_CMD:
				COMMS_CHECK_DISPATCH(HPT_PROGRAM_SECTOR);
				break;
			case HPT_PROGRAM_CHIP_CMD:
				COMMS_CHECK_DISPATCH(HPT_PROGRAM_CHIP);
				break;
			case HPT_GET_SECTOR_BIT_COUNT_CMD:
				comms_hpt_handle_get_sector_bit_count_cmd(&msg->GetSectorBitCountCmd, &g_msg_rsp);
				break;
			case HPT_READ_DATA_CMD:
				comms_hpt_handle_read_data_cmd(&msg->ReadDataCmd, &g_msg_rsp);
				break;
			case HPT_WRITE_DATA_CMD:
				COMMS_CHECK_DISPATCH(HPT_WRITE_DATA);
				break;
			case HPT_READ_WORD_CMD:
				comms_hpt_handle_read_word_cmd(&msg->ReadWordCmd, &g_msg_rsp);
				break;
			case HPT_WRITE_CFG_CMD:
				comms_hpt_handle_write_cfg_cmd(&msg->WriteCfgCmd, &g_msg_rsp);
				break;
			case HPT_READ_CFG_CMD:
				comms_hpt_handle_read_cfg_cmd(&msg->ReadCfgCmd, &g_msg_rsp);
				break;
			case HPT_CFG_FLASH_ENTER_CMD:
				comms_hpt_handle_cfg_flash_enter_cmd(&msg->NoDataCmdRsp, &g_msg_rsp);
				break;
			case HPT_CFG_FLASH_EXIT_CMD:
				comms_hpt_handle_cfg_flash_exit_cmd(&msg->NoDataCmdRsp, &g_msg_rsp);
				break;
			case HPT_CFG_FLASH_READ_CMD:
				comms_hpt_handle_cfg_flash_read_cmd(&msg->CfgFlashReadCmd, &g_msg_rsp);
				break;
			case HPT_CFG_FLASH_WRITE_CMD:
				comms_hpt_handle_cfg_flash_write_cmd(&msg->CfgFlashWriteCmd, &g_msg_rsp);
				break;
			case HPT_CFG_FLASH_ERASE_CMD:
				comms_hpt_handle_cfg_flash_erase_cmd(&msg->CfgFlashEraseCmd, &g_msg_rsp);
				break;
			case HPT_CFG_FLASH_DEV_INFO_CMD:
				comms_hpt_handle_cfg_flash_dev_info_cmd(&msg->NoDataCmdRsp, &g_msg_rsp);
				break;
			case HPT_ANA_SET_CAL_COUNTS_CMD:
				comms_hpt_handle_ana_set_cal_counts(&msg->AnaSetCalCountsCmd, &g_msg_rsp);
				break;
			case HPT_ANA_SET_ACTIVE_COUNTS_CMD:
				comms_hpt_handle_ana_set_active_counts(&msg->AnaSetActiveCountsCmd, &g_msg_rsp);
				break;
			default:
				// g_msg_rsp.Length += 0;
				g_msg_rsp.CmdRsp = HPT_UNKNOWN_COMMAND_RSP;
				break;
		}
	}

	if (g_msg_rsp.CmdRsp == HPT_FAILED_COMMAND_RSP) {
		g_msg_rsp.Length += sizeof(g_msg_rsp.FailureRsp.Failures);
		g_msg_rsp.Length += sizeof(HPT_FailureCode) * g_msg_rsp.FailureRsp.Failures;
	}

	if (g_msg_rsp.Length != 0) {
		uint32_t crc_index = (g_msg_rsp.Length-4)/4;
		g_msg_rsp.RawData32Bit[crc_index] = HAL_CRC_Calculate(&hcrc, g_msg_rsp.RawData32Bit, crc_index);
	}

	return g_msg_rsp.Length;

#undef COMMS_CHECK_DISPATCH
}

typedef enum {
	COMMS_HPT_RX_STATE_START,
	COMMS_HPT_RX_STATE_LENGTH_0,
	COMMS_HPT_RX_STATE_LENGTH_1,
	COMMS_HPT_RX_STATE_CMD,
	COMMS_HPT_RX_STATE_PAYLOAD,
	COMMS_HPT_RX_STATE_CRC_0,
	COMMS_HPT_RX_STATE_CRC_1,
	COMMS_HPT_RX_STATE_CRC_2,
	COMMS_HPT_RX_STATE_CRC_3,
} comms_hpt_rx_state;
comms_hpt_rx_state g_comms_hpt_rx_state = COMMS_HPT_RX_STATE_START;
uint32_t g_comms_hpt_rx_count = 0; // payload index

void comms_usb_hpt_receive_bytes(uint8_t *bytes, uint32_t nbytes, void **send_data, uint32_t *send_len)
{
	printf("rec %ld state=%d\n", nbytes, g_comms_hpt_rx_state);
	uint32_t total_crc = 0;
	*send_data = NULL;
	*send_len = 0;
	uint32_t i=0;
	while (i < nbytes) {
		switch (g_comms_hpt_rx_state) {
			case COMMS_HPT_RX_STATE_START:
				if (bytes[i] == '~') {
					g_msg_cmd.StartChar = bytes[i];
					g_comms_hpt_rx_state = COMMS_HPT_RX_STATE_LENGTH_0;
				}
				i++;
				break;
			case COMMS_HPT_RX_STATE_LENGTH_0:
				g_msg_cmd.Length = bytes[i];
				g_comms_hpt_rx_state = COMMS_HPT_RX_STATE_LENGTH_1;
				i++;
				break;
			case COMMS_HPT_RX_STATE_LENGTH_1:
				g_msg_cmd.Length |= ((uint16_t)bytes[i] << 8);
				if (g_msg_cmd.Length > (64 + HPT_MAX_CMD_PAYLOAD)) {
					g_comms_hpt_rx_state = COMMS_HPT_RX_STATE_START;
					break;
				} else {
					g_comms_hpt_rx_state = COMMS_HPT_RX_STATE_CMD;
				}
				i++;
				break;
			case COMMS_HPT_RX_STATE_CMD:
				g_msg_cmd.CmdRsp = bytes[i];
				g_comms_hpt_rx_count = 0;
				g_comms_hpt_rx_state = COMMS_HPT_RX_STATE_PAYLOAD;
				i++;
				break;
			case COMMS_HPT_RX_STATE_PAYLOAD:
				while (i < nbytes && g_comms_hpt_rx_count < (uint32_t)(g_msg_cmd.Length - 8)) {
					g_msg_cmd.RawData[4 + g_comms_hpt_rx_count] = bytes[i];
					g_comms_hpt_rx_count++;
					i++;
				}
				if (g_comms_hpt_rx_count == (uint32_t)(g_msg_cmd.Length - 8)) {
					g_comms_hpt_rx_state = COMMS_HPT_RX_STATE_CRC_0;
				}
				break;
			case COMMS_HPT_RX_STATE_CRC_0:
				g_msg_cmd.RawData[g_msg_cmd.Length - 4] = bytes[i];
				g_comms_hpt_rx_state = COMMS_HPT_RX_STATE_CRC_1;
				i++;
				break;
			case COMMS_HPT_RX_STATE_CRC_1:
				g_msg_cmd.RawData[g_msg_cmd.Length - 3] = bytes[i];
				g_comms_hpt_rx_state = COMMS_HPT_RX_STATE_CRC_2;
				i++;
				break;
			case COMMS_HPT_RX_STATE_CRC_2:
				g_msg_cmd.RawData[g_msg_cmd.Length - 2] = bytes[i];
				g_comms_hpt_rx_state = COMMS_HPT_RX_STATE_CRC_3;
				i++;
				break;
			case COMMS_HPT_RX_STATE_CRC_3:
				g_msg_cmd.RawData[g_msg_cmd.Length - 1] = bytes[i];
				// validate CRC
				total_crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)&g_msg_cmd.RawData32Bit[0], g_msg_cmd.Length/4);
				if (total_crc == 0) {
					comms_usb_hpt_receive_msg(&g_msg_cmd);
					// comms_usb_hpt_receive_msg fills out g_msg_rsp
					if (g_msg_rsp.Length != 0) {
						*send_data = &g_msg_rsp;
						*send_len = (uint32_t)g_msg_rsp.Length;
					}
				}
				g_comms_hpt_rx_state = COMMS_HPT_RX_STATE_START;
				i++;
				break;
			default:
				g_comms_hpt_rx_state = COMMS_HPT_RX_STATE_START;
				break;
		}
	}
}

void comms_usb_hpt_receive_bytes_onepacket(uint8_t *bytes, uint32_t nbytes, void **send_data, uint32_t *send_len)
{
	// TODO: message split across USB packets?
	if (nbytes > 7) {
		uint16_t len = (uint16_t)bytes[1] | ((uint16_t)bytes[2] << 8);
		if (bytes[0] == '~' && nbytes >= len) {
			// validate CRC
			uint32_t total_crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)bytes, len/4);
			if (total_crc == 0)
			{
				// re-interpret as command
				HPT_MsgCmd *ptr_msg = (HPT_MsgCmd*)bytes;
				comms_usb_hpt_receive_msg(ptr_msg);
				// comms_usb_hpt_receive_msg fills out g_msg_rsp
				if (g_msg_rsp.Length != 0) {
					*send_data = &g_msg_rsp;
					*send_len = (uint32_t)g_msg_rsp.Length;
				} else {
					*send_data = NULL;
					*send_len = 0;
				}
			}
		}
	}
}

void comms_usb_hpt_tick(void)
{
	// TODO: Request FIFO?

	g_comms_cmd_req_state = 1;
	switch (g_comms_cmd_req) {
		case HPT_ERASE_CHIP_CMD:
			puts("[comms_usb_hpt_tick] Handling HPT_ERASE_CHIP_CMD");
			g_comms_cmd_req_state = 2;
			DetReset();
			g_comms_cmd_req_state = 3;
			gDetApi->EraseChip();
			break;
		case HPT_ERASE_SECTOR_CMD:
			puts("[comms_usb_hpt_tick] Handling HPT_ERASE_SECTOR_CMD");
			g_comms_cmd_req_state = 2;
			DetReset();
			g_comms_cmd_req_state = 3;
			gDetApi->EraseSector(m_cmd_copy.EraseSectorCmd.SectorAddress);
			break;
		case HPT_PROGRAM_SECTOR_CMD:
			puts("[comms_usb_hpt_tick] Handling HPT_PROGRAM_SECTOR_CMD");
			g_comms_cmd_req_state = 2;
			DetReset();
			g_comms_cmd_req_state = 3;
			DetCmdProgramSector(m_cmd_copy.ProgramSectorCmd.SectorAddress, m_cmd_copy.ProgramSectorCmd.ProgramValue);;
			break;
		case HPT_PROGRAM_CHIP_CMD:
			puts("[comms_usb_hpt_tick] Handling HPT_PROGRAM_CHIP_CMD");
			g_comms_cmd_req_state = 2;
			DetReset();
			g_comms_cmd_req_state = 3;
			for (uint32_t i=0; i<1024; i++) {
				g_comms_cmd_req_state = 4 + i;
				DetCmdProgramSector(i * 0x10000, m_cmd_copy.ProgramChipCmd.ProgramValue);
			}
			break;
		case HPT_WRITE_DATA_CMD:
			puts("[comms_usb_hpt_tick] Handling HPT_WRITE_DATA_CMD");
			g_comms_cmd_req_state = 2;
			gDetApi->ProgramBuffer(m_cmd_copy.WriteDataCmd.BaseAddress, m_cmd_copy.WriteDataCmd.Data, m_cmd_copy.WriteDataCmd.NumWords);
			break;
		default:
			if (g_comms_cmd_req != HPT_NULL_MSG_CMD)
				printf("[comms_usb_hpt_tick] Unimplemented command %d\n", g_comms_cmd_req);
			break;
	}

	g_comms_cmd_req = HPT_NULL_MSG_CMD;
	g_comms_cmd_req_state = 0;
}
