/**
 * @file comms_usb_hpt.h
 * @brief USB communications driver for HPT protocol
 */

#ifndef COMMS_USB_HPT_H
#define COMMS_USB_HPT_H

#if defined(__cplusplus)
extern "C" {
#endif

#undef EXTERN
#ifdef COMMS_USB_HPT_C
#define EXTERN
#else
#define EXTERN extern
#endif

#include <stdint.h>

EXTERN uint8_t g_comms_cmd_req;
EXTERN uint32_t g_comms_cmd_req_state;

void comms_usb_hpt_reset(void);

/**
 * @brief HPT Bus command/response handler.
 * 
 * Validates HPT Bus command and passes data to response handler.
 * Only pass complete messages. Partial messages will result in a CRC error.
 * 
 * @param bytes     Pointer to received bytes
 * @param nbytes    Number of bytes received
 * @param send_data Pointer to response data, asssigned by callee. NULL indicates no response.
 * @param send_len  Pointer to response length, assigned by callee. Zero indicates no response.
 */
void comms_usb_hpt_receive_bytes(uint8_t *bytes, uint32_t nbytes, void **send_data, uint32_t *send_len);

/**
 * @brief Process communications tasks
 * 
 */
void comms_usb_hpt_tick(void);

#if defined(__cplusplus)
}
#endif

#endif /* !COMMS_USB_HPT_H */
