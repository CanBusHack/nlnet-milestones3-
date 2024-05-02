config OMNITRIX_OTA_SERVICE_UUID
    depends on OMNITRIX_ENABLE_BLE && OMNITRIX_ENABLE_OTA
    string "OTA BLE serivce UUID"
    default "0ac32576-eeb0-4be5-8cc3-01cfd3a6c233"

config OMNITRIX_OTA_CHARACTERISTIC_UUID
    depends on OMNITRIX_ENABLE_BLE && OMNITRIX_ENABLE_OTA
    string "OTA BLE characteristic UUID"
    default "324278c9-c3da-4c81-aaa0-b55c1211c4cc"
