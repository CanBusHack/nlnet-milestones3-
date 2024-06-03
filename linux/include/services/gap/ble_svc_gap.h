#ifndef SERVICES_GAP_BLE_SVC_GAP_H_
#define SERVICES_GAP_BLE_SVC_GAP_H_

const char* ble_svc_gap_device_name(void);
int ble_svc_gap_device_name_set(const char* name);
void ble_svc_gap_init(void);

#endif
