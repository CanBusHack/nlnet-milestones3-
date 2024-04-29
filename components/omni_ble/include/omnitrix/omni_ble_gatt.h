#ifndef OMNITRIX_BLE_GATT_H_
#define OMNITRIX_BLE_GATT_H_

#include <host/ble_gatt.h>

/** BLE GATT registration callback */
void omni_ble_gatts_register_cb(struct ble_gatt_register_ctxt* ctxt, void* arg);

/** BLE GATT server init */
int omni_ble_gatt_svr_init();

#endif
