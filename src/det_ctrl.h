/**
 * @file det_ctrl.h
 * @brief High-level detector control
 */

#ifndef DET_CTRL_H
#define DET_CTRL_H

#if defined(__cplusplus)
extern "C" {
#endif

#undef EXTERN
#ifdef DET_CTRL_C
#define EXTERN
#else
#define EXTERN extern
#endif /* DET_CTRL_C */

#include <stdint.h>

// configuration
#define DET_IS_IN_MODE_BYTE

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//		DETECTOR Vt MODE
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

EXTERN uint32_t					gDetIsInVtMode;
EXTERN uint16_t					gDetVt_mV;				///< Current Vt voltage in mV

typedef enum {
	DET_MODE_BYTE,
	DET_MODE_WORD
} det_mode;

EXTERN det_mode gDetMode;

extern void DetReset(void);
extern int DetEnterVtMode(void);
extern int DetSetVt(uint32_t vt_mv);
extern int DetExitVtMode(void);

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//		API
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct
{
	void (*Open)(void);
	void (*Close)(void);

	uint16_t	(*ReadWord)(uint32_t addr);
	void		(*ReadWords)(uint32_t *addrs, uint16_t *dest, uint32_t count);
	void 		(*ReadBlock)(uint32_t base, uint32_t count, uint16_t *dest);
	void		(*ReadPage)(uint32_t PageAddress, uint16_t *dest);
	void		(*WriteWord)(uint32_t addr, uint16_t word);
	void		(*WriteWords)(uint32_t *addrs, uint16_t *words, uint32_t count);
	void		(*WriteCommandWord)(uint32_t addr, uint16_t word);
	void		(*WriteCommandWords)(uint32_t *addrs, uint16_t *words, uint32_t count);
	void		(*WriteBlock)(uint32_t base, uint32_t count, void *block);
	void        (*ProgramWord)(uint32_t address, uint16_t word);
	void		(*ProgramBuffer)(uint32_t SectorAddress, void *data, uint16_t count);
	void		(*ProgramBuffer_single)(uint32_t SectorAddress, uint16_t word, uint16_t count);
	void		(*EraseSector)(uint32_t SectorAddress);
	void		(*EraseChip)(void);
	void        (*EnterVt)(void);
	void        (*ExitVt)(void);
} S_DetApi;

EXTERN S_DetApi			*gDetApi;



//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//		DETECTOR
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

typedef struct
{
	struct
	{
		// ID/Autoselect area
		uint16_t		ManufacturerID;
		uint16_t		DeviceID1;
		uint16_t		SectorProtectionVerification;
		uint16_t		IndicatorBits;
		uint16_t		RFU[8];
		uint16_t		LowerSoftwareBits;
		uint16_t		UpperSoftwareBits;
		uint16_t		DeviceID2;
		uint16_t		DeviceID3;
	} ID;

	struct
	{
		// CFI query identification string
		uint16_t		Q, R, Y;
		uint16_t		MfgID_0, MfgID_1;
		uint16_t		ExtTableAddr_0, ExtTableAddr_1;
		uint16_t		AltMfgId_0, AltMfgId_1;
		uint16_t		AltExtTableAddr_0, AltExtTableAddr_1;
	} CfiQuery;

	struct
	{
		// CFI System Interface String
		uint16_t		VccMin, VccMax;
		uint16_t		VppMin, VppMax;
		uint16_t		TypTimeSingleWordWrite;
		uint16_t		TypTimeMaxMultiByteProgram;
		uint16_t		TypTimeBlockErase;
		uint16_t		TypTimeChipErase;
		uint16_t		MaxTimeSingleWordWrite;
		uint16_t		MaxTimeBufferWrite;
		uint16_t		MaxTimeBlockErase;
		uint16_t		MaxTimeChipErase;
	} CfiInterface;

	struct
	{
		// CFI Device Geometry Definition
		uint16_t		Size;
		uint16_t		Interface_0, Interface_1;
		uint16_t		MaxBytesInMultiByteWrite_0, MaxBytesInMultiByteWrite_1;
		uint16_t		EraseBlockRegions;
		uint16_t		EraseBlockRegion1Info_0, EraseBlockRegion1Info_1, EraseBlockRegion1Info_2, EraseBlockRegion1Info_3;
		uint16_t		EraseBlockRegion2Info_0, EraseBlockRegion2Info_1, EraseBlockRegion2Info_2, EraseBlockRegion2Info_3;
		uint16_t		EraseBlockRegion3Info_0, EraseBlockRegion3Info_1, EraseBlockRegion3Info_2, EraseBlockRegion3Info_3;
		uint16_t		EraseBlockRegion4Info_0, EraseBlockRegion4Info_1, EraseBlockRegion4Info_2, EraseBlockRegion4Info_3;
		uint16_t		Reserved[3];
	} CfiGeo;

	struct
	{
		// CFI Primary Vendor-Specific Extended Query
		uint16_t		P, R, I;
		uint16_t		VersionMajor, VersionMinor;
		uint16_t		AddressSensitiveUnlock_ProcessTechnology;
		uint16_t		EraseSuspend;
		uint16_t		SectorProtect;
		uint16_t		TempSectorUnprotect;
		uint16_t		SectorProtectScheme;
		uint16_t		SimultaneousOperation;
		uint16_t		BurstModeType;
		uint16_t		PageModeType;
		uint16_t		AccSupplyMin, AccSupplyMax;
		uint16_t		WpProtection;
		uint16_t		ProgramSuspend;
		uint16_t		UnlockBypass;
		uint16_t		SecuredSiliconSectorSize;
		uint16_t		SoftwareFeatures;
		uint16_t		PageSize;
		uint16_t		MaxEraseSuspendTimeout;
		uint16_t		MaxProgramSuspendTimeout;
		uint16_t		Reserved[32];
		uint16_t		MaxEmbeddedHardwareResetTimeout;
		uint16_t		MaxNonEmbeddedHardwareResetTimeout;
	} CfiExtQuery;

	struct
	{
		uint16_t		ElectronicMarkingSize;
		uint16_t		ElectronicMarkingRev;
		uint16_t		FabLot[7];
		uint16_t		Wafer;
		uint16_t		DieXCoord, DieYCoord;
		uint16_t		ClassLot[7];
		uint16_t		RFU[13];
	} IdCfiAsoMap;

} S_DeviceInformation;

EXTERN S_DeviceInformation	gDetInfo;

EXTERN uint32_t				gDetIsBusy;

extern void DetCtrlInit(void);
extern void TaskDetCtrl(void);

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
//		DETECTOR COMMANDS
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

extern void DetCmdProgramSector(uint32_t address, uint16_t word);
extern uint32_t DetCmdCountBitsSector(uint32_t address);
extern void DetCmdCountBitsKPage(uint32_t address, uint8_t *countBlock);
extern void DetCmdVtReadVoltageBlock(void);

#if defined(__cplusplus)
}
#endif

#endif /* !DET_CTRL_H */
