#ifndef OMNITRIX_BLE_H_
#define OMNITRIX_BLE_H_

#include <host/ble_gatt.h>

/** Initialize BLE */
void omni_ble_main(const struct ble_gatt_svc_def* const* args);

#endif
