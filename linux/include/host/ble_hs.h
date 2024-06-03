#ifndef HOST_BLE_HS_H_
#define HOST_BLE_HS_H_

enum {
    BLE_OWN_ADDR_PUBLIC,
};

int ble_hs_util_ensure_addr(int addrtype);
int ble_hs_id_infer_auto(int addrtype, unsigned char* ptr);
int ble_hs_id_copy_addr(int addrtype, unsigned char* buf, void* unk);

#endif
