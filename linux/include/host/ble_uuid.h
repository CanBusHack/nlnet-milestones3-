#ifndef HOST_BLE_UUID_H_
#define HOST_BLE_UUID_H_

#include <stdint.h>

enum {
    BLE_UUID_STR_LEN = 37,
    BLE_SVC_ANS_UUID16 = 0x1811,
};

typedef struct {
    uint16_t value;
} ble_uuid16_t;

ble_uuid16_t BLE_UUID16_INIT(uint16_t value);

const char* ble_uuid_to_str(const void* uuid, char buf[BLE_UUID_STR_LEN]);

#endif
