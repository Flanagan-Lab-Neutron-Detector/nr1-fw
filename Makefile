######################################
# target
######################################
TARGET = nr1b-h7a3
PROGRAMMER ?= stm32prog


######################################
# building variables
######################################
# debug build?
DEBUG = 1
# optimization
OPT = -O0 -Og -std=c17


#######################################
# paths
#######################################
# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources
C_SOURCES =  \
src/det_driver_qspi.c \
src/det_ctrl.c \
src/syscalls.c \
src/comms_usb_hpt.c \
src/main.c \
src/stm32h7xx_it.c \
src/stm32h7xx_hal_msp.c \
src/USB_DEVICE/App/usb_device.c \
src/USB_DEVICE/App/usbd_desc.c \
src/USB_DEVICE/App/usbd_cdc_if.c \
src/USB_DEVICE/Target/usbd_conf.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usb.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_crc.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_crc_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ospi.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c \
src/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c \
src/system_stm32h7xx.c \
src/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c \
src/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
src/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c \
src/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c

# ASM sources
ASM_SOURCES =  \
src/startup_stm32h7a3xxq.s


#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

ifeq ($(PROGRAMMER), stm32prog)
PROG = STM32_Programmer_CLI
PROG_PORT = swd
PROG_FREQ = 24000
PROG_ADDR = 0x8000000
PROG_ARGS = -c port=$(PROG_PORT) freq=$(PROG_FREQ) -d $(BUILD_DIR)/$(TARGET).bin $(PROG_ADDR) -v
else ifeq ($(PROGRAMMER), stflash)
PROG = st-flash
PROG_ARGS = --reset write $(BUILD_DIR)/$(TARGET).bin 0x8000000
endif

#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m7

# fpu
FPU = -mfpu=fpv5-d16

# float-abi
FLOAT-ABI = -mfloat-abi=hard

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS =

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32H7A3xxQ \
-DDEBUG


# AS includes
AS_INCLUDES =

# C includes
C_INCLUDES =  \
-Isrc/USB_DEVICE/App \
-Isrc/USB_DEVICE/Target \
-Isrc \
-Isrc/Drivers/STM32H7xx_HAL_Driver/Inc \
-Isrc/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy \
-Isrc/Middlewares/ST/STM32_USB_Device_Library/Core/Inc \
-Isrc/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc \
-Isrc/Drivers/CMSIS/Device/ST/STM32H7xx/Include \
-Isrc/Drivers/CMSIS/Include


# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -Werror -fdata-sections -ffunction-sections

CFLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -Wextra -Wno-unused -Werror -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif


# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32H7A3ZITxQ_FLASH.ld

# libraries
LIBS = -lc -lm #-lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections
#LDFLAGS = $(MCU) -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir $@

#######################################
# flash
#######################################
flash: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	$(PROG) $(PROG_ARGS)
# st-flash erase
# st-flash --reset write $(BUILD_DIR)/$(TARGET).bin 0x8000000

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

#######################################
# extra
#######################################
.PHONY: all clean flash

# *** EOF ***
