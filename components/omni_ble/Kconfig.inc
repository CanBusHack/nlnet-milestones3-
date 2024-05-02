config OMNITRIX_BLE_DEVICE_NAME
    depends on OMNITRIX_ENABLE_BLE
    string "BLE device name"
    default "BlinkCar v1.0"
