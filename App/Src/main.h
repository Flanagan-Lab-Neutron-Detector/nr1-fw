/** @file main.h
 * Main header
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define OSC32_IN_Pin GPIO_PIN_14
#define OSC32_IN_GPIO_Port GPIOC
#define OSC32_OUT_Pin GPIO_PIN_15
#define OSC32_OUT_GPIO_Port GPIOC
#define PH0_MCU_Pin GPIO_PIN_0
#define PH0_MCU_GPIO_Port GPIOH
#define PH1_MCU_Pin GPIO_PIN_1
#define PH1_MCU_GPIO_Port GPIOH
#define nP15V_GOOD_Pin GPIO_PIN_2
#define nP15V_GOOD_GPIO_Port GPIOA
#define nN15V_GOOD_Pin GPIO_PIN_3
#define nN15V_GOOD_GPIO_Port GPIOA
#define LD1_Pin GPIO_PIN_0
#define LD1_GPIO_Port GPIOB
#define LD3_Pin GPIO_PIN_14
#define LD3_GPIO_Port GPIOB
#define STLINK_RX_Pin GPIO_PIN_8
#define STLINK_RX_GPIO_Port GPIOD
#define STLINK_TX_Pin GPIO_PIN_9
#define STLINK_TX_GPIO_Port GPIOD
#define USB_FS_OVCR_Pin GPIO_PIN_7
#define USB_FS_OVCR_GPIO_Port GPIOG
#define ANA_ACK_Pin GPIO_PIN_8
#define ANA_ACK_GPIO_Port GPIOG
#define USB_FS_VBUS_Pin GPIO_PIN_9
#define USB_FS_VBUS_GPIO_Port GPIOA
#define USB_FS_ID_Pin GPIO_PIN_10
#define USB_FS_ID_GPIO_Port GPIOA
#define USB_FS_N_Pin GPIO_PIN_11
#define USB_FS_N_GPIO_Port GPIOA
#define USB_FS_P_Pin GPIO_PIN_12
#define USB_FS_P_GPIO_Port GPIOA
#define ANA_ERR_Pin GPIO_PIN_14
#define ANA_ERR_GPIO_Port GPIOG
#define LD2_Pin GPIO_PIN_1
#define LD2_GPIO_Port GPIOE

#define DCE_SW_Pin   GPIO_PIN_10
#define DCE_SW_Port  GPIOC

#define CE_SWITCH_DIG HAL_GPIO_WritePin(DCE_SW_Port, DCE_SW_Pin, GPIO_PIN_SET)
#define CE_SWITCH_ANA HAL_GPIO_WritePin(DCE_SW_Port, DCE_SW_Pin, GPIO_PIN_RESET)

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
