#ifndef OS_OS_MBUF_H_
#define OS_OS_MBUF_H_

#include <stdint.h>

uint16_t OS_MBUF_PKTLEN(void* buf);
int ble_hs_mbuf_to_flat(void* buf, void* out, uint16_t len, uint16_t* out_len);
int os_mbuf_append(void* buf, const void* in, uint16_t len);

#endif
