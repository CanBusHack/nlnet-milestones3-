#ifndef HOST_BLE_UUID_H_
#define HOST_BLE_UUID_H_

#include <stdint.h>

enum {
    BLE_UUID_STR_LEN = 37,
    BLE_SVC_ANS_UUID16 = 0x1811,
};

typedef struct {
    int u;
    uint16_t value;
} ble_uuid16_t;

typedef struct {
    int u;
    uint8_t value[16];
} ble_uuid128_t;

#define BLE_UUID16_INIT(v) \
    { .value = v }
#define BLE_UUID128_INIT(...)    \
    {                            \
        .value = { __VA_ARGS__ } \
    }

const char* ble_uuid_to_str(const void* uuid, char buf[BLE_UUID_STR_LEN]);

#endif
