#ifndef HOST_BLE_UUID_H_
#define HOST_BLE_UUID_H_

enum {
    BLE_UUID_STR_LEN = 37,
};

typedef struct {
    int TODO;
} ble_uuid16_t;

#define BLE_UUID16_INIT(x)

const char* ble_uuid_to_str(const void* uuid, char buf[BLE_UUID_STR_LEN]);

#endif
