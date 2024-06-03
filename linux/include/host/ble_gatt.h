#ifndef HOST_BLE_GATT_H_
#define HOST_BLE_GATT_H_

#include <stdint.h>

struct ble_gatt_register_ctxt {
    int op;
    union {
        struct {
            int handle;
            struct {
                unsigned char* uuid;
            }* svc_def;
        } svc;
        struct {
            int def_handle;
            int val_handle;
            struct {
                unsigned char* uuid;
            }* chr_def;
        } chr;
        struct {
            int handle;
            struct {
                unsigned char* uuid;
            }* dsc_def;
        } dsc;
    };
};

struct ble_gatt_access_ctxt {
    int op;
    void* om;
};

enum {
    BLE_GATT_REGISTER_OP_SVC,
    BLE_GATT_REGISTER_OP_CHR,
    BLE_GATT_REGISTER_OP_DSC,
    BLE_GATT_ACCESS_OP_READ_CHR,
    BLE_GATT_ACCESS_OP_WRITE_CHR,
    BLE_GATT_SVC_TYPE_PRIMARY,
    BLE_GATT_CHR_F_READ,
    BLE_GATT_CHR_F_WRITE,
};

struct ble_gatt_chr_def {
    const int* uuid;
    int (*access_cb)(uint16_t, uint16_t, struct ble_gatt_access_ctxt*, void*);
    int flags;
    uint16_t* val_handle;
};

struct ble_gatt_svc_def {
    int type;
    const int* uuid;
    const struct ble_gatt_chr_def* characteristics;
};

int ble_gatts_count_cfg(const struct ble_gatt_svc_def* defs);
int ble_gatts_add_svcs(const struct ble_gatt_svc_def* defs);

#endif
