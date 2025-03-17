/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Copyright (C) 2024 CanBusHack Inc.
 *
 * This file has been modified by CanBusHack. Such modifications
 * are proprietary and are NOT available under the terms of the
 * Apache License.
 */

#include <sdkconfig.h>
#ifdef CONFIG_OMNITRIX_ENABLE_BLE

#include <assert.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <host/ble_gap.h>
#include <host/ble_gatt.h>
#include <host/ble_hs.h>
#include <host/ble_hs_adv.h>
#include <host/ble_hs_id.h>
#include <host/ble_store.h>
#include <host/ble_uuid.h>
#include <host/util/util.h>
#include <nimble/ble.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

#include <omnitrix/ble.h>
#include <omnitrix/libnvs.h>

/** Logging tag (omni_ble) */
static const char tag[] = "omni_ble";

/** Address type */
static uint8_t own_addr_type = BLE_OWN_ADDR_PUBLIC;

/** BLE GAP event callback */
static int omni_ble_gap_event_cb(struct ble_gap_event* event, void* arg);

/** Start advertising */
static void omni_ble_advertise(void) {
    const char* name = ble_svc_gap_device_name();
    static bool omit = false;
    omit = !omit;
    struct ble_hs_adv_fields fields = {
        .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
        .tx_pwr_lvl_is_present = 1,
        .tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO,
        .name = omit ? (const uint8_t*)name : NULL,
        .name_len = omit ? strlen(name) : 0,
        .name_is_complete = omit,
        .mfg_data = omit ? NULL : (const uint8_t*)"\xff\xff\x06\xcav\x1a\x90.?\xa2\x8fp\x02\xf4",  // 1CANBUSHACKISC00L
        .mfg_data_len = omit ? 0 : 14,
    };

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(tag, "error setting advertisement data: %d", rc);
        return;
    }

    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
    };

    rc = ble_gap_adv_start(own_addr_type, NULL, 10000, &adv_params, omni_ble_gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(tag, "error enabling advertisement: %d", rc);
    }
}

/** BLE GAP event callback */
static int omni_ble_gap_event_cb(struct ble_gap_event* event, void* arg) {
    assert(event);
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(tag, "connection %s: %d", event->connect.status == 0 ? "established" : "failed", event->connect.status);
        if (event->connect.status == 0) {
            struct ble_gap_conn_desc desc;
            int rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            ESP_LOGI(tag, "connection: %p", (void*)&desc); // TODO: print struct fields
        } else {
            omni_ble_advertise();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(tag, "disconnect: %d", event->disconnect.reason);
        omni_ble_advertise();
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(tag, "advertise complete: %d", event->adv_complete.reason);
        omni_ble_advertise();
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING: {
        struct ble_gap_conn_desc desc;
        int rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        if (rc == 0) {
            ble_store_util_delete_peer(&desc.peer_id_addr);
        }
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    }

    default:
        ESP_LOGI(tag, "unknown event type: %d", event->type);
        return 0;
    }
}

/** Bluetooth stack reset callback (error) */
static void omni_ble_on_reset(int reason) {
    ESP_LOGE(tag, "Resetting state: %d", reason);
}

/** Bluetooth stack sync callback (at startup and after reset) */
static void omni_ble_on_sync(void) {
    ESP_LOGI(tag, "nimble sync");

    int rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(tag, "error determining address type: %d", rc);
        return;
    }

    uint8_t addr_val[6] = { 0 };
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);
    if (rc != 0) {
        ESP_LOGE(tag, "error determining address: %d", rc);
        return;
    }

    ESP_LOGI(tag, "Device Name: %s", CONFIG_OMNITRIX_BLE_DEVICE_NAME);
    ESP_LOGI(tag, "Device Address: %02X:%02X:%02X:%02X:%02X:%02X", addr_val[5], addr_val[4], addr_val[3], addr_val[2], addr_val[1], addr_val[0]);

    omni_ble_advertise();
}

/** BLE GATT registration callback */
void omni_ble_gatts_register_cb(struct ble_gatt_register_ctxt* ctxt, void* arg) {
    assert(ctxt);
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(tag, "registered service %s with handle %d", ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf), ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(tag, "registering characteristic %s with def_handle %d val_handle %d", ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf), ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(tag, "registering descriptor %s with handle %d", ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf), ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

/** BLE host task */
static void omni_ble_host_task(void* param) {
    ESP_LOGI(tag, "BLE host task started");
    nimble_port_run();
    ESP_LOGE(tag, "BLE host task error");
    nimble_port_freertos_deinit();
}

/** Initialize BLE */
void omni_ble_main(const struct ble_gatt_svc_def* const* args) {
    assert(args);
    omni_libnvs_main();
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(tag, "Failed to init nimble: %d", ret);
        return;
    }

    ble_hs_cfg.reset_cb = omni_ble_on_reset;
    ble_hs_cfg.sync_cb = omni_ble_on_sync;
    ble_hs_cfg.gatts_register_cb = omni_ble_gatts_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding = 0;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_sc = 1;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    for (int i = 0; i < 10 && args[i]; i++) {
        int rc = ble_gatts_count_cfg(args[i]);
        if (rc != 0) {
            ESP_LOGE(tag, "Failed to init components: %d", ret);
            return;
        }
        if (i >= 9) {
            ESP_LOGE(tag, "Too many components or incorrect argument!");
            return;
        }
    }

    for (int i = 0; i < 10 && args[i]; i++) {
        int rc = ble_gatts_add_svcs(args[i]);
        if (rc != 0) {
            ESP_LOGE(tag, "Failed to init components: %d", ret);
            return;
        }
        if (i >= 9) {
            ESP_LOGE(tag, "Too many components or incorrect argument!");
            return;
        }
    }

    int rc = ble_svc_gap_device_name_set(CONFIG_OMNITRIX_BLE_DEVICE_NAME);
    assert(rc == 0);

    /** For some reason, this isn't in any header files?! */
    void ble_store_config_init(void);
    ble_store_config_init();

    nimble_port_freertos_init(omni_ble_host_task);
}

#endif
