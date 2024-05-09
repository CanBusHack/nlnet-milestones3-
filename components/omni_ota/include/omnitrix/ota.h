#ifndef OMNITRIX_OTA_H_
#define OMNITRIX_OTA_H_

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <host/ble_gatt.h>

extern const struct ble_gatt_svc_def omni_ota_gatt_svr_svcs[];
#endif

void omni_ota_main(void);

#endif
