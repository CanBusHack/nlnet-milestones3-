#ifndef OMNITRIX_OTA_H_
#define OMNITRIX_OTA_H_

#include <host/ble_gatt.h>

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
extern const struct ble_gatt_svc_def omni_ota_gatt_svr_svcs[];
#endif

#endif
