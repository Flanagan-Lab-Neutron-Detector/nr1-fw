/**
 * @file syscalls.c
 * @brief Implements very basic syscalls for libc
 */

#include <sys/stat.h>
#include <sys/unistd.h>
#include <errno.h>
#include "stm32h7xx_hal.h"

extern UART_HandleTypeDef huart3;
int _write(int fd, char *buf, int count)
{
	UNUSED(fd);
	HAL_StatusTypeDef status = HAL_OK;
	status = HAL_UART_Transmit(&huart3, (uint8_t*)buf, count, 10);
	if (status == HAL_BUSY || status == HAL_ERROR)
		return -1;
	return huart3.TxXferSize - huart3.TxXferCount;
}

int _read(int fd, char *buf, int count)
{
	UNUSED(fd);
	HAL_StatusTypeDef status = HAL_OK;
	status = HAL_UART_Receive(&huart3, (uint8_t*)buf, count, 500);
	if (status == HAL_BUSY || status == HAL_ERROR)
		return -1;
	return huart3.RxXferSize - huart3.RxXferCount;
}

extern unsigned char _heap_start;
extern unsigned char _heap_end;
void *_sbrk(int incr)
{
	// TODO: Make sure heap is in SRAM, not DTCM

	static unsigned char *heap = NULL;
	unsigned char *prev_heap;

	if (heap == NULL) {
		heap = (unsigned char *)&_heap_start;
	}

	unsigned char *stack = (unsigned char *)__get_MSP();
	if ((intptr_t)(heap + incr) > (intptr_t)stack) {
#ifdef DEBUG
		_write(STDERR_FILENO, "[_sbrk] Error: Requested increment collides with stack.\n", 57);
#endif // DEBUG
		errno = ENOMEM;
		return (void*)-1;
	}
	prev_heap = heap;
	heap += incr;
	return prev_heap;
}

int _close(int file) {
	UNUSED(file);
	return -1;
}

int _fstat(int file, struct stat *st) {
	UNUSED(file);
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file) {
	UNUSED(file);
	return 1;
}

int _lseek(int file, int ptr, int dir) {
	UNUSED(file);
	UNUSED(ptr);
	UNUSED(dir);
	return 0;
}

void _exit(int status) {
	UNUSED(status);
	__asm("BKPT #0");
	while (1) {}
}

void _kill(int pid, int sig) {
	UNUSED(pid);
	UNUSED(sig);
	return;
}

int _getpid(void) {
	return -1;
}
