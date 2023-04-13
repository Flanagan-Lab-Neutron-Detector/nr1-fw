/**
 * @file comms_hpt_msgs.h
 * @brief HPT protocol message definitions
 */

#ifndef COMMS_HPT_MSGS_H
#define COMMS_HPT_MSGS_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <cmsis_compiler.h>

// start character 0x7E = '~'
#define		HPT_MSG_SOM_CHAR		0x7E

#define     HPT_SIZE_OF_HEADER      4
#define     HPT_SIZE_OF_CRC         4

#define     HPT_CRC_INITIAL_SEED    7

/**
 * @brief HPT Bus command/response enum. Unique ID for each message (command or response). Valid range: [0,255]
 * 
 */
typedef enum __attribute__((__packed__))
{
	HPT_NULL_MSG_CMD				= 0,		// not allowed
	
	HPT_PING_CMD					= 2,
	HPT_PING_RSP					= 3,

	HPT_UNKNOWN_COMMAND_RSP			= 4,			// unknown command received
	HPT_FAILED_COMMAND_RSP			= 5,			// bad msg crc etc = retry

	HPT_VT_GET_BIT_COUNT_KPAGE_CMD	= 6,			// get bit counts for 1024 pages
	HPT_VT_GET_BIT_COUNT_KPAGE_RSP	= 7,

	HPT_ERASE_CHIP_CMD				= 8,			// erase whole chip
	HPT_ERASE_CHIP_RSP				= 9,
	HPT_ERASE_SECTOR_CMD			= 10,			// erase sector
	HPT_ERASE_SECTOR_RSP			= 11,
	HPT_PROGRAM_SECTOR_CMD			= 12,			// program sector with word
	HPT_PROGRAM_SECTOR_RSP			= 13,
	HPT_PROGRAM_CHIP_CMD			= 14,			// program chip with word
	HPT_PROGRAM_CHIP_RSP			= 15,
	HPT_GET_SECTOR_BIT_COUNT_CMD	= 16,			// get number of 1 bits in a sector
	HPT_GET_SECTOR_BIT_COUNT_RSP	= 17,
	HPT_READ_DATA_CMD				= 18,			// read 512 words (16 pages) from chip
	HPT_READ_DATA_RSP				= 19,
	HPT_WRITE_DATA_CMD				= 20,			// write up to 512 words (16 pages)
	HPT_WRITE_DATA_RSP				= 21,
	HPT_READ_WORD_CMD				= 22,			// read single word
	HPT_READ_WORD_RSP				= 23,

	HPT_ANA_GET_CAL_COUNTS_CMD		= 80,			// analog: get calibration counts for all channels
	HPT_ANA_GET_CAL_COUNTS_RSP		= 81,
	HPT_ANA_SET_CAL_COUNTS_CMD		= 82,			// analog: set calibration counts for one channel
	HPT_ANA_SET_CAL_COUNTS_RSP		= 83,
	HPT_ANA_SET_ACTIVE_COUNTS_CMD	= 84,			// analog: set channel
	HPT_ANA_SET_ACTIVE_COUNTS_RSP	= 85,

	HPT_CMD_RSP_LENGTH				= 0xFF,			// defines 1 byte for this enum (IAR) TODO: Does this work in GCC?
} HPT_CmdRespEnum;

#define		HPT_MAX_PAYLOAD		    1088
	
/////////////////////  COMMANDS  ////////////////////////

typedef __PACKED_STRUCT
{
} HPT_NoDataCmdRsp;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint32_t		UptimeSeconds;
	uint8_t			VersionString[16];
	uint32_t		IsDetectorBusy;
} HPT_PingRsp;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint32_t		BaseAddress;
	uint32_t		BitReadMv;				// read voltage in mV
} HPT_VtGetBitCountKPageCmd;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint8_t			BitCount[1024];			// 1 Byte per page, giving how many bits are 1 in that page
} HPT_VtGetBitCountKPageRsp;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint32_t		SectorAddress;
} HPT_EraseSectorCmd;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint32_t		SectorAddress;
	uint16_t		ProgramValue;
	uint16_t		_Pad[1];
} HPT_ProgramSectorCmd;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint16_t		ProgramValue;
	uint16_t		_Pad[1];
} HPT_ProgramChipCmd;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint32_t		BaseAddress;
	uint32_t		BitReadMv;				// read voltage in mV
} HPT_GetSectorBitCountCmd;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint32_t		BaseAddress;
	uint32_t		VtMode;					// true = read in Vt mode, else read normally
	uint32_t		BitReadMv;				// if VtMode, read voltage in mV
} HPT_ReadDataCmd;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint16_t		Data[512];
} HPT_ReadDataRsp;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint16_t		Data[512];
	uint32_t		BaseAddress;
	uint32_t		NumWords;				// how many words to write
} HPT_WriteDataCmd;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint32_t		BitSet;					// how many bits read as 1 in the sector
} HPT_GetSectorBitCountRsp;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint32_t		VtMode;					// true = read in Vt mode, else read normally
	uint32_t		BitReadMv;				// if VtMode, read voltage in mV
	uint32_t		WordAddress;
} HPT_ReadWordCmd;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint16_t		Word;
	uint16_t		_Pad[1];
} HPT_ReadWordRsp;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint16_t		Ce10VCalCounts;
	uint16_t		Reset10VCalCounts;
	uint16_t		WpAcc10VCalCounts;
	uint16_t		Spare10VCalCounts;
} HPT_AnaGetCalCountsRsp;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint16_t		Ce10VCalCounts;
	uint16_t		Reset10VCalCounts;
	uint16_t		WpAcc10VCalCounts;
	uint16_t		Spare10VCalCounts;
} HPT_AnaSetCalCountsCmd;

typedef __PACKED_STRUCT __ALIGNED(4)
{
	uint32_t		AnalogUnit; // 0=CE 1=RESET 2=WP/ACC 3=SPARE
	uint32_t		UnitCounts;
} HPT_AnaSetActiveCountsCmd;

/////////////////////  UNION OF ALL COMMANDS  ////////////////////////

/**
 * @brief HPT Bus message command/response structure
 * 
 * Holds header and union of all payloads. CRC is placed after
 * the message payload, at RawData32Bit[(Length-4)/4].
 * 
 */
typedef __PACKED_UNION __ALIGNED(4)
{
	uint8_t					    RawData[64 + HPT_MAX_PAYLOAD];
    uint32_t                    RawData32Bit[(64 + HPT_MAX_PAYLOAD)/4];

	__PACKED_STRUCT __ALIGNED(4) {
		uint8_t StartChar; uint16_t Length; HPT_CmdRespEnum CmdRsp;  // header data 4 bytes

		union __ALIGNED(4)
		{
			HPT_PingRsp					PingRsp;
			HPT_VtGetBitCountKPageCmd	VtGetBitCountKPageCmd;
			HPT_VtGetBitCountKPageRsp	VtGetBitCountKPageRsp;
			HPT_EraseSectorCmd			EraseSectorCmd;
			HPT_ProgramSectorCmd		ProgramSectorCmd;
			HPT_ProgramChipCmd			ProgramChipCmd;
			HPT_GetSectorBitCountCmd	GetSectorBitCountCmd;
			HPT_GetSectorBitCountRsp	GetSectorBitCountRsp;
			HPT_ReadDataCmd				ReadDataCmd;
			HPT_ReadDataRsp				ReadDataRsp;
			HPT_WriteDataCmd			WriteDataCmd;
			HPT_ReadWordCmd				ReadWordCmd;
			HPT_ReadWordRsp				ReadWordRsp;
			
			HPT_AnaGetCalCountsRsp		AnaGetCalCountsRsp;
			HPT_AnaSetCalCountsCmd		AnaSetCalCountsCmd;
			HPT_AnaSetActiveCountsCmd	AnaSetActiveCountsCmd;
			
			HPT_NoDataCmdRsp            NoDataCmdRsp;
		};
	};
} HPT_ComMsg;

#include <assert.h>
#include <stddef.h>

static_assert(sizeof(HPT_ComMsg) == 64 + HPT_MAX_PAYLOAD, "HPT_ComMsg wrong size :(");
static_assert(offsetof(HPT_ComMsg, PingRsp) == 4, "PingRsp is not at offset 4");
static_assert(offsetof(HPT_ComMsg, VtGetBitCountKPageCmd) == 4, "VtGetBitCountKPageCmd is not at offset 4");
static_assert(offsetof(HPT_ComMsg, VtGetBitCountKPageRsp) == 4, "VtGetBitCountKPageRsp is not at offset 4");
static_assert(offsetof(HPT_ComMsg, EraseSectorCmd) == 4, "EraseSectorCmd is not at offset 4");
static_assert(offsetof(HPT_ComMsg, ProgramSectorCmd) == 4, "ProgramSectorCmd is not at offset 4");
static_assert(offsetof(HPT_ComMsg, ProgramChipCmd) == 4, "ProgramChipCmd is not at offset 4");
static_assert(offsetof(HPT_ComMsg, GetSectorBitCountCmd) == 4, "PingRsp is not at offset 4");
static_assert(offsetof(HPT_ComMsg, GetSectorBitCountRsp) == 4, "GetSectorBitCountRsp is not at offset 4");
static_assert(offsetof(HPT_ComMsg, ReadDataCmd) == 4, "ReadDataCmd is not at offset 4");
static_assert(offsetof(HPT_ComMsg, ReadDataRsp) == 4, "ReadDataRsp is not at offset 4");
static_assert(offsetof(HPT_ComMsg, WriteDataCmd) == 4, "WriteDataCmd is not at offset 4");
static_assert(offsetof(HPT_ComMsg, ReadWordCmd) == 4, "ReadWordCmd is not at offset 4");
static_assert(offsetof(HPT_ComMsg, ReadWordRsp) == 4, "ReadWordRsp is not at offset 4");
static_assert(offsetof(HPT_ComMsg, AnaGetCalCountsRsp) == 4, "AnaGetCalCountsRsp is not at offset 4");
static_assert(offsetof(HPT_ComMsg, AnaSetCalCountsCmd) == 4, "AnaSetCalCountsCmd is not at offset 4");
static_assert(offsetof(HPT_ComMsg, AnaSetActiveCountsCmd) == 4, "AnaSetActiveCountsCmd is not at offset 4");
static_assert(offsetof(HPT_ComMsg, NoDataCmdRsp) == 4, "NoDataCmdRsp is not at offset 4");

#if defined(__cplusplus)
}
#endif

#endif /* !COMMS_HPT_MSGS_H */
