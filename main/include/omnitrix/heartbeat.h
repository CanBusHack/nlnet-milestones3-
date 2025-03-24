// heartbeat.h
#ifndef OMNITRIX_HEARTBEAT_H_
#define OMNITRIX_HEARTBEAT_H_

#include <sdkconfig.h>
#ifdef CONFIG_OMNITRIX_ENABLE_HEARTBEAT

#include <esp_err.h>
#include <host/ble_gatt.h>

/** Initialize heartbeat component */
void omni_heartbeat_main(void);

/** External service definition */
extern const struct ble_gatt_svc_def omni_heartbeat_gatt_svr_svcs[];

/** Check if connection is alive */
bool omni_heartbeat_is_alive(void);

/** Get last heartbeat timestamp */
uint32_t omni_heartbeat_last_time(void);

/** Update connection status for heartbeat service */
void omni_heartbeat_connection_update(uint16_t handle, bool connected);

#endif
#endif