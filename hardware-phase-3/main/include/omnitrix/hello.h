#ifndef OMNITRIX_HELLO_H_
#define OMNITRIX_HELLO_H_

#include <sdkconfig.h>
#ifdef CONFIG_OMNITRIX_ENABLE_HELLO

#ifdef CONFIG_OMNITRIX_ENABLE_BLE
#include <host/ble_gatt.h>

extern const struct ble_gatt_svc_def omni_hello_gatt_svr_svcs[];
#endif

void omni_hello_main(void);

#endif

#endif
