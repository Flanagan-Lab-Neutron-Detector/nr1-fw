/**
 * @file det_ctrl.c
 * @brief High-level detector control
 */

// Adapted from NR0 code

#define DET_CTRL_C
#include "det_ctrl.h"
#include "main.h"
#include "det_driver_sw.h"
#include "det_driver_qspi.h"
#include "analog.h"

#ifdef DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif

/*
// generate LUT for number of bits set in a 16-bit int
#define	B2(n)	n,		n+1,		n+1,		n+2
#define	B4(n)	B2(n),	B2(n+1),	B2(n+1),	B2(n+2)
#define	B6(n)	B4(n),	B4(n+1),	B4(n+1),	B4(n+2)
#define	B8(n)	B6(n),	B6(n+1),	B6(n+1),	B6(n+2)
#define	B10(n)	B8(n),	B8(n+1),	B8(n+1),	B8(n+2)
#define	B12(n)	B10(n),	B10(n+1),	B10(n+1),	B10(n+2)
#define	B14(n)	B12(n),	B12(n+1),	B12(n+1),	B12(n+2)
#define	B16(n)	B14(n),	B14(n+1),	B14(n+1),	B14(n+2)
static const uint8_t BitsSetTable65536[65536] = { B16(0) };
// TODO: Verify the above
*/

#define POPCOUNT(x) __builtin_popcount(x)

uint32_t CountBitsInRange(uint16_t *data, uint32_t offset, uint32_t count)
{
	uint32_t bitcount = 0;

	for (uint32_t w = offset; w < count; w++) {
		//bitcount += BitsSetTable65536[data[w]];
		bitcount += POPCOUNT(data[w]);
	}

	return bitcount;
}

uint8_t CountBitsInPage(uint16_t *page)
{
	uint8_t bitcount = 0;

	for (uint32_t w = 0; w < 8; w++) {
		//bitcount += BitsSetTable65536[page[w]];
		bitcount += POPCOUNT(page[w]);
	}

	return bitcount;
}

static void detDelay(uint32_t ticks)
{
	while (ticks--) __asm__("");
}

void DetReset(void)
{
//	TP1_LOW;
//	TP1_HIGH;

	//SSP_CRITICAL_SECTION_DEFINE;		// defines variables used to disable, enable interrupts

	//SSP_CRITICAL_SECTION_ENTER;		// disable interrupts

	// Set DAC outputs to safe values
	RESET_HIGH;
	WP_ACC_HIGH;

	CE_SWITCH_DIG;

	/////// For logic analyzer
	//		DIG_CE_LOW;			// low to trigger logic analyzer
	/////// For logic analyzer

	RESET_LOW;			// Set RESET# to 0V, keep chip in reset until we're ready
	WP_ACC_HIGH;		// Set WP#/ACC to 3.2V, normal operation
	//DET_OE_HIGH;		// initial condition, no output
	//DET_WE_HIGH;		// initial condition, no write
	// DET_BYTE_HIGH;		// word mode

	////		DIG_CE_HIGH;		// logic analyzer
	//		TP1_LOW;
	detDelay(10000);

	RESET_HIGH;	// Set RESET# to 3.2V, ready for use
	//		TP1_HIGH;
	detDelay(10000);
	//		TP1_LOW;
	//		TP1_HIGH;

	//SSP_CRITICAL_SECTION_EXIT;		// restore interrupts

	gDetIsInVtMode = 0;
	gDetVtRequested = 0;
}

//extern void Manual_WriteWord_internal(uint32_t addr, uint16_t word);
int DetEnterVtMode(void)
{
	gDetVtRequested = 1;
	if (gDetIsInVtMode) return 0;

	//DET_WE_HIGH;
	CE_SWITCH_ANA; // Send analog CE to chip
	detDelay(400);

	DacError dacerr = DAC_SUCCESS;

	dacerr = SET_WP_ACC_MV(12500);
	if (dacerr != DAC_SUCCESS)
		return 1;

	dacerr = RESET_HIGH;
	if (dacerr != DAC_SUCCESS)
		return 2;

	//detDelay(1000);

	if (DetSetVt(3000)) // Safe default to 3V. Users should call DetSetVt
		return 3;

	gDetApi->WriteWord(0x00000000, 0x0080);
	gDetApi->WriteWord(0x00000000, 0x0001);
	gDetApi->WriteWord(0x00000000, 0x0080);
	gDetApi->WriteWord(0x00000008, 0x0012);

	detDelay(100); // wait at least 1 us
	gDetApi->EnterVt();
	//DET_WE_LOW;
	detDelay(10); // wait at least 200 ns

	gDetIsInVtMode = 1;

	return 0;
}

int DetSetVt(uint32_t vt_mv)
{
	gDetVtRequested = 1;
	DacError dacerr = DAC_SUCCESS;
	dacerr = SET_RESET_MV(vt_mv);
	if (dacerr != DAC_SUCCESS)
		return 1;

	detDelay(10000);
	return 0;
}

int DetExitVtMode(void)
{
	if (!gDetIsInVtMode) return 0;

	DacError dacerr = DAC_SUCCESS;

	dacerr = WP_ACC_HIGH;
	if (dacerr != DAC_SUCCESS)
		return 1;
	dacerr = RESET_HIGH;
	if (dacerr != DAC_SUCCESS)
		return 2;

	detDelay(100);
	gDetApi->ExitVt();

	CE_SWITCH_DIG;					// Send digital CE to chip

	gDetIsInVtMode = 0;
	return 0;
}

void DetReadIdCfiData(S_DeviceInformation *detInfo, uint32_t SectorAddress)
{
//	uint32_t IdEntryAddrs[3] = { 0x555, 0x2AA, SectorAddress + 0x555 };
//	uint16_t IdEntryWords[3] = { 0xAA, 0x55, 0x90 };
//
//	// ID entry
//	Det_WriteWords(IdEntryAddrs, IdEntryWords, 3);

	gDetApi->WriteCommandWord(SectorAddress + 0x55, 0x98);


	// Read ID/Autoselect entry
	//gDetApi->ReadBlock(SectorAddress + 0x0000, 16, &detInfo->ID.ManufacturerID);
	//BYTE_READ(SectorAddress + 0x0000, 16, &detInfo->ID);

	// Read CFI Query Identification String
	gDetApi->ReadBlock(SectorAddress + 0x0010, 11, (uint16_t*)&detInfo->CfiQuery);

	// Read CFI System Interface String
	gDetApi->ReadBlock(SectorAddress + 0x001B, 12, (uint16_t*)&detInfo->CfiInterface);

	// Read CFI Device Geometry String
	gDetApi->ReadBlock(SectorAddress + 0x0027, 25, (uint16_t*)&detInfo->CfiGeo);

	// Read CFI Primary Vendor-Specific Extended Query
	gDetApi->ReadBlock(SectorAddress + 0x0040, 62, (uint16_t*)&detInfo->CfiExtQuery);

	// Read ID-CFI ASO Map
	gDetApi->ReadBlock(SectorAddress + 0x0080, sizeof(detInfo->IdCfiAsoMap)/2, (uint16_t*)&detInfo->IdCfiAsoMap);

#undef BYTE_READ

	// Exit
	gDetApi->WriteCommandWord(0, 0xF0);
}

void DetCtrlInit(void)
{
	//Manual_Open();
	//Bus_Open();

	RESET_HIGH;
	WP_ACC_HIGH;

	gDetMode = DET_MODE_WORD;
	//Manual_Open();
	QSPI_Open();

	DetReset();

	//det_pins_set_input();
	detDelay(10000);

	DetReadIdCfiData(&gDetInfo, 0);

	//DetReset();

	/*
	det_pins_set_input();
	detDelay(20000);

	// test erase, read, write

	uint32_t sa = 0;

	// erase sector 1
	gDetApi->EraseSector(sa*2);
	do {
		HAL_Delay(100);
	} while (DET_BUSY);

	#define LENGTH 100

	static uint16_t words[LENGTH];

	// read
	for (uint32_t i=0; i<LENGTH; i++) {
		uint16_t word = gDetApi->ReadWord(sa*2 + 2*i);
		words[i] = word;
		//if (word != 0xFFFF) Error_Handler();
	}

	// write ascending
	for (uint32_t i=0; i<LENGTH; i++) {
		gDetApi->ProgramWord(sa*2 + 2*i, i);
		do {
			HAL_Delay(1);
			// TODO: Check status lines
		} while (DET_BUSY);
	}

	// read
	for (uint32_t i=0; i<LENGTH; i++) {
		uint16_t word = gDetApi->ReadWord(sa*2 + 2*i);
		words[i] = word;
		//if (word != i) Error_Handler();
	}

	*/
}

/*
void TaskDetCtrl(void)
{
	if (DET_BUSY)
		gDetIsBusy = true;
	else
		gDetIsBusy = false;
}
*/

uint16_t mDataBlock[4096];	// 8K block = 4Kword = voltages for 256 words

void DetCmdCountBitsKPage(uint32_t address, uint8_t *countBlock)
{
	// read first half
	gDetApi->ReadBlock(address, 512*8, mDataBlock);
	// count first half
	for (uint32_t page=0; page<512; page++)
		countBlock[page] = CountBitsInPage(mDataBlock + 8*page);

	// read second half
	gDetApi->ReadBlock(address + 512*8, 512*8, mDataBlock);
	// count second half
	for (uint32_t page=512; page<1024; page++)
		countBlock[page] = CountBitsInPage(mDataBlock + 8*(page-512));
}

void DetCmdProgramSector(uint32_t address, uint16_t word)
{
	//uint32_t usdelay = 2 << gDetInfo.CfiInterface.TypTimeSingleWordWrite;
	gDetApi->ProgramBuffer_single(address, word, 65536);

	/*
	for (uint32_t i=0; i<2048; i++)
	{
		for (uint32_t j=0; j<32; j++) {
			gDetApi->ProgramWord(address + 32*i + j, word);
			HAL_Delay(1);
		}
	}
	*/
}

uint32_t DetCmdCountBitsSector(uint32_t address)
{
	uint32_t ret = 0;

	// For each 512 pages (1 sector = 16*512 pages):
	//   Read 512 pages (4096 words)
	//   Count bits
	for (uint32_t i=0; i<16; i++)
	{
		gDetApi->ReadBlock(address + 512*8*i, 512*8, mDataBlock);
		ret += CountBitsInRange(mDataBlock, 0, 512*8);
	}

	return ret;
}


