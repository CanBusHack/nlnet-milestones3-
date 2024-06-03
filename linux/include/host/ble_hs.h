#ifndef HOST_BLE_HS_H_
#define HOST_BLE_HS_H_

#include <host/ble_gatt.h>

enum {
    BLE_OWN_ADDR_PUBLIC,
    BLE_HS_IO_NO_INPUT_OUTPUT,
};

int ble_hs_util_ensure_addr(int addrtype);
int ble_hs_id_infer_auto(int addrtype, unsigned char* ptr);
int ble_hs_id_copy_addr(int addrtype, unsigned char* buf, void* unk);

extern struct {
    void (*reset_cb)(int);
    void (*sync_cb)(void);
    void (*gatts_register_cb)(struct ble_gatt_register_ctxt*, void*);
    int store_status_cb;
    int sm_io_cap;
    int sm_bonding;
    int sm_mitm;
    int sm_sc;
} ble_hs_cfg;

extern int ble_store_util_status_rr;

#endif
