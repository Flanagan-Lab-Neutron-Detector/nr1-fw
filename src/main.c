/** @file main.c
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
#include <stdbool.h>
#include "comms_usb_hpt.h"
#include "comms_hpt_msgs.h"
#include "det_ctrl.h"
#include "analog.h"

/* Private variables ---------------------------------------------------------*/

CRC_HandleTypeDef  hcrc;
OSPI_HandleTypeDef hospi1;
SPI_HandleTypeDef  hspi1;
UART_HandleTypeDef huart3;
TIM_HandleTypeDef  htim12;
TIM_HandleTypeDef  htim13;

volatile int gMainLoopSemaphore;
// TODO: Move this to a header?
uint32_t g_config_save_requested;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_OCTOSPI1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM12_Init(void);
static void MX_TIM13_Init(void);

extern void HAL_TIM_MspPostInit(TIM_HandleTypeDef *handle);

/* Private user code ---------------------------------------------------------*/

typedef union __packed __aligned(4) {
	struct {
		float DAC1_CalC0;
		float DAC1_CalC1;
		float DAC2_CalC0;
		float DAC2_CalC1;
		uint32_t nwrites;
	};
	struct {
		uint8_t raw_payload[8192-4];
		uint32_t crc;
	};
} FlashConfig;

extern unsigned char _app_config_block_0, _app_config_block_1;
static FlashConfig *mAppConfigBlock0 = (FlashConfig *)&_app_config_block_0;
static FlashConfig *mAppConfigBlock1 = (FlashConfig *)&_app_config_block_1;
FlashConfig gAppRamConfig;

// save ram config to flash
void FlashConfigSave(void)
{
	HAL_StatusTypeDef status = HAL_OK;
	// pull in config
	// TODO: do this better
	gAppRamConfig.DAC1_CalC0 = gDac1.CalC0;
	gAppRamConfig.DAC1_CalC1 = gDac1.CalC1;
	gAppRamConfig.DAC2_CalC0 = gDac2.CalC0;
	gAppRamConfig.DAC2_CalC1 = gDac2.CalC1;
	gAppRamConfig.nwrites++;
	// calculate CRC in ram
	gAppRamConfig.crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)gAppRamConfig.raw_payload, sizeof(gAppRamConfig) / 4 - 1);
	// Write to flash
	status = HAL_FLASH_Unlock();
	if (status != HAL_OK) {
		Error_Handler();
	}
	// erase sectors 254 and 255
	FLASH_EraseInitTypeDef erase_params;
	erase_params.TypeErase = FLASH_TYPEERASE_SECTORS;
	erase_params.Banks = FLASH_BANK_2;
	erase_params.Sector = FLASH_SECTOR_126;
	erase_params.NbSectors = 2;
	erase_params.VoltageRange = 0;
	uint32_t sectorError = 0;
	status = HAL_FLASHEx_Erase(&erase_params, &sectorError);
	if (status != HAL_OK)
	{
		Error_Handler();
	}
	// wait for erase to complete
	// write to sector 254
	for (uint32_t i = 0; i < sizeof(FlashConfig); i += 16)
	{
		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, (uint32_t)(&_app_config_block_0 + i), (uint32_t)((uint8_t *)&gAppRamConfig + i));
		if (status != HAL_OK)
		{
			Error_Handler();
		}
	}
	for (uint32_t i = 0; i < sizeof(FlashConfig); i += 16)
	{
		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, (uint32_t)(&_app_config_block_1 + i), (uint32_t)((uint8_t *)&gAppRamConfig + i));
		if (status != HAL_OK)
		{
			Error_Handler();
		}
	}
	status = HAL_FLASH_Lock();
	if (status != HAL_OK)
	{
		Error_Handler();
	}
}

void FlashConfigReset(void) {
	// Reset to default values
	gAppRamConfig.DAC1_CalC0 = 0.0f;
	gAppRamConfig.DAC1_CalC1 = 1.0f;
	gAppRamConfig.DAC2_CalC0 = 0.0f;
	gAppRamConfig.DAC2_CalC1 = 1.0f;
	gAppRamConfig.nwrites = 0;
	FlashConfigSave();
}

void FlashConfigInit(void) {
	// Validate CRCs
	uint32_t block_0_good = 0;
	uint32_t block_1_good = 0;
	uint32_t crc0 = HAL_CRC_Calculate(&hcrc, (uint32_t *)&_app_config_block_0, sizeof(FlashConfig) / 4 - 1);
	uint32_t crc1 = HAL_CRC_Calculate(&hcrc, (uint32_t *)&_app_config_block_1, sizeof(FlashConfig) / 4 - 1);
	if (crc0 == mAppConfigBlock0->crc) {
		block_0_good = 1;
		for (uint32_t i=0; i<sizeof(FlashConfig); i++) {
			((uint8_t *)&gAppRamConfig)[i] = ((uint8_t *)mAppConfigBlock0)[i];
		}
	} else if (crc1 == mAppConfigBlock1->crc) {
		block_1_good = 1;
		for (uint32_t i = 0; i < sizeof(FlashConfig); i++) {
			((uint8_t *)&gAppRamConfig)[i] = ((uint8_t *)mAppConfigBlock1)[i];
		}
	} else {
		block_0_good = 1;
		block_1_good = 1;
		FlashConfigReset();
	}
}

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* Enable I-Cache---------------------------------------------------------*/
	SCB_EnableICache();

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_CRC_Init();
	MX_OCTOSPI1_Init();
	MX_SPI1_Init();
	MX_USART3_UART_Init();
	MX_TIM12_Init();
	MX_TIM13_Init();
	MX_USB_DEVICE_Init();

	// Load config from flash
	FlashConfigInit();

	DacInit(gAppRamConfig.DAC1_CalC0, gAppRamConfig.DAC1_CalC1, gAppRamConfig.DAC2_CalC0, gAppRamConfig.DAC2_CalC1);
	// The detector driver may take a while to reset
	HAL_Delay(200);
	DetCtrlInit();

	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
	HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_1);
	HAL_TIM_Base_Start_IT(&htim13);

	#define DET_RESET_PERIOD 100
	uint32_t det_reset_count = 0;

	/* Infinite loop */
	while (1)
	{
		while (gMainLoopSemaphore) {
			gMainLoopSemaphore = 0;
			HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
			comms_usb_hpt_tick();
			if (g_config_save_requested) {
				g_config_save_requested = 0;
				FlashConfigSave();
			}
			HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
		}

		det_reset_count++;
		if (det_reset_count >= DET_RESET_PERIOD) {
			det_reset_count = 0;
			// disable comms interrupt
			NVIC_DisableIRQ(OTG_HS_IRQn);
			if (!gDetVtRequested) {
				DetExitVtMode();
			}
			gDetVtRequested = 0;
			// enable comms interrupt
			NVIC_EnableIRQ(OTG_HS_IRQn);
		}

		HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	}
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/*AXI clock gating */
	RCC->CKGAENR = 0xFFFFFFFF;

	/** Supply configuration update enable
	*/
	HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

	/** Configure the main internal regulator output voltage
	*/
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

	while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 70;
	RCC_OscInitStruct.PLL.PLLP = 2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	RCC_OscInitStruct.PLL.PLLR = 2;
	RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
	RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
	RCC_OscInitStruct.PLL.PLLFRACN = 0;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
		| RCC_CLOCKTYPE_PCLK1   | RCC_CLOCKTYPE_PCLK2
		| RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
	RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief CRC Initialization Function
 * @param None
 * @retval None
 */
static void MX_CRC_Init(void)
{
	hcrc.Instance = CRC;
	hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_DISABLE;
	hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_DISABLE;
	hcrc.Init.GeneratingPolynomial = 79764919; // Castagnoli
	hcrc.Init.CRCLength = CRC_POLYLENGTH_32B;
	hcrc.Init.InitValue = 0x00000007; // NR0 protocol initial value TODO: replace with constant
	hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
	hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
	hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;
	if (HAL_CRC_Init(&hcrc) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief OCTOSPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_OCTOSPI1_Init(void)
{
	OSPIM_CfgTypeDef sOspiManagerCfg = {0};

	/* OCTOSPI1 parameter configuration*/
	hospi1.Instance = OCTOSPI1;
	hospi1.Init.FifoThreshold = 1;
	hospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
	hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
	hospi1.Init.DeviceSize = 32;
	hospi1.Init.ChipSelectHighTime = 1;
	hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
	hospi1.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
	hospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
	hospi1.Init.ClockPrescaler = 12;
	hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
	hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
	hospi1.Init.ChipSelectBoundary = 0;
	hospi1.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
	hospi1.Init.MaxTran = 0;
	hospi1.Init.Refresh = 0;
	if (HAL_OSPI_Init(&hospi1) != HAL_OK)
	{
		Error_Handler();
	}
	sOspiManagerCfg.ClkPort = 1;
	sOspiManagerCfg.NCSPort = 1;
	sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
	if (HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{
	/* SPI1 parameter configuration*/
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 0x0;
	hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
	hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
	hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
	hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
	hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
	hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
	hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
	hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
	hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
	if (HAL_SPI_Init(&hspi1) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief USART3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART3_UART_Init(void)
{
	huart3.Instance = USART3;
	huart3.Init.BaudRate = 115200;
	huart3.Init.WordLength = UART_WORDLENGTH_8B;
	huart3.Init.StopBits = UART_STOPBITS_1;
	huart3.Init.Parity = UART_PARITY_NONE;
	huart3.Init.Mode = UART_MODE_TX_RX;
	huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart3.Init.OverSampling = UART_OVERSAMPLING_16;
	huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
	huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart3) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief TIM12 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM12_Init(void)
{
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};
	TIM_OC_InitTypeDef sConfigOC = {0};

	htim12.Instance = TIM12;
	htim12.Init.Prescaler = 0;
	htim12.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim12.Init.Period = 65535;
	htim12.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim12) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim12, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_PWM_Init(&htim12) != HAL_OK)
	{
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 4096;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim12, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
	HAL_TIM_MspPostInit(&htim12);
}

/**
 * @brief TIM13 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM13_Init(void)
{
	TIM_ClockConfigTypeDef sClockSourceConfig = {0};

	htim13.Instance = TIM13;
	htim13.Init.Prescaler = 128;
	htim13.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim13.Init.Period = 65535;
	htim13.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
	htim13.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim13) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim13, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, LD1_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : nP15V_GOOD_Pin nN15V_GOOD_Pin */
	GPIO_InitStruct.Pin = nP15V_GOOD_Pin|nN15V_GOOD_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : LD1_Pin LD3_Pin */
	GPIO_InitStruct.Pin = LD1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : USB_FS_OVCR_Pin */
	GPIO_InitStruct.Pin = USB_FS_OVCR_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(USB_FS_OVCR_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : ANA_ACK_Pin ANA_ERR_Pin */
	GPIO_InitStruct.Pin = ANA_ACK_Pin|ANA_ERR_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

	/*Configure GPIO pin : LD2_Pin */
	GPIO_InitStruct.Pin = LD2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

	// Configure GPIO pin: DCE_SW_Pin
	GPIO_InitStruct.Pin  = DCE_SW_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_MEDIUM;
	HAL_GPIO_Init(DCE_SW_Port, &GPIO_InitStruct);
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
       eg: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
