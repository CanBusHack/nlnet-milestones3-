#ifndef HOST_BLE_HS_ADV_H_
#define HOST_BLE_HS_ADV_H_

#include <host/ble_uuid.h>
#include <stdint.h>

struct ble_hs_adv_fields {
    int flags;
    int tx_pwr_lvl_is_present;
    int tx_pwr_lvl;
    const uint8_t* name;
    int name_len;
    int name_is_complete;
    const ble_uuid16_t* uuids16;
    int num_uuids16;
    int uuids16_is_complete;
};

#endif
