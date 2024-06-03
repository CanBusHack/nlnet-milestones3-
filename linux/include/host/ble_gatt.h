#ifndef HOST_BLE_GATT_H_
#define HOST_BLE_GATT_H_

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

enum {
    BLE_GATT_REGISTER_OP_SVC,
    BLE_GATT_REGISTER_OP_CHR,
    BLE_GATT_REGISTER_OP_DSC,
};

struct ble_gatt_svc_def {
    int TODO;
};

int ble_gatts_count_cfg(const struct ble_gatt_svc_def* defs);
int ble_gatts_add_svcs(const struct ble_gatt_svc_def* defs);

#endif
