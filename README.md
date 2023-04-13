# NR1 Controller Module Firmware

NR1 Controller Module firmware for STM Nucleo-H7A3.

## Building
Requires recent `arm-none-eabi-gcc` (c17 support) and either `stm32prog` or `stflash` for `make flash`.

To build:

```make -j $(nproc)```

## Flashing
To flash (will make if necessary):

```make flash```
