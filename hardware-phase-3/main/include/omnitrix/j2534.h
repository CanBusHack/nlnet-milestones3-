#ifndef OMNITRIX_J2534_H_
#define OMNITRIX_J2534_H_

#include <sdkconfig.h>
#ifdef CONFIG_OMNITRIX_ENABLE_J2534

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <host/ble_gatt.h>

extern const struct ble_gatt_svc_def omni_j2534_gatt_svr_svcs[];
#endif

void omni_j2534_main(void);

#endif

#endif
