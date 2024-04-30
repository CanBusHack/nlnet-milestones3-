# Omnitrix BLE Component

This component provides BLE connectivity to the rest of the firmware. It
is based heavily on the NimBLE Peripheral Example from ESP-IDF, but with
changes in how the GATT server is defined:

1.  Other components can provide their own `ble_gatt_svc_def` arrays
2.  The main component takes those arrays and provides them to the BLE
    component
3.  The BLE component allows GATT services from other components to
    coexist

## Configuration

- `OMNITRIX_ENABLE_BLE`: (default: y) enables this component. When
  disabled, there will be no BLE in the firmware.
- `OMNITRIX_BLE_DEVICE_NAME`: (default: BlinkCar v1.0) Bluetooth device
  name.
