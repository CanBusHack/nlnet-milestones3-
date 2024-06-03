#ifndef HOST_BLE_GAP_H_
#define HOST_BLE_GAP_H_

#include <host/ble_hs_adv.h>

struct ble_gap_event {
    int type;
    union {
        struct {
            int status;
            int conn_handle;
        } connect;
        struct {
            int reason;
        } disconnect;
        struct {
            int reason;
        } adv_complete;
        struct {
            int conn_handle;
        } repeat_pairing;
    };
};

struct ble_gap_conn_desc {
    int peer_id_addr;
};

struct ble_gap_adv_params {
    int conn_mode;
    int disc_mode;
};

enum {
    BLE_HS_FOREVER,
    BLE_HS_ADV_F_DISC_GEN,
    BLE_HS_ADV_F_BREDR_UNSUP,
    BLE_HS_ADV_TX_PWR_LVL_AUTO,
    BLE_GAP_CONN_MODE_UND,
    BLE_GAP_DISC_MODE_GEN,
    BLE_GAP_EVENT_CONNECT,
    BLE_GAP_EVENT_DISCONNECT,
    BLE_GAP_EVENT_ADV_COMPLETE,
    BLE_GAP_EVENT_REPEAT_PAIRING,
    BLE_GAP_REPEAT_PAIRING_RETRY,
};

int ble_gap_adv_set_fields(struct ble_hs_adv_fields* fields);
int ble_gap_adv_start(int addrtype, void* unk1, int unk2, struct ble_gap_adv_params* params, int (*callback)(struct ble_gap_event*, void*), void* unk3);
int ble_gap_conn_find(int handle, struct ble_gap_conn_desc* desc);
void ble_store_util_delete_peer(int* peer_id_addr);

#endif
